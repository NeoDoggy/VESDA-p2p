#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <GP2YDustSensor.h>

TaskHandle_t Task2;

const uint8_t SHARP_LED_PIN = 6;   // Sharp Dust/particle sensor Led Pin
const uint8_t SHARP_VO_PIN = 7;    // Sharp Dust/particle analog out pin used for reading 

GP2YDustSensor dustSensor(GP2YDustSensorType::GP2Y1014AU0F, SHARP_LED_PIN, SHARP_VO_PIN);
 
char* host = "esp32";
char* ssid = "AA";
char* password = "imdiethankyouforever";
WebServer server(80);
 
String css =
"<style>#file-input,input{width:100%;height:100px;border-radius:5px;margin:10px auto;font-size:16px}"
"input{background:#fff;border:0;padding:0 15px}body{background:#5398D9;font-family:sans-serif;font-size:14px;color:#4d4d4d}"
"#file-input{padding:0;border:2px solid #ddd;line-height:45px;text-align:left;display:block;cursor:pointer}"
"#bar,#prgbar{background-color:#4d4d4d;border-radius:15px}#bar{background-color:#5398D9;width:0%;height:15px}"
"form{background:#EDF259;max-width:300px;margin:80px auto;padding:30px;border-radius:5px;text-align:center}"
".btn{background:#5398D9;color:#EDF259;cursor:pointer}</style>";
 
String loginIndex =
"<form name='loginForm'>"
"<h2>Chosemaker ESP32 Login</h2>"
"<input name=userid placeholder='Username'> "
"<input name=pwd placeholder='Password' type=Password> "
"<input type=submit onclick=check(this.form) class=btn value=Login></form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
"</script>" + css;
 
String serverIndex =
 "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
 "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
 "<input type='file' name='update'>"
 "<input type='submit' value='Update'>"
 "<div id='prg'>0%</div>"
 "<div id='prgbar'><div id='bar'></div></div></form>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('Running ' + Math.round(per*100) + '%');"
  "$('#bar').css('width',Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>" + css;
 
void setup(void) {
  Serial.begin(115200);
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

  dustSensor.setBaseline(0); // set no dust voltage according to your own experiments
  dustSensor.setCalibrationFactor(1.1); // calibrate against precision instrument
  dustSensor.begin();

  xTaskCreatePinnedToCore(
                    loop1,       /* Task function. */
                    "loop1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */
}

void loop1(void * pvParameters ){
  while(1){
    Serial.print("Dust density: ");
    Serial.print(dustSensor.getDustDensity());
    Serial.print(" ug/m3; Running average: ");
    Serial.print(dustSensor.getRunningAverage());
    Serial.println(" ug/m3");
    delay(1000);
  }
}
 
void loop(void) {
  server.handleClient();
  delay(1);
}