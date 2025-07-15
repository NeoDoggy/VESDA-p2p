#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>

#define rx2 16
#define tx2 17

typedef struct p2pMsgStruct {
    char nodeMac[18];
    int curD;
    int avgD;
} p2pMsgStruct;

p2pMsgStruct recvData;
char esp32Mac[18];

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&recvData, incomingData, sizeof(recvData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial2.print("From node:\n"+String(recvData.nodeMac)+"\nDD: "+String(recvData.curD)+"\nAVG: "+String(recvData.avgD)+"\n\r");
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, rx2, tx2);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

String message;

void loop() {
}