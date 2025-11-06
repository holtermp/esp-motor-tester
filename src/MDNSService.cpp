#include "MDNSService.h"

bool MDNSService::isStarted = false;

void MDNSService::begin() {
  if (isStarted) return;
  
  if (!MDNS.begin("esp-racepi-motor-tester")) {
    Serial.println("Error setting up MDNS responder!");
    return;
  }
  
  Serial.println("mDNS responder started: esp-racepi-motor-tester.local");
  
  // Add HTTP service
  MDNS.addService("http", "tcp", 80);
  
  isStarted = true;
}

void MDNSService::update() {
  if (isStarted) {
    MDNS.update();
  }
}

void MDNSService::addService(const char* service, const char* proto, uint16_t port) {
  if (isStarted) {
    MDNS.addService(service, proto, port);
  }
}