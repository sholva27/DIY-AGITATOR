#include <esp_now.h>
#include <WiFi.h>

// Replace with the MAC Address of your Stirrer ESP32 (printed on OLED at boot)
uint8_t stirrerAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct {
  int target_speed;         // 0-100%
  bool remote_lock;         // If true, ignore local encoder rotation
} stirrer_command_t;

typedef struct {
  int actual_rpm;
  int current_pwm;          // 0-255
  uint8_t status;           // 0:IDLE, 1:LOCAL, 2:REMOTE, 3:SERIAL, 4:STOPPED, 5:DECOUPLED, 6:TIMER_DONE
} stirrer_telemetry_t;

stirrer_command_t myCmd;
stirrer_telemetry_t stirrerStats;

// Callback when data is received from the Stirrer
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t * recv_info, const uint8_t *incoming, int len) {
#else
void OnDataRecv(const uint8_t * mac, const uint8_t *incoming, int len) {
#endif
  if (len == sizeof(stirrer_telemetry_t)) {
    memcpy(&stirrerStats, incoming, len);
    Serial.print("Stirrer RPM: "); Serial.println(stirrerStats.actual_rpm);
    Serial.print("Stirrer Status: "); Serial.println(stirrerStats.status);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, stirrerAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  // Example: Send 50% speed with remote lock enabled
  myCmd.target_speed = 50;
  myCmd.remote_lock = true;

  esp_now_send(stirrerAddress, (uint8_t *) &myCmd, sizeof(myCmd));

  delay(5000);
}
