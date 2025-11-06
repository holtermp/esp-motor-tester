#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "wifi_secrets.h"
#include "MDNSService.h"
#include "OTAService.h"
#include "WebServer.h"
#include "RPMCounter.h"

// Pin definitions
#define RPM_SENSOR_PIN D4

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP RacePi Motor Tester ===");
  Serial.println("Initializing...");
  
  // Initialize WiFi
  WiFi.begin(MY_SSID, MY_PW);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Hostname: esp-racepi-motor-tester.local");
  Serial.println();
  
  // Initialize services
  MDNSService::begin();
  OTAService::begin();
  WebServer::begin();
  
  // Initialize RPM counter
  RPMCounter::begin(RPM_SENSOR_PIN);
  
  Serial.println("=== System Ready ===");
  Serial.println("RPM measurement active on pin D4");
  Serial.println("Visit http://esp-racepi-motor-tester.local for web interface");
  Serial.println();
}

void loop() {
  // Update all services
  MDNSService::update();
  OTAService::handle();
  WebServer::handle();
  
  // Process RPM counter signals
  RPMCounter::update();
  
  // Keep the main loop responsive
  delay(10);
}