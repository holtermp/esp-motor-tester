#ifndef OTA_SERVICE_H
#define OTA_SERVICE_H

#include <ArduinoOTA.h>

class OTAService {
  public:
    static void begin();
    static void handle();
    
  private:
    static void onStart();
    static void onEnd();
    static void onProgress(unsigned int progress, unsigned int total);
    static void onError(ota_error_t error);
    
    static bool isStarted;
};

#endif