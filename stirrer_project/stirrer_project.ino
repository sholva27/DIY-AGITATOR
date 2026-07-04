#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Preferences.h>
#include <Adafruit_INA219.h>

/* --- FEATURE FLAGS --- */
#define ENABLE_HALL
#define ENABLE_INA219
#define ENABLE_KILL_SWITCH

/* --- CONFIGURATION --- */
#define FAN_PPR 2
#define SCREEN_ADDRESS 0x3C
#define I2C_SPEED 400000
#define TELEMETRY_INTERVAL 1000
#define DISPLAY_INTERVAL 200
#define PID_INTERVAL 100
#define HEARTBEAT_TIMEOUT 5000

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
enum ControlMode { IDLE, LOCAL, REMOTE_LOCKED, SERIAL_CTL, STOPPED, FAN_STALLED, DECOUPLED, TIMER_DONE };
volatile ControlMode currentMode = IDLE;
volatile bool system_on = false;

/* --- DATA STRUCTURES --- */
#pragma pack(push, 1)
typedef struct {
  uint8_t start_byte;   // 0xAA
  uint8_t len;
  uint8_t target_speed; // 0-100%
  uint8_t command;      // 0: Release, 1: Acquire/Lock
  uint8_t crc8;
} stirrer_command_t;

typedef struct {
  uint8_t start_byte;   // 0xBB
  uint8_t len;
  uint8_t status;
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
volatile long timerMinutes = 0;
volatile int barPulseCount = 0;
volatile int fanPulseCount = 0;
volatile bool isTimerActive = false;
volatile bool editTimerMode = false;

int actualBarRPM = 0;
int actualFanRPM = 0;
float current_pwm_val = 0;
float current_ma_val = 0;
unsigned long remainingSeconds = 0;
bool oled_present = false;
bool ina219_present = false;
uint8_t last_master_mac[6] = {0};
unsigned long last_master_heartbeat = 0;

stirrer_command_t incomingCmd = {0xAA, sizeof(stirrer_command_t), 0, 0, 0};
stirrer_telemetry_t outgoingData;

Preferences prefs;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
Adafruit_INA219 ina219;

/* --- PID & CONTROL --- */
float Kp = 0.8, Ki = 0.2, Kd = 0.05;
float integral = 0, lastError = 0;

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
    case SERIAL_CTL: return "SERIAL";
    case STOPPED: return "STOPPED";
    case FAN_STALLED: return "FAN STALL";
    case DECOUPLED: return "DECOUPLED";
    case TIMER_DONE: return "DONE!";
    default: return "UNKNOWN";
  }
}

void IRAM_ATTR onFanPulse() { fanPulseCount++; }
void IRAM_ATTR onBarPulse() { barPulseCount++; }

void IRAM_ATTR handleEncoder() {
  static int lastEncoded = 0;
  int MSB = digitalRead(ENCODER_CLK);
  int LSB = digitalRead(ENCODER_DT);
  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;

  if (currentMode != REMOTE_LOCKED) {
    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
      if (editTimerMode) timerMinutes++; else targetSetpoint++;
    }
    if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
      if (editTimerMode) { if (timerMinutes > 0) timerMinutes--; } else targetSetpoint--;
    }
    targetSetpoint = constrain(targetSetpoint, 0, 100);
    timerMinutes = constrain(timerMinutes, 0, 999);
    if (targetSetpoint > 0 && !system_on) { system_on = true; currentMode = LOCAL; }
  }
  lastEncoded = encoded;
}

void setRGB(int r, int g, int b) {
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(LED_R, r); ledcWrite(LED_G, g); ledcWrite(LED_B, b);
  #else
    ledcWrite(LED_CHAN_R, r); ledcWrite(LED_CHAN_G, g); ledcWrite(LED_CHAN_B, b);
  #endif
}

void updateVisuals() {
  unsigned long now = millis();
  // 1. RGB LED Status
  if (currentMode < STOPPED) {
    int b = (sin(now * 0.003) + 1) * 127;
    setRGB(0, b, 0); // Breathing Green
  } else if (currentMode == STOPPED) {
    setRGB(255, 0, 0); // Solid Red
  } else {
    if ((now / 200) % 2 == 0) setRGB(255, 0, 0); else setRGB(0, 0, 0); // Flashing Red
  }

  // 2. Non-blocking Buzzer Alarm
  if (currentMode >= FAN_STALLED) {
    if ((now / 500) % 2 == 0) digitalWrite(BUZZER_PIN, HIGH);
    else digitalWrite(BUZZER_PIN, LOW);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

/* --- COMMUNICATION HELPERS --- */
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t * recv_info, const uint8_t *incoming, int len) {
  if (len == sizeof(stirrer_command_t) && incoming[0] == 0xAA) {
    uint8_t calc = calc_crc8(incoming, len - 1);
    if (calc == incoming[len-1]) {
      memcpy(&incomingCmd, incoming, len);
      memcpy(last_master_mac, recv_info->src_addr, 6);
      last_master_heartbeat = millis();
      targetSetpoint = incomingCmd.target_speed;
      if (incomingCmd.command == 1) { currentMode = REMOTE_LOCKED; system_on = true; }
      else if (currentMode == REMOTE_LOCKED) { currentMode = IDLE; }
    }
  }
}
#else
void OnDataRecv(const uint8_t * mac, const uint8_t *incoming, int len) {
  if (len == sizeof(stirrer_command_t) && incoming[0] == 0xAA) {
    uint8_t calc = calc_crc8(incoming, len - 1);
    if (calc == incoming[len-1]) {
      memcpy(&incomingCmd, incoming, len);
      memcpy(last_master_mac, mac, 6);
      last_master_heartbeat = millis();
      targetSetpoint = incomingCmd.target_speed;
      if (incomingCmd.command == 1) { currentMode = REMOTE_LOCKED; system_on = true; }
      else if (currentMode == REMOTE_LOCKED) { currentMode = IDLE; }
    }
  }
}
#endif

void handleSerial() {
  if (Serial1.available() >= sizeof(stirrer_command_t)) {
    if (Serial1.peek() == 0xAA) {
      uint8_t buf[sizeof(stirrer_command_t)];
      Serial1.readBytes(buf, sizeof(stirrer_command_t));
      uint8_t calc = calc_crc8(buf, sizeof(stirrer_command_t) - 1);
      if (calc == buf[sizeof(stirrer_command_t) - 1]) {
        memcpy(&incomingCmd, buf, sizeof(stirrer_command_t));
        targetSetpoint = incomingCmd.target_speed;
        currentMode = SERIAL_CTL;
        system_on = true;
      }
    } else { Serial1.read(); }
  }
}

void sendTelemetry() {
  outgoingData.start_byte = 0xBB;
  outgoingData.len = sizeof(stirrer_telemetry_t);
  outgoingData.status = (uint8_t)currentMode;
  outgoingData.setpoint = (uint8_t)targetSetpoint;
  outgoingData.bar_rpm = (int16_t)actualBarRPM;
  outgoingData.fan_rpm = (int16_t)actualFanRPM;
  outgoingData.current_ma = (uint16_t)current_ma_val;
  outgoingData.timer_rem = (uint32_t)remainingSeconds;
  outgoingData.crc8 = calc_crc8((uint8_t*)&outgoingData, sizeof(stirrer_telemetry_t) - 1);

  if (last_master_mac[0] != 0) {
    esp_now_send(last_master_mac, (uint8_t *)&outgoingData, sizeof(outgoingData));
  }
  Serial1.write((uint8_t*)&outgoingData, sizeof(outgoingData));
}

// Function prototypes
void ControlTask(void *pvParameters);
void InterfaceTask(void *pvParameters);

void setup() {
  Serial.begin(115200);

  // Load settings
  prefs.begin("stirrer", false);
  targetSetpoint = prefs.getLong("last_speed", 0);
  timerMinutes = prefs.getLong("last_timer", 0);

  xTaskCreatePinnedToCore(ControlTask, "Control", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(InterfaceTask, "Interface", 8192, NULL, 2, NULL, 0);
}

void loop() { vTaskDelete(NULL); }

void ControlTask(void *pvParameters) {
  pinMode(KILL_SWITCH_PIN, OUTPUT);
  pinMode(FAN_PWM_PIN, OUTPUT);
  pinMode(FAN_TACHO_PIN, INPUT_PULLUP);
  pinMode(HALL_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FAN_TACHO_PIN), onFanPulse, FALLING);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN), onBarPulse, FALLING);

  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(FAN_PWM_PIN, 25000, 8);
  #else
    ledcSetup(0, 25000, 8);
    ledcAttachPin(FAN_PWM_PIN, 0);
  #endif

  unsigned long lastCalc = millis();
  unsigned long safetyTimer = 0;

  for (;;) {
    unsigned long now = millis();
    if (now - lastCalc >= PID_INTERVAL) {
      noInterrupts();
      int fCount = fanPulseCount; fanPulseCount = 0;
      int bCount = barPulseCount; barPulseCount = 0;
      interrupts();

      actualFanRPM = (fCount * 60 * (1000 / PID_INTERVAL)) / FAN_PPR;
      actualBarRPM = (bCount * 60 * (1000 / PID_INTERVAL)) / 1;

      if (system_on) {
        digitalWrite(KILL_SWITCH_PIN, HIGH);
        float targetRPM = targetSetpoint * 30;
        float error = targetRPM - actualBarRPM;
        if (actualBarRPM < 50 && actualFanRPM > 200) error = targetRPM - actualFanRPM;

        integral += error * (PID_INTERVAL / 1000.0);
        float derivative = (error - lastError) / (PID_INTERVAL / 1000.0);
        float output = (Kp * error) + (Ki * integral) + (Kd * derivative);
        float ff = (targetSetpoint / 100.0) * 255.0;
        current_pwm_val = constrain(ff + output, 0, 255);
        lastError = error;
      } else {
        current_pwm_val = 0; integral = 0;
        digitalWrite(KILL_SWITCH_PIN, LOW);
      }

      #if ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcWrite(FAN_PWM_PIN, (int)current_pwm_val);
      #else
        ledcWrite(0, (int)current_pwm_val);
      #endif

      // Safety Checks
      if (system_on && targetSetpoint > 25) {
        if (actualFanRPM < 100 || (actualBarRPM < 50 && actualFanRPM > 500)) {
          if (safetyTimer == 0) safetyTimer = now;
          if (now - safetyTimer > 4000) {
            currentMode = (actualFanRPM < 100) ? FAN_STALLED : DECOUPLED;
            system_on = false; targetSetpoint = 0;
          }
        } else safetyTimer = 0;
      }
      lastCalc = now;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void InterfaceTask(void *pvParameters) {
  Serial1.begin(115200, SERIAL_8N1, BIO_RX, BIO_TX);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() == ESP_OK) esp_now_register_recv_cb(OnDataRecv);

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(I2C_SPEED);
  if (display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) oled_present = true;
  if (ina219.begin()) ina219_present = true;

  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(LED_R, 5000, 8); ledcAttach(LED_G, 5000, 8); ledcAttach(LED_B, 5000, 8);
  #else
    ledcSetup(LED_CHAN_R, 5000, 8); ledcAttachPin(LED_R, LED_CHAN_R);
    ledcSetup(LED_CHAN_G, 5000, 8); ledcAttachPin(LED_G, LED_CHAN_G);
    ledcSetup(LED_CHAN_B, 5000, 8); ledcAttachPin(LED_B, LED_CHAN_B);
  #endif

  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), handleEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT), handleEncoder, CHANGE);

  unsigned long lastDisp = 0, lastTelem = 0, lastSecond = 0, lastPrefs = 0;
  unsigned long btnStart = 0;

  for (;;) {
    unsigned long now = millis();
    handleSerial();

    if (ina219_present) current_ma_val = ina219.getCurrent_mA();

    // Heartbeat safety
    if (currentMode == REMOTE_LOCKED && now - last_master_heartbeat > HEARTBEAT_TIMEOUT) {
      currentMode = IDLE; system_on = false; targetSetpoint = 0;
    }

    // Timer logic
    if (system_on && isTimerActive) {
      if (now - lastSecond >= 1000) {
        if (remainingSeconds > 0) remainingSeconds--;
        else { system_on = false; currentMode = TIMER_DONE; isTimerActive = false; }
        lastSecond = now;
      }
    }

    // Persistence
    if (now - lastPrefs >= 30000) {
      prefs.putLong("last_speed", targetSetpoint);
      prefs.putLong("last_timer", timerMinutes);
      lastPrefs = now;
    }

    // Non-blocking button
    bool btn = digitalRead(ENCODER_SW);
    if (btn == LOW) {
      if (btnStart == 0) btnStart = now;
    } else {
      if (btnStart > 0) {
        unsigned long dur = now - btnStart;
        if (dur > 50 && dur < 600) editTimerMode = !editTimerMode;
        else if (dur >= 600) {
          system_on = !system_on;
          currentMode = system_on ? LOCAL : STOPPED;
          if (system_on && timerMinutes > 0) { remainingSeconds = timerMinutes * 60; isTimerActive = true; }
        }
        btnStart = 0;
      }
    }

    if (oled_present && now - lastDisp >= DISPLAY_INTERVAL) {
      display.clearDisplay();
      display.setCursor(0,0); display.print(getStatusString(currentMode));
      if (currentMode == REMOTE_LOCKED) display.print(" [LOCKED]");
      display.drawLine(0, 10, 127, 10, 1);
      display.setCursor(0,15); display.print("Set: "); display.print(targetSetpoint); display.print("%");
      if (editTimerMode) display.print(" <T>");
      display.setTextSize(2); display.setCursor(0,30); display.print(actualBarRPM); display.print(" RPM");
      display.setTextSize(1); display.setCursor(0,55);
      if (isTimerActive) {
        display.print("Rem: "); display.print(remainingSeconds/60); display.print("m");
      } else {
        display.print("Fan: "); display.print(actualFanRPM);
        if (ina219_present) { display.print(" "); display.print((int)current_ma_val); display.print("mA"); }
      }
      display.display();
      lastDisp = now;
    }

    if (now - lastTelem >= TELEMETRY_INTERVAL) {
      sendTelemetry(); lastTelem = now;
    }

    updateVisuals();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
