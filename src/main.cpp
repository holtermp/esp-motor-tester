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
  
  // Try to connect to existing WiFi network first
  WiFi.begin(MY_SSID, MY_PW);
  Serial.print("Attempting to connect to existing WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Connected to existing WiFi network!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname: esp-racepi-motor-tester.local");
    Serial.println();
  } else {
    Serial.println();
    Serial.println("Failed to connect to existing WiFi. Starting Access Point mode...");
    
    // Start as Access Point
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP-MotorTester", "motortester123");  // SSID and Password
    
    Serial.println("Access Point started!");
    Serial.print("AP SSID: ESP-MotorTester");
    Serial.println();
    Serial.print("AP Password: motortester123");
    Serial.println();
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    Serial.println("Connect your phone to this WiFi network");
  }
  
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
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Visit http://esp-racepi-motor-tester.local for web interface");
  } else {
    Serial.println("Visit http://192.168.4.1 for web interface");
    Serial.println("(Connect to ESP-MotorTester WiFi network first)");
  }
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