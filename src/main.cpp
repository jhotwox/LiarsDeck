// #include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <cstdlib>
#include <ctime>

#include "network/WebSocketHandler.h"

const char* environment = "development";
// const char* environment = "production";

// Production Access Point credentials
const char* ssid = "LyingCards";
const char* password = "12345678";

// Development Wi-Fi credentials
const char* wifi_ssid = "Totalplay-61AE"; // Replace with your Wi-Fi SSID
const char* wifi_password = "Refurinfunflay"; // Replace with your Wi-Fi password

AsyncWebServer server(80);
// AsyncWebSocket ws("/ws");

void setup() {
  Serial.begin(115200);
  
  // Initialize random number generator
  // randomSeed(esp_random());
  srand(time(0));

  // Set up Wi-Fi based on the environment
  if (strcmp(environment, "production") == 0) {
    // Wi-Fi Access Point Mode for Production Environment
    WiFi.softAP(ssid, password);
    Serial.println("AP Started");
    Serial.println(WiFi.localIP());
  } else {
    // Wi-Fi Client Mode for Development Environment
    // WiFi.mode(WIFI_AP_STA);
    WiFi.begin(wifi_ssid, wifi_password);
    Serial.println("Connecting to Wi-Fi");

    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
    }
    Serial.println("Connected to Wi-Fi!");
    Serial.println(WiFi.localIP());
  }

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