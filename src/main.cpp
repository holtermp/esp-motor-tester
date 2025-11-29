#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "wifi_secrets.h"
#include "MDNSService.h"
#include "OTAService.h"
#include "WebServer.h"
#include "RPMCounter.h"
#include "MotorController.h"
#include "MotorTest.h"

// Global motor test instance
MotorTest motorTest;

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
  
  // Set motor test instance for web server
  WebServer::setMotorTest(&motorTest);
  
  // Initialize RPM counter
  RPMCounter::begin();
  
  // Initialize motor controller
  MotorController::begin();
  
  Serial.println("=== System Ready ===");
  Serial.println("RPM measurement active on pin D6");
  Serial.println("Motor control active (L298N on D1, D2, D3)");
  Serial.println("Visit http://esp-racepi-motor-tester.local for web interface");
  Serial.println();
}

void loop() {
  // Check if motor test is running - prioritize for maximum accuracy
  bool testRunning = motorTest.isRunning();
  
  if (testRunning) {
    // During motor test: minimize interference for maximum precision
    // Only update critical components
    RPMCounter::update();
    motorTest.update();
  } else {
    // Normal operation: update all services
    MDNSService::update();
    OTAService::handle();
    WebServer::handle();
    
    // Process RPM counter signals
    RPMCounter::update();   
    // Keep the main loop responsive
    delay(100);
  }
}