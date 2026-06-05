#include <esp_now.h>
#include <WiFi.h>

// Replace with the MAC Address of your Stirrer ESP32
uint8_t stirrerAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct struct_message {
  int speed;
} struct_message;

struct_message myData;

esp_now_peer_info_t peerInfo;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  memcpy(peerInfo.peer_addr, stirrerAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  // Example: Send 50% speed every 5 seconds
  myData.speed = 50;
  esp_now_send(stirrerAddress, (uint8_t *) &myData, sizeof(myData));
  delay(5000);
}
