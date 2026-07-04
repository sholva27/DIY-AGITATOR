#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Preferences.h>

/* --- FEATURE FLAGS --- */
#define ENABLE_HALL
#define ENABLE_INA219
#define ENABLE_KILL_SWITCH

/* --- CONFIGURATION --- */
#define FAN_PPR 2
#define MIN_PWM 45          // Minimum duty to keep fan spinning
#define KICKSTART_MS 400    // Boost at startup to overcome friction
#define PID_INTERVAL 100
#define DISPLAY_INTERVAL 200
#define TELEMETRY_INTERVAL 1000
#define HEARTBEAT_TIMEOUT 5000

/* --- PIN DEFINITIONS --- */
#define HALL_PIN 4          // Stir Bar Sensor - MOUNT LATERALLY
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
enum ControlMode { IDLE, LOCAL, REMOTE_LOCKED, STOPPED, FAN_STALLED, DECOUPLED, TIMER_DONE };
volatile ControlMode currentMode = IDLE;
volatile bool system_on = false;

/* --- DATA STRUCTURES --- */
#pragma pack(push, 1)
typedef struct {
  uint8_t start_byte;   // 0xAA
  uint8_t target_speed; // 0-100%
  uint8_t command;      // 1: Lock, 0: Unlock
  uint8_t crc8;
} stirrer_command_t;

typedef struct {
  uint8_t start_byte;   // 0xBB
  uint8_t status;       // ControlMode enum
  uint8_t setpoint;     // 0-100%
  int16_t bar_rpm;
  int16_t fan_rpm;
  uint8_t crc8;
} stirrer_telemetry_t;
#pragma pack(pop)

/* --- GLOBALS --- */
volatile long targetSetpoint = 0;
volatile int barPulseCount = 0;
volatile int fanPulseCount = 0;
int actualBarRPM = 0, actualFanRPM = 0;
float current_pwm_val = 0;
uint8_t last_master_mac[6] = {0};
unsigned long last_master_heartbeat = 0;

Preferences prefs;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

/* --- PID --- */
float Kp = 0.5, Ki = 0.1, Kd = 0.05;
float integral = 0, lastError = 0;

/* --- ISRs --- */
void IRAM_ATTR onFanPulse() { fanPulseCount++; }
void IRAM_ATTR onBarPulse() { barPulseCount++; }

void IRAM_ATTR handleEncoder() {
  static int lastEncoded = 0;
  int MSB = digitalRead(ENCODER_CLK);
  int LSB = digitalRead(ENCODER_DT);
  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;

  if (currentMode != REMOTE_LOCKED) {
    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) targetSetpoint++;
    if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) targetSetpoint--;
    targetSetpoint = constrain(targetSetpoint, 0, 100);
    if (targetSetpoint > 0 && !system_on) { system_on = true; currentMode = LOCAL; }
  }
  lastEncoded = encoded;
}

/* --- CHECKSUM --- */
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

/* --- HELPERS --- */
String getStatusString(ControlMode mode) {
  switch(mode) {
    case IDLE: return "IDLE";
    case LOCAL: return "LOCAL";
    case REMOTE_LOCKED: return "REMOTE LOCK";
    case STOPPED: return "STOPPED";
    case FAN_STALLED: return "FAN STALL";
    case DECOUPLED: return "DECOUPLED";
    default: return "UNKNOWN";
  }
}

void setRGB(int r, int g, int b) {
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(LED_R, r); ledcWrite(LED_G, g); ledcWrite(LED_B, b);
  #else
    ledcWrite(1, r); ledcWrite(2, g); ledcWrite(3, b);
  #endif
}

void updateFeedback() {
  unsigned long now = millis();
  // 1. RGB Breathing/State
  if (currentMode < STOPPED) {
    int b = (sin(now * 0.003) + 1) * 127;
    setRGB(0, b, 0); // Breathing Green
  } else if (currentMode == STOPPED) {
    setRGB(255, 0, 0); // Solid Red
  } else {
    if ((now / 200) % 2 == 0) setRGB(255, 0, 0); else setRGB(0, 0, 0); // Flashing Red
  }

  // 2. Alarm Buzzer (Non-blocking)
  if (currentMode >= FAN_STALLED) {
    if ((now / 500) % 2 == 0) digitalWrite(BUZZER_PIN, HIGH); else digitalWrite(BUZZER_PIN, LOW);
  } else { digitalWrite(BUZZER_PIN, LOW); }
}

void sendTelemetry() {
  stirrer_telemetry_t telem;
  telem.start_byte = 0xBB;
  telem.status = (uint8_t)currentMode;
  telem.setpoint = (uint8_t)targetSetpoint;
  telem.bar_rpm = (int16_t)actualBarRPM;
  telem.fan_rpm = (int16_t)actualFanRPM;
  telem.crc8 = calc_crc8((uint8_t*)&telem, sizeof(telem) - 1);

  if (last_master_mac[0] != 0) {
    esp_now_send(last_master_mac, (uint8_t *)&telem, sizeof(telem));
  }
  Serial1.write((uint8_t*)&telem, sizeof(telem));
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t * recv_info, const uint8_t *incoming, int len) {
  if (len == sizeof(stirrer_command_t) && incoming[0] == 0xAA) {
    if (calc_crc8(incoming, len - 1) == incoming[len-1]) {
       stirrer_command_t cmd; memcpy(&cmd, incoming, len);
       targetSetpoint = cmd.target_speed;
       currentMode = (cmd.command == 1) ? REMOTE_LOCKED : LOCAL;
       system_on = (targetSetpoint > 0);
       memcpy(last_master_mac, recv_info->src_addr, 6);
       last_master_heartbeat = millis();
    }
  }
}
#else
void OnDataRecv(const uint8_t * mac, const uint8_t *incoming, int len) {
  if (len == sizeof(stirrer_command_t) && incoming[0] == 0xAA) {
    if (calc_crc8(incoming, len - 1) == incoming[len-1]) {
       stirrer_command_t cmd; memcpy(&cmd, incoming, len);
       targetSetpoint = cmd.target_speed;
       currentMode = (cmd.command == 1) ? REMOTE_LOCKED : LOCAL;
       system_on = (targetSetpoint > 0);
       memcpy(last_master_mac, mac, 6);
       last_master_heartbeat = millis();
    }
  }
}
#endif

void ControlTask(void *pvParameters) {
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(FAN_PWM_PIN, 25000, 8);
  #else
    ledcSetup(0, 25000, 8); ledcAttachPin(FAN_PWM_PIN, 0);
  #endif

  unsigned long lastPID = millis();
  unsigned long startMoment = 0;
  unsigned long safetyTimer = 0;

  for (;;) {
    unsigned long now = millis();
    if (now - lastPID >= PID_INTERVAL) {
      noInterrupts();
      int f = fanPulseCount; fanPulseCount = 0;
      int b = barPulseCount; barPulseCount = 0;
      interrupts();
      actualFanRPM = (f * 60 * (1000 / PID_INTERVAL)) / FAN_PPR;
      actualBarRPM = (b * 60 * (1000 / PID_INTERVAL));

      if (system_on && targetSetpoint > 0) {
        digitalWrite(KILL_SWITCH_PIN, HIGH);
        if (startMoment == 0) startMoment = now;

        if (now - startMoment < KICKSTART_MS) {
          current_pwm_val = 255;
        } else {
          float targetRPM = targetSetpoint * 30;
          float error = targetRPM - actualBarRPM;
          if (actualBarRPM < 50 && actualFanRPM > 200) error = targetRPM - actualFanRPM; // Fallback
          integral += error * (PID_INTERVAL / 1000.0);
          float output = (Kp * error) + (Ki * integral);
          float ff = (targetSetpoint / 100.0) * 255.0;
          current_pwm_val = constrain(ff + output, MIN_PWM, 255);
        }

        // Safety Logic
        if (now - startMoment > 2000) {
           if (actualFanRPM < 100 || (actualBarRPM < 50 && actualFanRPM > 500)) {
             if (safetyTimer == 0) safetyTimer = now;
             if (now - safetyTimer > 4000) {
               currentMode = (actualFanRPM < 100) ? FAN_STALLED : DECOUPLED;
               system_on = false; targetSetpoint = 0;
             }
           } else safetyTimer = 0;
        }
      } else {
        current_pwm_val = 0; integral = 0; startMoment = 0; safetyTimer = 0;
        digitalWrite(KILL_SWITCH_PIN, LOW);
      }

      #if ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcWrite(FAN_PWM_PIN, (int)current_pwm_val);
      #else
        ledcWrite(0, (int)current_pwm_val);
      #endif
      lastPID = now;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void InterfaceTask(void *pvParameters) {
  Serial1.begin(115200, SERIAL_8N1, BIO_RX, BIO_TX);
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);

  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(LED_R, 5000, 8); ledcAttach(LED_G, 5000, 8); ledcAttach(LED_B, 5000, 8);
  #else
    ledcSetup(1, 5000, 8); ledcAttachPin(LED_R, 1);
    ledcSetup(2, 5000, 8); ledcAttachPin(LED_G, 2);
    ledcSetup(3, 5000, 8); ledcAttachPin(LED_B, 3);
  #endif

  Wire.begin(OLED_SDA, OLED_SCL);
  bool oled = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  unsigned long lastDisp = 0, lastTelem = 0;
  static bool lastBtnState = HIGH;

  for (;;) {
    unsigned long now = millis();

    // Serial Control
    if (Serial1.available() >= sizeof(stirrer_command_t)) {
       uint8_t head = Serial1.read();
       if (head == 0xAA) {
          uint8_t buf[sizeof(stirrer_command_t)]; buf[0] = 0xAA;
          Serial1.readBytes(&buf[1], sizeof(stirrer_command_t)-1);
          if (calc_crc8(buf, sizeof(buf)-1) == buf[sizeof(buf)-1]) {
            stirrer_command_t sc; memcpy(&sc, buf, sizeof(sc));
            targetSetpoint = sc.target_speed;
            currentMode = SERIAL_CTL; system_on = (targetSetpoint > 0);
          }
       }
    }

    // Remote Lock Heartbeat
    if (currentMode == REMOTE_LOCKED && now - last_master_heartbeat > HEARTBEAT_TIMEOUT) {
      system_on = false; currentMode = IDLE; targetSetpoint = 0;
    }

    // Button Toggle (Non-blocking)
    bool btn = digitalRead(ENCODER_SW);
    if (btn == LOW && lastBtnState == HIGH) {
       system_on = !system_on;
       currentMode = system_on ? LOCAL : STOPPED;
    }
    lastBtnState = btn;

    if (oled && now - lastDisp >= DISPLAY_INTERVAL) {
      display.clearDisplay();
      display.setCursor(0,0); display.print(getStatusString(currentMode));
      display.drawLine(0, 10, 127, 10, 1);
      display.setCursor(0,15); display.print("Set: "); display.print(targetSetpoint); display.print("%");
      display.setCursor(0,30); display.setTextSize(2); display.print(actualBarRPM); display.print(" RPM");
      display.setTextSize(1); display.setCursor(0,55); display.print("Fan: "); display.print(actualFanRPM);
      display.display();
      lastDisp = now;
    }

    if (now - lastTelem >= TELEMETRY_INTERVAL) {
      sendTelemetry(); lastTelem = now;
    }

    updateFeedback();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BIO_RX, INPUT); pinMode(BIO_TX, OUTPUT);
  pinMode(KILL_SWITCH_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(HALL_PIN, INPUT_PULLUP);
  pinMode(FAN_TACHO_PIN, INPUT_PULLUP);
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(FAN_TACHO_PIN), onFanPulse, FALLING);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN), onBarPulse, FALLING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), handleEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT), handleEncoder, CHANGE);

  xTaskCreatePinnedToCore(ControlTask, "Ctrl", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(InterfaceTask, "UI", 4096, NULL, 2, NULL, 0);
}

void loop() { vTaskDelete(NULL); }
