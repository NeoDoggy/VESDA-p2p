#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define rx2 16
#define tx2 17

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, rx2, tx2);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Waiting for incomings");
  display.println("---------------------");
  display.display(); 
}

String message;
int counter=0;

void loop() {
  if (Serial2.available()) {
    counter++;
    message = Serial2.readStringUntil('\r');
    Serial.println(message);
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Recieved: "+String(counter));
    display.println("---------------------");
    display.println(message);
    display.display();
  }
}