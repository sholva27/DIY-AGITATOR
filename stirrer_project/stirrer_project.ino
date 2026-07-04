#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_now.h>
#include <WiFi.h>

/* --- CONFIGURATION --- */
#define FAN_PPR 2           // Pulses Per Revolution of the fan
#define SCREEN_ADDRESS 0x3C
#define I2C_SPEED 400000
#define TELEMETRY_INTERVAL 1000
#define DISPLAY_INTERVAL 200
#define PID_INTERVAL 100

/* --- PIN DEFINITIONS --- */
#define FAN_PWM_PIN 14
#define FAN_TACHO_PIN 13
#define BUZZER_PIN 15
#define LED_R 16
#define LED_G 17
#define LED_B 18
#define ENCODER_CLK 10
#define ENCODER_DT  11
#define ENCODER_SW  12
#define BIO_RX 4
#define BIO_TX 5

/* --- PWM CHANNELS (Legacy API) --- */
#define FAN_PWM_CHAN 0
#define LED_CHAN_R 1
#define LED_CHAN_G 2
#define LED_CHAN_B 3

/* --- DATA STRUCTURES (Synchronized with Bioreactor integration.md) --- */
typedef struct {
  int target_speed;         // 0-100%
  bool remote_lock;         // If true, ignore local encoder rotation
} stirrer_command_t;

typedef struct {
  int actual_rpm;
  int current_pwm;          // 0-255
  uint8_t status;           // ControlMode enum
} stirrer_telemetry_t;

stirrer_command_t incomingCmd = {0, false};
stirrer_telemetry_t outgoingData;

/* --- STATE MACHINE --- */
enum ControlMode { IDLE, LOCAL, REMOTE, SERIAL_CTL, STOPPED, DECOUPLED, TIMER_DONE };
volatile ControlMode currentMode = IDLE;
volatile bool system_on = false;

/* --- GLOBAL VARIABLES --- */
volatile long encoderValue = 0;
volatile long timerMinutes = 0;
volatile int pulseCount = 0;
int actualRPM = 0;
int targetRPM = 0;
float current_pwm_output = 0;
unsigned long remainingSeconds = 0;
bool isTimerActive = false;
bool editTimerMode = false;
bool oled_enabled = false;
uint8_t master_mac[6] = {0};

// Timers
unsigned long lastRPMCalcTime = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastTelemetryTime = 0;
unsigned long lastPIDTime = 0;
unsigned long lastSecondUpdate = 0;
unsigned long decouplingTimer = 0;

Adafruit_SSD1306 display(128, 64, &Wire, -1);

/* --- PID CONSTANTS --- */
float Kp = 0.5, Ki = 0.1, Kd = 0.05;
float integral = 0, lastError = 0;

/* --- HELPERS --- */
String getStatusString(ControlMode mode) {
  switch(mode) {
    case IDLE: return "IDLE";
    case LOCAL: return "LOCAL";
    case REMOTE: return "REMOTE";
    case SERIAL_CTL: return "SERIAL";
    case STOPPED: return "STOPPED";
    case DECOUPLED: return "DECOUPLED";
    case TIMER_DONE: return "DONE!";
    default: return "UNKNOWN";
  }
}

void IRAM_ATTR handleTachoPulse() { pulseCount++; }

void IRAM_ATTR handleEncoder() {
  static int lastEncoded = 0;
  int MSB = digitalRead(ENCODER_CLK);
  int LSB = digitalRead(ENCODER_DT);
  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;

  if (!incomingCmd.remote_lock) {
    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
      if (editTimerMode) timerMinutes++;
      else encoderValue++;
      currentMode = LOCAL;
    }
    if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
      if (editTimerMode) { if (timerMinutes > 0) timerMinutes--; }
      else encoderValue--;
      currentMode = LOCAL;
    }
    if (encoderValue > 100) encoderValue = 100;
    if (encoderValue < 0) encoderValue = 0;
    if (timerMinutes > 999) timerMinutes = 999;
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
  if (currentMode < STOPPED) {
    int b = (sin(now * 0.003) + 1) * 127;
    setRGB(0, b, 0);
  } else if (currentMode == STOPPED) {
    setRGB(255, 0, 0);
  } else {
    if ((now / 200) % 2 == 0) setRGB(255, 0, 0); else setRGB(0, 0, 0);
  }

  static unsigned long buzzerStart = 0;
  if (currentMode >= DECOUPLED && buzzerStart == 0) { buzzerStart = now; }
  if (buzzerStart > 0) {
    if (now - buzzerStart < 1000) {
       if ((now / 200) % 2 == 0) digitalWrite(BUZZER_PIN, HIGH);
       else digitalWrite(BUZZER_PIN, LOW);
    } else {
       digitalWrite(BUZZER_PIN, LOW);
       if (currentMode < DECOUPLED) buzzerStart = 0;
    }
  }
}

/* --- COMMUNICATION --- */
void sendTelemetry() {
  outgoingData.actual_rpm = actualRPM;
  outgoingData.current_pwm = (int)current_pwm_output;
  outgoingData.status = (uint8_t)currentMode;

  Serial1.print("RPM:"); Serial1.print(actualRPM);
  Serial1.print(",PWM:"); Serial1.print(outgoingData.current_pwm);
  Serial1.print(",STAT:"); Serial1.println(getStatusString(currentMode));

  if (master_mac[0] != 0) {
    esp_now_send(master_mac, (uint8_t *) &outgoingData, sizeof(outgoingData));
  }
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t * recv_info, const uint8_t *incoming, int len) {
  if (len == sizeof(stirrer_command_t)) {
    memcpy(&incomingCmd, incoming, len);
    memcpy(master_mac, recv_info->src_addr, 6);
    encoderValue = incomingCmd.target_speed;
    currentMode = REMOTE;
  }
}
#else
void OnDataRecv(const uint8_t * mac, const uint8_t *incoming, int len) {
  if (len == sizeof(stirrer_command_t)) {
    memcpy(&incomingCmd, incoming, len);
    memcpy(master_mac, mac, 6);
    encoderValue = incomingCmd.target_speed;
    currentMode = REMOTE;
  }
}
#endif

void setupESPNOW() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
}

void handleSerial() {
  if (Serial1.available() > 0) {
    String input = Serial1.readStringUntil('\n');
    int val = input.toInt();
    if (val >= 0 && val <= 100) {
      encoderValue = val;
      currentMode = SERIAL_CTL;
    }
  }
}

/* --- CONTROL LOGIC --- */
void updatePID() {
  if (!system_on) {
    current_pwm_output = 0;
    integral = 0;
  } else {
    targetRPM = encoderValue * 30;
    float error = targetRPM - actualRPM;
    integral += error * (PID_INTERVAL / 1000.0);
    float derivative = (error - lastError) / (PID_INTERVAL / 1000.0);
    float output = (Kp * error) + (Ki * integral) + (Kd * derivative);
    float feedforward = (encoderValue / 100.0) * 255.0;
    current_pwm_output = constrain(feedforward + output, 0, 255);
    lastError = error;
  }

  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(FAN_PWM_PIN, (int)current_pwm_output);
  #else
    ledcWrite(FAN_PWM_CHAN, (int)current_pwm_output);
  #endif
}


void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, BIO_RX, BIO_TX);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(FAN_TACHO_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FAN_TACHO_PIN), handleTachoPulse, FALLING);

  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), handleEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT), handleEncoder, CHANGE);

  WiFi.mode(WIFI_STA);
  setupESPNOW();

  Wire.begin(8, 9);
  Wire.setClock(I2C_SPEED);
  if(display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    oled_enabled = true;
    display.clearDisplay();
    display.setTextColor(1);
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("BIO-STIRRER S3");
    display.print("MAC: "); display.println(WiFi.macAddress());
    display.display();
    delay(2000);
  }

  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(FAN_PWM_PIN, 25000, 8);
    ledcAttach(LED_R, 5000, 8); ledcAttach(LED_G, 5000, 8); ledcAttach(LED_B, 5000, 8);
  #else
    ledcSetup(FAN_PWM_CHAN, 25000, 8); ledcAttachPin(FAN_PWM_PIN, FAN_PWM_CHAN);
    ledcSetup(LED_CHAN_R, 5000, 8); ledcAttachPin(LED_R, LED_CHAN_R);
    ledcSetup(LED_CHAN_G, 5000, 8); ledcAttachPin(LED_G, LED_CHAN_G);
    ledcSetup(LED_CHAN_B, 5000, 8); ledcAttachPin(LED_B, LED_CHAN_B);
  #endif
}

void loop() {
  unsigned long now = millis();
  handleSerial();

  if (now - lastRPMCalcTime >= 1000) {
    noInterrupts();
    int p = pulseCount; pulseCount = 0;
    interrupts();
    actualRPM = (p * 60) / FAN_PPR;
    lastRPMCalcTime = now;
  }

  if (now - lastPIDTime >= PID_INTERVAL) {
    updatePID();
    lastPIDTime = now;
  }

  // checkSafety logic using 'now'
  if (system_on && encoderValue > 25) {
    if (actualRPM < 100) {
      if (decouplingTimer == 0) decouplingTimer = now;
      if (now - decouplingTimer > 4000) {
        currentMode = DECOUPLED; system_on = false; encoderValue = 0;
      }
    } else decouplingTimer = 0;
  }

  if (isTimerActive && system_on) {
    if (now - lastSecondUpdate >= 1000) {
      if (remainingSeconds > 0) remainingSeconds--;
      else {
        isTimerActive = false; system_on = false; encoderValue = 0;
        currentMode = TIMER_DONE;
      }
      lastSecondUpdate = now;
    }
  }

  if (digitalRead(ENCODER_SW) == LOW) {
    unsigned long start = millis();
    while(digitalRead(ENCODER_SW) == LOW);
    if (millis() - start < 500) editTimerMode = !editTimerMode;
    else {
      system_on = !system_on;
      if (system_on) {
        if (timerMinutes > 0) { remainingSeconds = timerMinutes * 60; isTimerActive = true; }
        if (encoderValue == 0) encoderValue = 30;
      } else {
        isTimerActive = false; currentMode = STOPPED;
      }
    }
  }

  if (now - lastTelemetryTime >= TELEMETRY_INTERVAL) {
    sendTelemetry();
    lastTelemetryTime = now;
  }

  if (oled_enabled && (now - lastDisplayUpdate >= DISPLAY_INTERVAL)) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print(getStatusString(currentMode));
    if (incomingCmd.remote_lock) display.print(" [LOCKED]");
    display.drawLine(0, 10, 127, 10, 1);
    display.setCursor(0, 15); display.print("Set: "); display.print(encoderValue); display.print("%");
    if (editTimerMode) display.print(" <T>");
    display.setTextSize(2);
    display.setCursor(0, 30); display.print(actualRPM); display.print(" RPM");
    display.setTextSize(1);
    display.setCursor(0, 55);
    if (isTimerActive) {
      display.print("Time: "); display.print(remainingSeconds/60); display.print(":");
      if(remainingSeconds%60 < 10) display.print("0"); display.print(remainingSeconds%60);
    } else {
      display.print("Timer: "); display.print(timerMinutes); display.print("m");
    }
    display.display();
    lastDisplayUpdate = now;
  }

  updateVisuals();
}
