#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "SSD1306Wire.h" // install library "ESP8266 and ESP32 OLED driver for..." version should be 4.4.1
#include <ArduinoJson.h> // install library "ArduinoJson" by Benoit 

// Configure the name and password of the connected wifi, and Server host !!!!!!!!!!!!!!!!!!!!!!
const char* ssid        = "name"; // your hotspot name
const char* password    = "password"; // your hotspot password
const String server = "http://192.168.141.49:5000/test"; // your computer ip address, ensure both are connected to hotspot

String ipaddr = "";

// elderly m5
String m5_hardware_id = ""; // location uses this
String elderly = "";
String geofenced_area = "";

// area-coordinates
// String name = "";
// String start_x = "";
// String end_x = "";
// String start_y = "";
// String end_y = "";
// String geofenced_area = "";

// location
// String m5_hardware_id = ""; // already declared
// String x = "";
// String y = "";
// String floor = "";
// String timestamp = "";

SSD1306Wire display(0x3c, 18, 17);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(100);
  display.init();
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
 
  display.drawString(0, 0, "connecting to wifi");
  display.display();
  delay(2000);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Connected to WiFi
  ipaddr = WiFi.localIP().toString();
  display.clear();
  display.drawString(0, 0,"wifi connected");
  display.drawString(0, 10, "IP address:");
  display.drawString(60, 10, ipaddr);
  display.display();

  
}

void loop() {
  // GET data from other lilygo

  // elderly
  m5_hardware_id = "testing_id_to_db";
  elderly = "colin";
  geofenced_area = "Flat C";

  // area-coordinates
  // name = "" ;
  // start_x = "";
  // end_x = "";
  // start_y = "";
  // end_y = "";
  // geofenced_area = "";
  
  // location
  // x = "";
  // y = "";
  // floor = "";
  // timestamp = "";


  // put your main code here, to run repeatedly:
  if(WiFi.status() == WL_CONNECTED){

    HTTPClient http;

    //set endpoint and content type
    http.begin(server);
    http.addHeader("Content-Type", "application/json");

    // Create JSON object and add data
    StaticJsonDocument<200> jsonDoc;
    
    // elderly
    jsonDoc["m5_hardware_id"] = m5_hardware_id;
    jsonDoc["elderly"] = elderly;
    jsonDoc["geofenced_area"] = geofenced_area;

    // area-coordinates
    // jsonDoc["name"] = name;
    // jsonDoc["start_x"] = start_x;
    // jsonDoc["end_x"] = end_x;
    // jsonDoc["start_y"] = start_y;
    // jsonDoc["end_y"] = end_y;
    // jsonDoc["geofenced_area"] = geofenced_area;

    // location
    // jsonDoc["m5_hardware_id"] = m5_hardware_id;
    // jsonDoc["x"] = x;
    // jsonDoc["y"] = y;
    // jsonDoc["floor"] = floor;
    // jsonDoc["timestamp"] = timestamp;

    // Serialize JSON object to string
    String jsonString;
    serializeJson(jsonDoc, jsonString);

    // Create JSON data and POST
    int httpCode = http.POST(jsonString);
    
    // check if there is error doing POST request from device
    if (httpCode>0) {
      // Response from server after successful POST
      String payload = http.getString();
      display.clear();
      display.drawString(0, 0,"sending POST request");
      display.drawString(0, 10, payload);
      display.display();
    }
    else {
      display.clear();
      display.drawString(0, 0,"Error when sending");
      display.display();
    }

    http.end();

    delay(3000);
  }

}

