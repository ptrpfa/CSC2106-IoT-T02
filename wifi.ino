#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "SSD1306Wire.h" // install library "ESP8266 and ESP32 OLED driver for..." version should be 4.4.1
#include <ArduinoJson.h> // install library "ArduinoJson" by Benoit 

// Configure the name and password of the connected wifi, and Server host !!!!!!!!!!!!!!!!!!!!!!
const char* ssid        = "name"; // your hotspot name
const char* password    = "password"; // your hotspot password
const String server = "http://192.168.75.49:5000/"; // your computer ip address, ensure both are connected to hotspot
String ipaddr = "";
String third = "";
String second = "";
String first = "";

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
  third = "testing3";
  second = "testing2";
  first = "testing1";

  // put your main code here, to run repeatedly:
  if(WiFi.status() == WL_CONNECTED){

    HTTPClient http;

    //set endpoint and content type
    http.begin(server);
    http.addHeader("Content-Type", "application/json");

    // Create JSON object and add data
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["ipaddr"] = ipaddr;
    jsonDoc["third"] = third;
    jsonDoc["second"] = second;
    jsonDoc["first"] = first;

    // Serialize JSON object to string
    String jsonString;
    serializeJson(jsonDoc, jsonString);

    // Create JSON data and POST
    // String requestData = "ipaddr=" + String(ipaddr);
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

