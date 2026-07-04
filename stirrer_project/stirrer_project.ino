#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Preferences.h>

/* --- FEATURE FLAGS --- */
#define ENABLE_HALL
//#define ENABLE_INA219
#define ENABLE_KILL_SWITCH

/* --- CONFIGURATION --- */
#define STRUCT_VERSION 0x02
#define FAN_PPR 2
#define MIN_PWM 45
#define KICKSTART_MS 400
#define PID_INTERVAL 100
#define DISPLAY_INTERVAL 200
#define TELEMETRY_INTERVAL 1000
#define HEARTBEAT_TIMEOUT 5000
#define CALIBRATION_POINTS 11
#define SCREEN_ADDRESS 0x3C

/* --- PIN DEFINITIONS --- */
#define HALL_PIN 4
#define KILL_SWITCH_PIN 5
#define BIO_RX 6
#define BIO_TX 7
#define OLED_SDA 8
#define OLED_SCL 9
#define ENCODER_CLK 10
#define ENCODER_DT  11
#define ENCODER_SW  12
#define FAN_TACHO_PIN 13
#define FAN_PWM_PIN 14
#define BUZZER_PIN 15
#define LED_R 16
#define LED_G 17
#define LED_B 18

/* --- STATE MACHINE --- */
enum ControlMode { IDLE, LOCAL, REMOTE_LOCKED, STOPPED, FAN_STALL, DECOUPLED, TIMER_DONE, SERIAL_CTL };
volatile ControlMode currentMode = IDLE;
volatile bool system_on = false;

/* --- DATA STRUCTURES --- */
#pragma pack(push, 1)
typedef struct {
  uint8_t start_byte;   // 0xAA
  uint8_t version;
  uint8_t target_speed; // 0-100%
  uint8_t command;      // 0: Release, 1: Acquire/Lock
  uint8_t crc8;
} stirrer_command_t;

typedef struct {
  uint8_t start_byte;   // 0xBB
  uint8_t version;
  uint8_t mode;
  uint8_t setpoint;
  int16_t bar_rpm;
  int16_t fan_rpm;
  uint16_t current_ma;
  uint32_t timer_rem;
  uint8_t crc8;
} stirrer_telemetry_t;
#pragma pack(pop)

/* --- GLOBALS --- */
volatile long targetSetpoint = 0;
long lastSavedSetpoint = -1;
volatile unsigned long lastChangeTime = 0;
volatile int barPulseCount = 0;
volatile bool isCalibrating = false;
volatile bool settingTimer = false;
volatile long timerMinutes = 0;
volatile int fanPulseCount = 0;
int actualBarRPM = 0, actualFanRPM = 0;
float current_pwm_val = 0;
unsigned long remainingSeconds = 0;
bool oled_present = false;
bool bar_sensor_valid = false;
uint8_t last_master_mac[6] = {0};
unsigned long last_heartbeat = 0;

int calibration_map[CALIBRATION_POINTS]; // PWM index to RPM

Preferences prefs;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

/* --- PID --- */
float Kp = 0.5, Ki = 0.1, Kd = 0.05;
float integral = 0, lastError = 0;

/* --- UTILS --- */
uint8_t calc_crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  while (len--) {
    uint8_t extract = *data++;
    for (uint8_t tempI = 8; tempI; tempI--) {
      uint8_t sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) crc ^= 0x8C;
      extract >>= 1;
    }
  }
  return crc;
}

/* --- COMMS --- */
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t * recv_info, const uint8_t *incoming, int len) {
  if (len == sizeof(stirrer_command_t) && incoming[0] == 0xAA && incoming[1] == STRUCT_VERSION) {
    stirrer_command_t cmd; memcpy(&cmd, incoming, len);
    targetSetpoint = cmd.target_speed;
    if (cmd.command == 1) currentMode = REMOTE_LOCKED;
    else if (currentMode == REMOTE_LOCKED) currentMode = IDLE;
    memcpy(last_master_mac, recv_info->src_addr, 6);
    last_heartbeat = millis();
    system_on = (targetSetpoint > 0);
  }
}
#else
void OnDataRecv(const uint8_t * mac, const uint8_t *incoming, int len) {
  if (len == sizeof(stirrer_command_t) && incoming[0] == 0xAA && incoming[1] == STRUCT_VERSION) {
    stirrer_command_t cmd; memcpy(&cmd, incoming, len);
    targetSetpoint = cmd.target_speed;
    if (cmd.command == 1) currentMode = REMOTE_LOCKED;
    else if (currentMode == REMOTE_LOCKED) currentMode = IDLE;
    memcpy(last_master_mac, mac, 6);
    last_heartbeat = millis();
    system_on = (targetSetpoint > 0);
  }
}
#endif

void handleSerial() {
  if (Serial1.available() >= sizeof(stirrer_command_t)) {
    if (Serial1.peek() == 0xAA) {
      uint8_t buf[sizeof(stirrer_command_t)];
      Serial1.readBytes(buf, sizeof(stirrer_command_t));
      if (buf[1] == STRUCT_VERSION && calc_crc8(buf, sizeof(stirrer_command_t)-1) == buf[sizeof(stirrer_command_t)-1]) {
        stirrer_command_t cmd; memcpy(&cmd, buf, sizeof(stirrer_command_t));
        targetSetpoint = cmd.target_speed;
        currentMode = SERIAL_CTL;
        system_on = (targetSetpoint > 0);
      }
    } else { Serial1.read(); }
  }
}

void sendTelemetry() {
  stirrer_telemetry_t telem;
  telem.start_byte = 0xBB;
  telem.version = STRUCT_VERSION;
  telem.mode = (uint8_t)currentMode;
  telem.setpoint = (uint8_t)targetSetpoint;
  telem.bar_rpm = (int16_t)actualBarRPM;
  telem.fan_rpm = (int16_t)actualFanRPM;
  telem.current_ma = 0;
  telem.timer_rem = (uint32_t)remainingSeconds;
  telem.crc8 = calc_crc8((uint8_t*)&telem, sizeof(telem) - 1);
  if (last_master_mac[0] != 0) esp_now_send(last_master_mac, (uint8_t *)&telem, sizeof(telem));
  Serial1.write((uint8_t*)&telem, sizeof(telem));
}

/* --- ISRs --- */
void IRAM_ATTR onFanPulse() { fanPulseCount++; }
void IRAM_ATTR onBarPulse() { barPulseCount++; }

void IRAM_ATTR handleEncoder() {
  static int lastEncoded = 0;
  int MSB = digitalRead(ENCODER_CLK);
  int LSB = digitalRead(ENCODER_DT);
  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;
  if (currentMode != REMOTE_LOCKED && !isCalibrating) {
    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
       if (settingTimer) timerMinutes += 5;
       else targetSetpoint++;
       lastChangeTime = millis();
    }
    if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
       if (settingTimer) { if (timerMinutes >= 5) timerMinutes -= 5; }
       else targetSetpoint--;
       lastChangeTime = millis();
    }
    targetSetpoint = constrain(targetSetpoint, 0, 100);
    timerMinutes = constrain(timerMinutes, 0, 1440);
    if (!settingTimer && targetSetpoint > 0 && !system_on) { system_on = true; currentMode = LOCAL; }
  }
  lastEncoded = encoded;
}

void ControlTask(void *pvParameters);
void InterfaceTask(void *pvParameters);

void setup() {
  Serial.begin(115200);
  xTaskCreatePinnedToCore(ControlTask, "Control", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(InterfaceTask, "Interface", 8192, NULL, 2, NULL, 0);
}

void loop() { vTaskDelete(NULL); }

void ControlTask(void *pvParameters) {
  pinMode(FAN_TACHO_PIN, INPUT_PULLUP);
  pinMode(HALL_PIN, INPUT_PULLUP);
  pinMode(KILL_SWITCH_PIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(FAN_TACHO_PIN), onFanPulse, FALLING);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN), onBarPulse, FALLING);

  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(FAN_PWM_PIN, 25000, 8);
  #else
    ledcSetup(0, 25000, 8); ledcAttachPin(FAN_PWM_PIN, 0);
  #endif

  unsigned long lastPID = millis();
  unsigned long kickstartStart = 0;

  for (;;) {
    unsigned long now = millis();
    if (now - lastPID >= PID_INTERVAL) {
      noInterrupts();
      int fCount = fanPulseCount; fanPulseCount = 0;
      int bCount = barPulseCount; barPulseCount = 0;
      interrupts();
      actualFanRPM = (fCount * 60 * 10) / FAN_PPR;
      actualBarRPM = (bCount * 60 * 10);

      // Check Hall Crosstalk: if bar RPM perfectly matches fan RPM for too long, sensor is misplaced
      // Bar RPM calculation: (bCount * 60 * 10). Fan RPM: (fCount * 60 * 10) / 2.
      // If locked, actualBarRPM == actualFanRPM.
      if (system_on && actualBarRPM > 100 && abs(actualBarRPM - actualFanRPM) < 50) {
        bar_sensor_valid = false;
      } else if (actualBarRPM > 50) {
        bar_sensor_valid = true;
      }

      if (isCalibrating) {
         for (int i=0; i<CALIBRATION_POINTS; i++) {
            current_pwm_val = 50 + (i * 20);
            #if ESP_ARDUINO_VERSION_MAJOR >= 3
              ledcWrite(FAN_PWM_PIN, (int)current_pwm_val);
            #else
              ledcWrite(0, (int)current_pwm_val);
            #endif
            vTaskDelay(pdMS_TO_TICKS(2000));
            noInterrupts(); fanPulseCount = 0; interrupts();
            vTaskDelay(pdMS_TO_TICKS(1000));
            noInterrupts(); int fC = fanPulseCount; interrupts();
            calibration_map[i] = (fC * 60) / FAN_PPR;
         }
         isCalibrating = false;
         system_on = false;
      } else if (system_on && targetSetpoint > 0) {
        if (remainingSeconds == 0 && timerMinutes > 0) {
            system_on = false; currentMode = TIMER_DONE;
        }
        digitalWrite(KILL_SWITCH_PIN, HIGH);
        if (kickstartStart == 0) kickstartStart = now;
        if (now - kickstartStart < KICKSTART_MS) current_pwm_val = 255;
        else {
          float targetRPM = targetSetpoint * 30;
          float error = targetRPM - (bar_sensor_valid ? actualBarRPM : actualFanRPM);
          integral += error * 0.1;
          float output = (Kp * error) + (Ki * integral);
          current_pwm_val = constrain((targetSetpoint * 2.55) + output, MIN_PWM, 255);
        }
      } else {
        current_pwm_val = 0; integral = 0; kickstartStart = 0;
        digitalWrite(KILL_SWITCH_PIN, LOW);
      }
      #if ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcWrite(FAN_PWM_PIN, (int)current_pwm_val);
      #else
        ledcWrite(0, (int)current_pwm_val);
      #endif

      // Safety Checks
      if (system_on && targetSetpoint > 25 && (now - kickstartStart > 2000)) {
        if (actualFanRPM < 100) {
           currentMode = FAN_STALL; system_on = false; targetSetpoint = 0;
        } else if (bar_sensor_valid && actualBarRPM < 50 && actualFanRPM > 500) {
           currentMode = DECOUPLED; system_on = false; targetSetpoint = 0;
        }
      }
      lastPID = now;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setLED(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(LED_R, 255-r); analogWrite(LED_G, 255-g); analogWrite(LED_B, 255-b);
}

void InterfaceTask(void *pvParameters) {
  prefs.begin("stirrer", true);
  targetSetpoint = prefs.getLong("setpoint", 0);
  lastSavedSetpoint = targetSetpoint;
  prefs.end();

  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial1.begin(115200, SERIAL_8N1, BIO_RX, BIO_TX);
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) oled_present = true;

  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), handleEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT), handleEncoder, CHANGE);

  unsigned long lastDisp = 0, lastTelem = 0, lastBtn = 0;
  for (;;) {
    unsigned long now = millis();
    handleSerial();
    if (currentMode == REMOTE_LOCKED && now - last_heartbeat > HEARTBEAT_TIMEOUT) {
      system_on = false; currentMode = IDLE; targetSetpoint = 0;
    }

    if (digitalRead(ENCODER_SW) == LOW && now - lastBtn > 300) {
      unsigned long pressStart = now;
      while(digitalRead(ENCODER_SW) == LOW && millis() - pressStart < 3000) { vTaskDelay(10); }
      unsigned long duration = millis() - pressStart;
      if (duration >= 3000) {
         isCalibrating = true;
      } else if (duration >= 1000) {
         settingTimer = !settingTimer;
         if (!settingTimer && timerMinutes > 0) remainingSeconds = timerMinutes * 60;
      } else {
         system_on = !system_on;
         if (system_on && targetSetpoint == 0) targetSetpoint = 30;
         if (system_on && timerMinutes > 0) remainingSeconds = timerMinutes * 60;
         if (currentMode == TIMER_DONE) currentMode = LOCAL;
      }
      lastBtn = now;
    }

    // Debounced Persistence
    if (targetSetpoint != lastSavedSetpoint && now - lastChangeTime > 5000) {
      prefs.begin("stirrer", false);
      prefs.putLong("setpoint", targetSetpoint);
      prefs.end();
      lastSavedSetpoint = targetSetpoint;
    }

    if (oled_present && now - lastDisp >= DISPLAY_INTERVAL) {
      display.clearDisplay();
      if (isCalibrating) {
         display.setCursor(0,0); display.print("CALIBRATING...");
         display.setCursor(0, 20); display.print("PWM: "); display.print(current_pwm_val);
         display.display();
         lastDisp = now;
         continue;
      }
      display.setCursor(0,0);
      if (currentMode == TIMER_DONE) display.print("DONE");
      else display.print(system_on ? "ON " : "OFF ");

      if (currentMode == REMOTE_LOCKED) display.print(" LOCK");
      if (settingTimer) display.print(" [T-SET]");

      display.setCursor(0, 12);
      display.print("Set: "); display.print(targetSetpoint); display.print("%");

      display.setCursor(80, 12);
      if (timerMinutes > 0) {
        if (system_on) { display.print(remainingSeconds/60); display.print("m"); }
        else { display.print("T:"); display.print(timerMinutes); display.print("m"); }
      }

      display.setCursor(0, 28); display.setTextSize(2);
      display.print(bar_sensor_valid ? actualBarRPM : actualFanRPM);
      display.print(" RPM");

      display.setTextSize(1);
      display.setCursor(0, 54);
      if (currentMode == DECOUPLED) display.print("!! DECOUPLED !!");
      else if (currentMode == FAN_STALL) display.print("!! FAN STALL !!");
      else if (!bar_sensor_valid) display.print("[ESTIMATED]");

      display.display();
      lastDisp = now;
    }

    if (now - lastTelem >= TELEMETRY_INTERVAL) {
      sendTelemetry();
      if (system_on && remainingSeconds > 0) remainingSeconds--;
      lastTelem = now;
    }

    // Status LED & Buzzer logic
    if (currentMode == FAN_STALL || currentMode == DECOUPLED) {
      setLED(255, 0, 0); // Red
      if ((now / 500) % 2) digitalWrite(BUZZER_PIN, HIGH); else digitalWrite(BUZZER_PIN, LOW);
    } else if (currentMode == TIMER_DONE) {
      setLED(255, 255, 0); // Yellow
      if ((now / 1000) % 2) digitalWrite(BUZZER_PIN, HIGH); else digitalWrite(BUZZER_PIN, LOW);
    } else if (currentMode == REMOTE_LOCKED) {
      setLED(0, 0, 255); // Blue
      digitalWrite(BUZZER_PIN, LOW);
    } else if (system_on) {
      setLED(0, 255, 0); // Green
      digitalWrite(BUZZER_PIN, LOW);
    } else {
      setLED(20, 20, 20); // Dim White standby
      digitalWrite(BUZZER_PIN, LOW);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
