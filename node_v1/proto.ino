#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <GP2YDustSensor.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "conf.secret.h"


#define SHARP_LED_PIN 6
#define SHARP_VO_PIN 7
#define SDA0_Pin 16
#define SCL0_Pin 15
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define BUTTON_PIN 18


uint8_t broadcastAddress[] = {0xC0, 0x49, 0xef, 0xd0, 0xd6, 0xb8}; //c0:49:ef:d0:d6:b8
typedef struct p2pMsgStruct {
    char nodeMac[18];
    int curD;
    int avgD;
} p2pMsgStruct;


p2pMsgStruct sendData;
TaskHandle_t serverTaskHandler;

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

GP2YDustSensor dustSensor(GP2YDustSensorType::GP2Y1014AU0F, SHARP_LED_PIN, SHARP_VO_PIN);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WebServer server(80);
 
extern String css;
extern String loginIndex;
extern String serverIndex;

bool hasOTA=false;

uint8_t baseMac[6];
char esp32Mac[18];

void readMacAddress(){
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  snprintf(esp32Mac,18,"%02x:%02x:%02x:%02x:%02x:%02x\n",
                      baseMac[0], baseMac[1], baseMac[2],
                      baseMac[3], baseMac[4], baseMac[5]);
}
 
void setup(void) {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Wire.begin(SDA0_Pin, SCL0_Pin);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.setRotation(2);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  // Display static text
  display.println("Starting UP");
  display.println("Hold button to enter OTA");
  display.display(); 

  delay(3000);

  if(digitalRead(BUTTON_PIN)==LOW){ // ota mode
    hasOTA=true;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Entered OTA mode");
    display.println("Waiting for connection to:");
    display.println("SSID: "+String(ssid));
    display.display(); 
    // Connect to WiFi network
    WiFi.begin(ssid, password);
    Serial.println("");
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("IP address: ");
    display.println(WiFi.localIP());
    display.display(); 

    if (!MDNS.begin(host)) { 
      Serial.println("Error setting up MDNS responder!");
      while (1) {
        delay(1000);
      }
    }
    Serial.println("mDNS responder started");
    /*return index page which is stored in serverIndex */
    server.on("/", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", loginIndex);
    });
    server.on("/serverIndex", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", serverIndex);
    });
    /*handling uploading firmware file */
    server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        /* flashing firmware to ESP*/
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
      }
    });
    server.begin();
  }
  else{ // p2p
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }
    esp_now_register_send_cb(OnDataSent);
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      Serial.println("Failed to add peer");
      return;
    }
  }

  readMacAddress();

  dustSensor.setBaseline(0); // set no dust voltage according to your own experiments
  dustSensor.setCalibrationFactor(1.1); // calibrate against precision instrument
  dustSensor.begin();

  xTaskCreatePinnedToCore(
                    serverHandleLoop,       /* Task function. */
                    "serverHandleLoop",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &serverTaskHandler,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */


}

// 1KHZ loop
void serverHandleLoop(void * pvParameters ){while(1){
  if(hasOTA){
    server.handleClient();
  }

delay(1);}}
 
void loop(void) {
  int dd=dustSensor.getDustDensity();
  int ddavg=dustSensor.getRunningAverage();
  Serial.print("Dust density: ");
  Serial.print(dd);
  Serial.print(" ug/m3; Running average: ");
  Serial.print(ddavg);
  Serial.println(" ug/m3");
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  if(hasOTA){
    display.println("IP address: ");
    display.println(WiFi.localIP());
    display.println("---------------------");
  }
  display.println("Density: "+String(dd)+" ug/m3");
  display.println("Running AVG: "+String(ddavg)+" ug/m3");
  display.println("---------------------");
  display.println("MAC:");
  display.println(esp32Mac);
  display.display();
  if(!hasOTA){
    strcpy(sendData.nodeMac, esp32Mac);
    sendData.curD=dd;
    sendData.avgD=ddavg;
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendData, sizeof(sendData));
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
  }
  delay(1000);
}