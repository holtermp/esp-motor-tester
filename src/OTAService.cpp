#include "OTAService.h"

bool OTAService::isStarted = false;

void OTAService::begin() {
  if (isStarted) return;
  
  ArduinoOTA.setHostname("esp-racepi-motor-tester");
  
  ArduinoOTA.onStart([]() {
    onStart();
  });
  
  ArduinoOTA.onEnd([]() {
    onEnd();
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    onProgress(progress, total);
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    onError(error);
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA service started");
  
  isStarted = true;
}

void OTAService::handle() {
  if (isStarted) {
    ArduinoOTA.handle();
  }
}

void OTAService::onStart() {
  String type;
  if (ArduinoOTA.getCommand() == U_FLASH) {
    type = "sketch";
  } else { // U_SPIFFS
    type = "filesystem";
  }
  
  Serial.println("Start updating " + type);
}

void OTAService::onEnd() {
  Serial.println("\nEnd");
}

void OTAService::onProgress(unsigned int progress, unsigned int total) {
  Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
}

void OTAService::onError(ota_error_t error) {
  Serial.printf("Error[%u]: ", error);
  if (error == OTA_AUTH_ERROR) {
    Serial.println("Auth Failed");
  } else if (error == OTA_BEGIN_ERROR) {
    Serial.println("Begin Failed");
  } else if (error == OTA_CONNECT_ERROR) {
    Serial.println("Connect Failed");
  } else if (error == OTA_RECEIVE_ERROR) {
    Serial.println("Receive Failed");
  } else if (error == OTA_END_ERROR) {
    Serial.println("End Failed");
  }
}