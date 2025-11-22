#ifndef MOTOR_TESTER_WEBSERVER_H
#define MOTOR_TESTER_WEBSERVER_H

#ifdef ESP32
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

// Forward declaration
class MotorTest;

class WebServer {
  public:
    static void begin();
    static void handle();
    static void setMotorTest(MotorTest* test);
    
  private:
    static AsyncWebServer server;
    static bool isStarted;
    static MotorTest* motorTest;
    
    static void setupRoutes();
    static void handleNotFound(AsyncWebServerRequest *request);
};

#endif