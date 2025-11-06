#ifndef MDNS_SERVICE_H
#define MDNS_SERVICE_H

#include <ESP8266mDNS.h>

class MDNSService {
  public:
    static void begin();
    static void update();
    static void addService(const char* service, const char* proto, uint16_t port);
    
  private:
    static bool isStarted;
};

#endif