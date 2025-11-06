#include "WebServer.h"
#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include "RPMCounter.h"

AsyncWebServer WebServer::server(80);
bool WebServer::isStarted = false;

void WebServer::begin() {
  if (isStarted) return;
  
  setupRoutes();
  server.begin();
  Serial.println("Web server started on port 80");
  
  isStarted = true;
}

void WebServer::handle() {
  // AsyncWebServer handles requests automatically
  // This method exists for consistency with other services
}

void WebServer::setupRoutes() {
  // Serve the home page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", generateHomePage());
  });
  
  // API endpoint for RPM data
  server.on("/api/rpm", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"rpm\":" + String(RPMCounter::getCurrentRPM(), 1) + ",";
    json += "\"signalCount\":" + String(RPMCounter::getSignalCount()) + ",";
    json += "\"lastSignalTime\":" + String(RPMCounter::getLastSignalTime()) + ",";
    json += "\"timeBetweenSignalsMicros\":" + String(RPMCounter::getTimeBetweenSignals()) + ",";
    json += "\"timeBetweenSignalsMs\":" + String(RPMCounter::getTimeBetweenSignals() / 1000.0, 3) + ",";
    json += "\"timestamp\":" + String(millis());
    json += "}";
    
    // Set CORS headers for API access
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });
  
  // Handle not found
  server.onNotFound([](AsyncWebServerRequest *request){
    handleNotFound(request);
  });
}

String WebServer::generateHomePage() {
  String html = "<!DOCTYPE html>";
  html += "<html><head>";
  html += "<title>ESP RacePi Motor Tester</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f0f0f0; }";
  html += ".container { max-width: 800px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1); }";
  html += "h1 { color: #333; text-align: center; }";
  html += ".info-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin: 20px 0; }";
  html += ".info-box { background-color: #f8f9fa; padding: 15px; border-radius: 5px; }";
  html += ".info-box h3 { margin-top: 0; color: #495057; }";
  html += ".status { text-align: center; background-color: #d4edda; color: #155724; padding: 10px; border-radius: 5px; margin: 20px 0; }";
  html += ".coming-soon { text-align: center; background-color: #fff3cd; color: #856404; padding: 15px; border-radius: 5px; margin: 20px 0; }";
  html += ".live-control { text-align: center; margin: 20px 0; padding: 15px; background-color: #e7f3ff; border-radius: 5px; }";
  html += ".rpm-value { font-size: 1.5em; font-weight: bold; color: #007bff; }";
  html += ".updating { animation: pulse 1s infinite; }";
  html += "@keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.7; } 100% { opacity: 1; } }";
  html += "</style>";
  html += "</head><body>";
  
  html += "<div class='container'>";
  html += "<h1>ESP RacePi Motor Tester</h1>";
  
  html += "<div class='status'>";
  html += "<strong>System Status:</strong> Online and Ready";
  html += "</div>";
  
  html += "<div class='live-control'>";
  html += "<label><input type='checkbox' id='liveUpdate' onchange='toggleLiveUpdate()'> Enable Live RPM Updates (every 5 seconds)</label>";
  html += "<div style='margin-top: 10px;'>";
  html += "<span>Live RPM: </span><span id='liveRPM' class='rpm-value'>--</span>";
  html += "</div>";
  html += "</div>";
  
  html += "<div class='info-grid'>";
  
  html += "<div class='info-box'>";
  html += "<h3>Device Information</h3>";
  html += "<p><strong>Hostname:</strong> esp-racepi-motor-tester.local</p>";
  html += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
  html += "<p><strong>MAC Address:</strong> " + WiFi.macAddress() + "</p>";
  html += "<p><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
  html += "</div>";
  
  html += "<div class='info-box'>";
  html += "<h3>RPM Sensor Status</h3>";
  html += "<p><strong>Sensor Pin:</strong> D4</p>";
  html += "<p><strong>Signal Count:</strong> <span id='signalCount'>" + String(RPMCounter::getSignalCount()) + "</span></p>";
  html += "<p><strong>Current RPM:</strong> <span id='currentRPM'>" + String(RPMCounter::getCurrentRPM(), 1) + "</span></p>";
  html += "<p><strong>Last Signal:</strong> <span id='lastSignalTime'>" + String(RPMCounter::getLastSignalTime()) + "</span> ms</p>";
  html += "<p><strong>Signal Interval:</strong> <span id='timeBetweenSignalsMicros'>" + String(RPMCounter::getTimeBetweenSignals()) + "</span> μs (<span id='timeBetweenSignalsMs'>" + String(RPMCounter::getTimeBetweenSignals() / 1000.0, 3) + "</span> ms)</p>";
  html += "<p><strong>Status:</strong> " + String(RPMCounter::hasPendingSignal() ? "Signal Pending" : "Ready") + "</p>";
  html += "<p><strong>Debounce:</strong> 0.5ms (for high-speed motors)</p>";
  html += "</div>";
  
  html += "<div class='info-box'>";
  html += "<h3>Network Information</h3>";
  html += "<p><strong>SSID:</strong> " + WiFi.SSID() + "</p>";
  html += "<p><strong>Signal Strength:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
  html += "<p><strong>Gateway:</strong> " + WiFi.gatewayIP().toString() + "</p>";
  html += "<p><strong>Subnet:</strong> " + WiFi.subnetMask().toString() + "</p>";
  html += "</div>";
  
  html += "</div>";
  
  html += "<div class='coming-soon'>";
  html += "<h3>High-Speed RPM Measurement Active!</h3>";
  html += "<p>The device is now measuring RPM signals on pin D4 with 0.5ms debouncing.</p>";
  html += "<p>Capable of measuring motors up to 21,000 RPM accurately.</p>";
  html += "<p>Connect your RPM sensor to pin D4 and monitor the serial output for signal detection.</p>";
  html += "<p>Future features will include RPM logging, averaging, and motor control capabilities.</p>";
  html += "</div>";
  
  html += "</div>";
  
  // Add JavaScript for live updates
  html += "<script>";
  html += "let updateInterval = null;";
  html += "function toggleLiveUpdate() {";
  html += "  const checkbox = document.getElementById('liveUpdate');";
  html += "  if (checkbox.checked) {";
  html += "    startLiveUpdate();";
  html += "  } else {";
  html += "    stopLiveUpdate();";
  html += "  }";
  html += "}";
  html += "function startLiveUpdate() {";
  html += "  updateRPMData();"; // Update immediately
  html += "  updateInterval = setInterval(updateRPMData, 5000);"; // Then every 5 seconds
  html += "}";
  html += "function stopLiveUpdate() {";
  html += "  if (updateInterval) {";
  html += "    clearInterval(updateInterval);";
  html += "    updateInterval = null;";
  html += "  }";
  html += "  document.getElementById('liveRPM').classList.remove('updating');";
  html += "}";
  html += "function updateRPMData() {";
  html += "  const liveRPM = document.getElementById('liveRPM');";
  html += "  liveRPM.classList.add('updating');";
  html += "  fetch('/api/rpm')";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      document.getElementById('liveRPM').textContent = data.rpm + ' RPM';";
  html += "      document.getElementById('signalCount').textContent = data.signalCount;";
  html += "      document.getElementById('currentRPM').textContent = data.rpm;";
  html += "      document.getElementById('lastSignalTime').textContent = data.lastSignalTime;";
  html += "      document.getElementById('timeBetweenSignalsMicros').textContent = data.timeBetweenSignalsMicros;";
  html += "      document.getElementById('timeBetweenSignalsMs').textContent = data.timeBetweenSignalsMs;";
  html += "      liveRPM.classList.remove('updating');";
  html += "    })";
  html += "    .catch(error => {";
  html += "      console.error('Error fetching RPM data:', error);";
  html += "      document.getElementById('liveRPM').textContent = 'Error';";
  html += "      liveRPM.classList.remove('updating');";
  html += "    });";
  html += "}";
  html += "</script>";
  
  html += "</body></html>";
  
  return html;
}

void WebServer::handleNotFound(AsyncWebServerRequest *request) {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += request->args();
  message += "\n";
  
  for (uint8_t i = 0; i < request->args(); i++) {
    message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
  }
  
  request->send(404, "text/plain", message);
}