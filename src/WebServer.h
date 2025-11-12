#ifndef MOTOR_TESTER_WEBSERVER_H
#define MOTOR_TESTER_WEBSERVER_H

#ifdef ESP32
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

class WebServer {
  public:
    static void begin();
    static void handle();
    
  private:
    static AsyncWebServer server;
    static bool isStarted;
    
    static void setupRoutes();
    static void handleNotFound(AsyncWebServerRequest *request);
};

#endif