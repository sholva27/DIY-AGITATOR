#include <esp_now.h>
#include <WiFi.h>

// Replace with the MAC Address of your Stirrer ESP32 (printed on OLED at boot)
uint8_t stirrerAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* --- DATA STRUCTURES (Synchronized with stirrer_project.ino) --- */
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

stirrer_command_t myCmd = {0xAA, sizeof(stirrer_command_t), 0, 0, 0};
stirrer_telemetry_t stirrerStats;

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

// Callback when telemetry is received from the Stirrer
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t * recv_info, const uint8_t *incoming, int len) {
#else
void OnDataRecv(const uint8_t * mac, const uint8_t *incoming, int len) {
#endif
  if (len == sizeof(stirrer_telemetry_t) && incoming[0] == 0xBB) {
    uint8_t calc = calc_crc8(incoming, len - 1);
    if (calc == incoming[len-1]) {
      memcpy(&stirrerStats, incoming, len);
      Serial.print("Stirrer RPM: "); Serial.println(stirrerStats.bar_rpm);
      Serial.print("Stirrer Status: "); Serial.println(stirrerStats.status);
    }
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
  // Example: Send 50% speed with remote lock enabled every 10 seconds
  myCmd.target_speed = 50;
  myCmd.command = 1; // Acquire Lock
  myCmd.crc8 = calc_crc8((uint8_t*)&myCmd, sizeof(stirrer_command_t) - 1);

  esp_now_send(stirrerAddress, (uint8_t *) &myCmd, sizeof(myCmd));

  delay(10000);
}
