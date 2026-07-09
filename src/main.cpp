// #include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>

#include "network/WebSocketHandler.h"

const char* ssid = "LyingCards";
const char* password = "12345678";

AsyncWebServer server(80);
// AsyncWebSocket ws("/ws");

void setup() {
  Serial.begin(115200);
  
  // Initialize random number generator
  randomSeed(esp_random());

  WiFi.softAP(ssid, password);
  Serial.println("AP Started");
  

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Error");
    return;
  }

  setupWebSocket(server);

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.begin();
}


void loop() {
  // put your main code here, to run repeatedly:
}