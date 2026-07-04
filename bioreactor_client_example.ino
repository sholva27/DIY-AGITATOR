#include <esp_now.h>
#include <WiFi.h>

#define STRUCT_VERSION 0x02

typedef struct {
  uint8_t start_byte;
  uint8_t version;
  uint8_t target_speed;
  uint8_t command;
  uint8_t crc8;
} stirrer_command_t;

typedef struct {
  uint8_t start_byte;
  uint8_t version;
  uint8_t mode;
  uint8_t setpoint;
  int16_t bar_rpm;
  int16_t fan_rpm;
  uint16_t current_ma;
  uint32_t timer_rem;
  uint8_t crc8;
} stirrer_telemetry_t;

uint8_t stirrer_mac[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66}; // Change to your stirrer MAC

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

void OnDataRecv(const esp_now_recv_info_t * recv_info, const uint8_t *incoming, int len) {
  if (len == sizeof(stirrer_telemetry_t) && incoming[0] == 0xBB) {
    stirrer_telemetry_t telem;
    memcpy(&telem, incoming, sizeof(telem));
    if (telem.version == STRUCT_VERSION) {
       Serial.printf("Stirrer RPM: %d, Fan RPM: %d, Mode: %d\n", telem.bar_rpm, telem.fan_rpm, telem.mode);
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) return;

  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, stirrer_mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 5000) {
    stirrer_command_t cmd;
    cmd.start_byte = 0xAA;
    cmd.version = STRUCT_VERSION;
    cmd.target_speed = 50; // 50% speed
    cmd.command = 1;       // LOCK mode
    cmd.crc8 = calc_crc8((uint8_t*)&cmd, sizeof(cmd)-1);

    esp_now_send(stirrer_mac, (uint8_t*)&cmd, sizeof(cmd));
    lastSend = millis();
  }
}
