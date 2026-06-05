#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_now.h>
#include <WiFi.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin Definitions
#define FAN_PWM_PIN 14
#define FAN_TACHO_PIN 13

// Rotary Encoder Pins
#define ENCODER_CLK 10
#define ENCODER_DT  11
#define ENCODER_SW  12

// UART for Bioreactor
#define BIO_RX 44
#define BIO_TX 43

// PWM Settings
#define PWM_FREQ 25000
#define PWM_RES 8
#define PWM_CHAN 0

typedef struct struct_message {
  int speed;
} struct_message;

struct_message incomingData;

// Global Variables
volatile int targetSpeedPercent = 0;
float currentRampSpeed = 0;
volatile int lastEncoded = 0;
volatile long encoderValue = 0;
volatile int pulseCount = 0;
int actualRPM = 0;
int lastRPM = 0;
unsigned long startTime = 0;
unsigned long lastRPMCalcTime = 0;
unsigned long decouplingTimer = 0;
bool isStirring = false;
const float RAMP_STEP = 0.5; // Speed of ramping

enum ControlMode { IDLE, LOCAL, REMOTE, SERIAL_CTL, STOPPED, DECOUPLED };
volatile ControlMode currentMode = IDLE;

// Helper to get status string safely
String getStatusString(ControlMode mode) {
  switch(mode) {
    case IDLE: return "IDLE";
    case LOCAL: return "LOCAL";
    case REMOTE: return "REMOTE";
    case SERIAL_CTL: return "SERIAL";
    case STOPPED: return "STOPPED";
    case DECOUPLED: return "DECOUPLED";
    default: return "UNKNOWN";
  }
}

// Interrupt Service Routine for Tachometer
void IRAM_ATTR handleTachoPulse() {
  pulseCount++;
}

void IRAM_ATTR handleEncoder() {
  int MSB = digitalRead(ENCODER_CLK);
  int LSB = digitalRead(ENCODER_DT);

  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
    encoderValue++;
    currentMode = LOCAL;
  }
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
    encoderValue--;
    currentMode = LOCAL;
  }

  lastEncoded = encoded;

  if (encoderValue > 100) encoderValue = 100;
  if (encoderValue < 0) encoderValue = 0;
}

void setupFan() {
  // PWM Setup (Compatible with ESP32 Arduino 3.0+)
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(FAN_PWM_PIN, PWM_FREQ, PWM_RES);
  #else
    ledcSetup(PWM_CHAN, PWM_FREQ, PWM_RES);
    ledcAttachPin(FAN_PWM_PIN, PWM_CHAN);
  #endif

  // Tachometer Setup
  pinMode(FAN_TACHO_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FAN_TACHO_PIN), handleTachoPulse, FALLING);
}

void setupEncoder() {
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), handleEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT), handleEncoder, CHANGE);
}

// New ESP-NOW callback signature for Arduino 3.0+
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t * recv_info, const uint8_t *incoming, int len) {
#else
void OnDataRecv(const uint8_t * mac, const uint8_t *incoming, int len) {
#endif
  memcpy(&incomingData, incoming, sizeof(incomingData));
  encoderValue = incomingData.speed;
  currentMode = REMOTE;
}

void setupESPNOW() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
}

void handleSerial() {
  if (Serial1.available() > 0) {
    String input = Serial1.readStringUntil('\n');
    int speed = input.toInt();
    if (speed >= 0 && speed <= 100) {
      encoderValue = speed;
      currentMode = SERIAL_CTL;
    }
  }
}

void setFanSpeed(int percent) {
  targetSpeedPercent = constrain(percent, 0, 100);
  int dutyCycle = (targetSpeedPercent * 255) / 100;

  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(FAN_PWM_PIN, dutyCycle);
  #else
    ledcWrite(PWM_CHAN, dutyCycle);
  #endif

  if (targetSpeedPercent > 0) {
    if (!isStirring) {
      isStirring = true;
      startTime = millis();
    }
  } else {
    isStirring = false;
    if (currentMode != STOPPED && currentMode != DECOUPLED) currentMode = IDLE;
  }
}

void checkDecoupling() {
  if (isStirring && targetSpeedPercent > 20) {
    // If RPM is 0 while target is high (Fan stalled or Tacho failed)
    // Or if RPM drops significantly suddenly
    if (actualRPM < 100 && targetSpeedPercent > 30) {
      if (decouplingTimer == 0) decouplingTimer = millis();
      if (millis() - decouplingTimer > 3000) {
        currentMode = DECOUPLED;
        encoderValue = 0;
        currentRampSpeed = 0;
        setFanSpeed(0);
        decouplingTimer = 0;
      }
    } else {
      decouplingTimer = 0;
    }
  }
}

void calculateRPM() {
  unsigned long currentTime = millis();
  unsigned long timeDiff = currentTime - lastRPMCalcTime;

  if (timeDiff >= 1000) {
    noInterrupts();
    int currentPulseCount = pulseCount;
    pulseCount = 0;
    interrupts();

    actualRPM = (currentPulseCount * 60) / 2;
    lastRPMCalcTime = currentTime;
  }
}

void setupOLED() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Stirrer Initializing...");
  display.print("MAC: ");
  display.println(WiFi.macAddress());
  display.display();
  delay(3000);
}

void updateDisplay() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("STATUS: ");
  display.println(getStatusString(currentMode));

  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setCursor(0, 15);
  display.print("Target: ");
  display.setTextSize(2);
  display.print(targetSpeedPercent);
  display.print("%");

  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print("RPM: ");
  display.setTextSize(2);
  display.print(actualRPM);

  display.setTextSize(1);
  display.setCursor(0, 55);
  unsigned long elapsed = 0;
  if (isStirring) {
    elapsed = (millis() - startTime) / 1000;
  }
  display.print("Time: ");
  display.print(elapsed / 60);
  display.print("m ");
  display.print(elapsed % 60);
  display.print("s");

  display.display();
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, BIO_RX, BIO_TX);
  setupOLED();
  setupFan();
  setupEncoder();
  setupESPNOW();
}

void loop() {
  handleSerial();

  // Soft Start / Ramping Logic
  if (currentRampSpeed < (float)encoderValue) {
    currentRampSpeed += RAMP_STEP;
    if (currentRampSpeed > (float)encoderValue) currentRampSpeed = (float)encoderValue;
    setFanSpeed((int)currentRampSpeed);
  } else if (currentRampSpeed > (float)encoderValue) {
    currentRampSpeed -= RAMP_STEP * 2;
    if (currentRampSpeed < (float)encoderValue) currentRampSpeed = (float)encoderValue;
    setFanSpeed((int)currentRampSpeed);
  }

  if (digitalRead(ENCODER_SW) == LOW) {
    delay(50);
    if (digitalRead(ENCODER_SW) == LOW) {
      if (isStirring) {
        encoderValue = 0;
        currentRampSpeed = 0;
        currentMode = STOPPED;
        setFanSpeed(0);
      }
      while(digitalRead(ENCODER_SW) == LOW);
    }
  }

  calculateRPM();
  checkDecoupling();
  updateDisplay();
  delay(100);
}
