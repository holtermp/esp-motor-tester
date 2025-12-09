#include "WebServer.h"
#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include "RPMCounter.h"
#include "MotorController.h"
#include "MotorTest.h"

AsyncWebServer WebServer::server(80);
bool WebServer::isStarted = false;
MotorTest* WebServer::motorTest = nullptr;

// Test mode: simulate AP mode for testing (set to false for production)
#define TEST_AP_MODE false

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

void WebServer::setMotorTest(MotorTest* test) {
  motorTest = test;
}

void WebServer::setupRoutes() {
  // Serve a minimal home page that loads content via AJAX
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    // Check if we're in WiFi mode (or simulating AP mode for testing)
    bool useChartJS = !TEST_AP_MODE && (WiFi.status() == WL_CONNECTED);
    
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    
    // HTML header
    response->print(F("<!DOCTYPE html>\n<html><head><title>ESP Motor Tester</title>\n"));
    response->print(F("<meta name='viewport' content='width=device-width, initial-scale=1'>\n"));
    
    // Conditionally include Chart.js CDN
    if (useChartJS) {
      response->print(F("<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>\n"));
    }
    
    // CSS
    response->print(F("<style>\n"));
    response->print(F("body{font-family:Arial;margin:0;padding:20px;background:#f0f0f0}\n"));
    response->print(F(".container{max-width:1000px;margin:0 auto;background:white;padding:20px;border-radius:10px}\n"));
    response->print(F("h1{color:#333;text-align:center}\n"));
    response->print(F(".grid{display:grid;grid-template-columns:1fr 1fr;gap:20px;margin:20px 0}\n"));
    response->print(F(".box{background:#f8f9fa;padding:15px;border-radius:5px}\n"));
    response->print(F(".status{text-align:center;background:#d4edda;color:#155724;padding:10px;border-radius:5px;margin:20px 0}\n"));
    response->print(F(".control{text-align:center;margin:20px 0;padding:15px;background:#e7f3ff;border-radius:5px}\n"));
    response->print(F(".motor{text-align:center;margin:20px 0;padding:15px;background:#f8f9fa;border-radius:5px;border:2px solid #28a745}\n"));
    response->print(F(".graph{margin:20px 0;padding:15px;background:#ffffff;border-radius:5px;border:1px solid #ddd;box-sizing:border-box}\n"));
    response->print(F(".buttons{display:flex;justify-content:center;gap:10px;margin:15px 0;flex-wrap:wrap}\n"));
    response->print(F(".btn{padding:8px 12px;background:#fff;border:2px solid #28a745;border-radius:5px;cursor:pointer}\n"));
    response->print(F(".btn.active{background:#28a745;color:white}\n"));
    response->print(F(".rpm{font-size:1.5em;font-weight:bold;color:#007bff}\n"));
    response->print(F("#chartContainer{display:none;margin-top:20px}\n"));
    response->print(F("#rpmChart{max-height:400px}\n"));
    response->print(F("</style></head><body>\n"));
    
    // HTML body
    response->print(F("<div class='container'>\n"));
    response->print(F("<h1>ESP Motor Tester</h1>\n"));
    response->print(F("<div class='status'>System Status: <span id='status'>Loading...</span></div>\n"));
    response->print(F("<div class='control'>\n"));
    response->print(F("<label><input type='checkbox' id='live' onclick='toggleLive()'> Live Updates (2s)</label>\n"));
    response->print(F("<div>RPM: <span id='rpm' class='rpm'>--</span></div>\n"));
    response->print(F("</div>\n"));
    response->print(F("<div class='motor'>\n"));
    response->print(F("<h3>Motor Control</h3>\n"));
    response->print(F("<div>Speed: <span id='speed'>0</span>%</div>\n"));
    response->print(F("<div class='buttons'>\n"));
    response->print(F("<button class='btn' onclick='setSpeed(0)'>OFF</button>\n"));
    response->print(F("<button class='btn' onclick='setSpeed(25)'>25%</button>\n"));
    response->print(F("<button class='btn' onclick='setSpeed(50)'>50%</button>\n"));
    response->print(F("<button class='btn' onclick='setSpeed(75)'>75%</button>\n"));
    response->print(F("<button class='btn' onclick='setSpeed(100)'>100%</button>\n"));
    response->print(F("</div>\n"));
    response->print(F("<div style='margin-top:15px;'>\n"));
    response->print(F("<button class='btn' onclick='startMotorTest()' style='background:#007bff;border-color:#007bff;color:white;'>Start Motor Test</button>\n"));
    response->print(F("<button class='btn' onclick='getTestResult()' style='background:#28a745;border-color:#28a745;color:white;margin-left:10px;'>Get Test Result</button>\n"));
    response->print(F("<div id='testResult' style='margin-top:10px;font-weight:bold;'></div>\n"));
    response->print(F("</div></div>\n"));
    response->print(F("<div class='graph' id='chartContainer'>\n"));
    response->print(F("<h3>Motor Test Results</h3>\n"));
    response->print(F("<canvas id='rpmChart' style='display:block;width:100%;height:380px;border:1px solid #ddd;'></canvas>\n"));
    response->print(F("</div>\n"));
    response->print(F("<div class='grid'>\n"));
    response->print(F("<div class='box'><h3>Device</h3><div id='device'>Loading...</div></div>\n"));
    response->print(F("<div class='box'><h3>RPM Sensor</h3><div id='sensor'>Loading...</div></div>\n"));
    response->print(F("<div class='box'><h3>Motor</h3><div id='motor'>Loading...</div></div>\n"));
    response->print(F("<div class='box'><h3>Network</h3><div id='network'>Loading...</div></div>\n"));
    response->print(F("</div></div>\n"));
    
    // JavaScript - Lightweight chart function (only if not using Chart.js)
    if (!useChartJS) {
      response->print(F("<script>\n"));
      response->print(F("function drawChart(c,d){\n"));
      response->print(F("const v=document.getElementById(c),x=v.getContext('2d');\n"));
      response->print(F("const r=v.getBoundingClientRect(),w=v.width=r.width||800,h=v.height=400;\n"));
      response->print(F("x.clearRect(0,0,w,h);if(!d||!d.length)return;\n"));
      response->print(F("const mt=Math.max(...d.map(p=>p.x)),mr=Math.max(...d.map(p=>p.y)),mi=Math.min(...d.map(p=>p.y));\n"));
      response->print(F("const p=40,cw=w-2*p,ch=h-2*p;\n"));
      response->print(F("x.strokeStyle='#ccc';x.lineWidth=1;x.beginPath();\n"));
      response->print(F("x.moveTo(p,p);x.lineTo(p,h-p);x.lineTo(w-p,h-p);x.stroke();\n"));
      response->print(F("x.strokeStyle='#eee';x.lineWidth=0.5;\n"));
      response->print(F("for(let i=1;i<5;i++){const y=p+(ch*i/5);x.beginPath();x.moveTo(p,y);x.lineTo(w-p,y);x.stroke();}\n"));
      response->print(F("x.strokeStyle='#007bff';x.fillStyle='#007bff';x.lineWidth=2;x.beginPath();\n"));
      response->print(F("for(let i=0;i<d.length;i++){\n"));
      response->print(F("const px=p+(d[i].x/mt)*cw,py=h-p-((d[i].y-mi)/(mr-mi))*ch;\n"));
      response->print(F("if(i===0)x.moveTo(px,py);else x.lineTo(px,py);x.fillRect(px-2,py-2,4,4);}\n"));
      response->print(F("x.stroke();x.fillStyle='#333';x.font='12px Arial';\n"));
      response->print(F("x.fillText('Time (ms)',w/2-30,h-5);x.save();x.translate(15,h/2);x.rotate(-Math.PI/2);\n"));
      response->print(F("x.fillText('RPM',0,0);x.restore();\n"));
      response->print(F("x.fillText('0',p-20,h-p+5);x.fillText(mt.toFixed(0),w-p-20,h-p+15);\n"));
      response->print(F("x.fillText(mi.toFixed(0),5,h-p);x.fillText(mr.toFixed(0),5,p+5);}\n"));
      response->print(F("</script>\n"));
    }
    
    // JavaScript - Main application logic
    response->print(F("<script>\n"));
    response->print(F("let interval=null;\n"));
    response->print(F("function toggleLive(){\n"));
    response->print(F("let cb=document.getElementById('live');\n"));
    response->print(F("if(cb.checked){interval=setInterval(update,2000);update();}\n"));
    response->print(F("else{clearInterval(interval);interval=null;}}\n"));
    response->print(F("function update(){\n"));
    response->print(F("fetch('/api/status').then(r=>r.json()).then(d=>{\n"));
    response->print(F("document.getElementById('status').textContent='Online';\n"));
    response->print(F("document.getElementById('rpm').textContent=d.rpm.current+' RPM';\n"));
    response->print(F("document.getElementById('speed').textContent=d.motor.speed;\n"));
    response->print(F("document.getElementById('device').innerHTML='Free Heap: '+d.freeHeap+'<br>IP: '+d.ip;\n"));
    response->print(F("document.getElementById('sensor').innerHTML='Pin: D6<br>RPM: '+d.rpm.current+'<br>Signals: '+d.rpm.signalCount;\n"));
    response->print(F("document.getElementById('motor').innerHTML='Speed: '+d.motor.speed+'%<br>Running: '+(d.motor.running?'Yes':'No')+'<br>PWM: 0-255';\n"));
    response->print(F("document.getElementById('network').innerHTML='SSID: '+d.ssid+'<br>Signal: '+d.rssi+' dBm'+(d.motorTest?' Test: '+(d.motorTest.running?'Running':'Idle'):'');\n"));
    response->print(F("updateBtns(d.motor.speed);\n"));
    response->print(F("}).catch(e=>{document.getElementById('status').textContent='Error';});}\n"));
    response->print(F("function setSpeed(s){\n"));
    response->print(F("let fd=new FormData();fd.append('speed',s);\n"));
    response->print(F("fetch('/api/motor/speed',{method:'POST',body:fd})\n"));
    response->print(F(".then(r=>r.json()).then(d=>{\n"));
    response->print(F("if(d.success){document.getElementById('speed').textContent=s;updateBtns(s);}\n"));
    response->print(F("}).catch(e=>console.log(e));}\n"));
    response->print(F("function updateBtns(speed){\n"));
    response->print(F("document.querySelectorAll('.btn').forEach(b=>b.classList.remove('active'));\n"));
    response->print(F("let btns=document.querySelectorAll('.btn');\n"));
    response->print(F("let speeds=[0,25,50,75,100];\n"));
    response->print(F("let idx=speeds.indexOf(speed);\n"));
    response->print(F("if(idx>=0)btns[idx].classList.add('active');}\n"));
    response->print(F("function startMotorTest(){\n"));
    response->print(F("document.getElementById('testResult').innerHTML='<span style=\"color:orange\">Starting motor test...</span>';\n"));
    response->print(F("fetch('/api/motor-test/start',{method:'POST'})\n"));
    response->print(F(".then(r=>r.json()).then(d=>{\n"));
    response->print(F("if(d.success){\n"));
    response->print(F("document.getElementById('testResult').innerHTML='<span style=\"color:green\">Motor test started! It will take ~5 seconds.</span>';\n"));
    response->print(F("}else{\n"));
    response->print(F("document.getElementById('testResult').innerHTML='<span style=\"color:red\">Failed to start motor test</span>';\n"));
    response->print(F("}\n"));
    response->print(F("}).catch(e=>{\n"));
    response->print(F("document.getElementById('testResult').innerHTML='<span style=\"color:red\">Error starting motor test</span>';\n"));
    response->print(F("console.log(e);\n"));
    response->print(F("});}\n"));
    response->print(F("function getTestResult(){\n"));
    response->print(F("document.getElementById('testResult').innerHTML='<span style=\"color:blue\">Getting test result...</span>';\n"));
    response->print(F("fetch('/api/motor-test/result')\n"));
    response->print(F(".then(r=>r.json()).then(d=>{\n"));
    response->print(F("if(d.samples){\n"));
    response->print(F("let resultText = '<span style=\"color:green\">Test completed with '+d.samples.length+' samples.</span>';\n"));
    response->print(F("if(d.analysis){\n"));
    response->print(F("resultText += '<br><strong>Time to Top RPM: '+d.analysis.timeToTopRPM_ms+'ms</strong><br>';\n"));
    response->print(F("resultText += '<small>Max RPM: '+d.analysis.maxRPM+' | Window: '+d.analysis.parameters.movingAverageWindow+' samples</small>';\n"));
    response->print(F("if(d.timing){\n"));
    response->print(F("resultText += '<br><small>Actual frequency: '+d.timing.actualSampleFrequencyHz+'Hz ('+d.timing.actualSampleIntervalMs+'ms/sample)</small>';\n"));
    response->print(F("}\n"));
    response->print(F("}\n"));
    response->print(F("document.getElementById('testResult').innerHTML=resultText;\n"));
    response->print(F("console.log('Motor Test Results:',d);\n"));
    response->print(F("displayChart(d.samples, d.filteredSamples, d.analysis, d.timing);\n"));
    response->print(F("}else{\n"));
    response->print(F("document.getElementById('testResult').innerHTML='<span style=\"color:orange\">'+d.message+'</span>';\n"));
    response->print(F("}\n"));
    response->print(F("}).catch(e=>{\n"));
    response->print(F("document.getElementById('testResult').innerHTML='<span style=\"color:red\">Error getting test result</span>';\n"));
    response->print(F("console.log(e);\n"));
    response->print(F("});\n"));
    response->print(F("}\n"));
    
    // displayChart function - conditional implementation
    if (useChartJS) {
      // Chart.js version
      response->print(F("let rpmChart=null;\n"));
      response->print(F("function displayChart(samples, filteredSamples, analysis, timing){\n"));
      response->print(F("const ctx=document.getElementById('rpmChart').getContext('2d');\n"));
      response->print(F("document.getElementById('chartContainer').style.display='block';\n"));
      response->print(F("const sampleIntervalMs = (timing && timing.actualSampleIntervalMs) ? timing.actualSampleIntervalMs : 10;\n"));
      response->print(F("const labels=samples.map((v,i)=>(i*sampleIntervalMs).toFixed(1)+'ms');\n"));
      response->print(F("let datasets=[{\n"));
      response->print(F("label:'Original RPM',\n"));
      response->print(F("data:samples,\n"));
      response->print(F("borderColor:'#6c757d',\n"));
      response->print(F("backgroundColor:'rgba(108,117,125,0.1)',\n"));
      response->print(F("borderWidth:1,\n"));
      response->print(F("fill:false,\n"));
      response->print(F("tension:0.1,\n"));
      response->print(F("pointRadius:1\n"));
      response->print(F("},{\n"));
      response->print(F("label:'Filtered RPM (outliers removed)',\n"));
      response->print(F("data:filteredSamples || samples,\n"));
      response->print(F("borderColor:'#007bff',\n"));
      response->print(F("backgroundColor:'rgba(0,123,255,0.1)',\n"));
      response->print(F("borderWidth:2,\n"));
      response->print(F("fill:false,\n"));
      response->print(F("tension:0.1,\n"));
      response->print(F("pointRadius:1\n"));
      response->print(F("}];\n"));
      response->print(F("if(analysis && analysis.timeToTopRPM_ms){\n"));
      response->print(F("const timeIndex = Math.floor(analysis.timeToTopRPM_ms / sampleIntervalMs);\n"));
      response->print(F("if(timeIndex >= 0 && timeIndex < samples.length) {\n"));
      response->print(F("const markerData = new Array(samples.length).fill(null);\n"));
      response->print(F("markerData[timeIndex] = filteredSamples ? filteredSamples[timeIndex] : samples[timeIndex];\n"));
      response->print(F("datasets.push({\n"));
      response->print(F("label:'Time to Top RPM: '+analysis.timeToTopRPM_ms.toFixed(0)+'ms ('+markerData[timeIndex].toFixed(0)+' RPM)',\n"));
      response->print(F("data: markerData,\n"));
      response->print(F("borderColor:'#28a745',\n"));
      response->print(F("backgroundColor:'#28a745',\n"));
      response->print(F("borderWidth:0,\n"));
      response->print(F("fill:false,\n"));
      response->print(F("pointRadius:10,\n"));
      response->print(F("pointHoverRadius:12,\n"));
      response->print(F("pointBackgroundColor:'#28a745',\n"));
      response->print(F("pointBorderColor:'#ffffff',\n"));
      response->print(F("pointBorderWidth:3,\n"));
      response->print(F("showLine:false,\n"));
      response->print(F("pointStyle:'circle'\n"));
      response->print(F("});\n"));
      response->print(F("}\n"));
      response->print(F("}\n"));
      response->print(F("if(rpmChart){rpmChart.destroy();}\n"));
      response->print(F("rpmChart=new Chart(ctx,{\n"));
      response->print(F("type:'line',\n"));
      response->print(F("data:{\n"));
      response->print(F("labels:labels,\n"));
      response->print(F("datasets:datasets\n"));
      response->print(F("},\n"));
      response->print(F("options:{\n"));
      response->print(F("responsive:true,\n"));
      response->print(F("maintainAspectRatio:false,\n"));
      response->print(F("scales:{\n"));
      response->print(F("x:{\n"));
      response->print(F("display:true,\n"));
      response->print(F("title:{display:true,text:'Time (ms)'},\n"));
      response->print(F("ticks:{maxTicksLimit:10}\n"));
      response->print(F("},\n"));
      response->print(F("y:{\n"));
      response->print(F("display:true,\n"));
      response->print(F("title:{display:true,text:'RPM'},\n"));
      response->print(F("beginAtZero:true\n"));
      response->print(F("}\n"));
      response->print(F("},\n"));
      response->print(F("plugins:{\n"));
      response->print(F("title:{\n"));
      response->print(F("display:true,\n"));
      response->print(F("text:'Motor Test - RPM vs Time'+(timing?' ('+timing.actualSampleFrequencyHz.toFixed(1)+'Hz sampling)':'')+\n"));
      response->print(F("(analysis && analysis.timeToTopRPM_ms ? ' - Moving Average Method' : ''),\n"));
      response->print(F("font:{size:16}\n"));
      response->print(F("},\n"));
      response->print(F("legend:{display:true,position:'top'}\n"));
      response->print(F("},\n"));
      response->print(F("animation:{duration:1000}\n"));
      response->print(F("}\n"));
      response->print(F("});\n"));
      response->print(F("}\n"));
    } else {
      // Lightweight canvas version
      response->print(F("function displayChart(samples, filteredSamples, analysis, timing){\n"));
      response->print(F("document.getElementById('chartContainer').style.display='block';\n"));
      response->print(F("const sampleIntervalMs = (timing && timing.actualSampleIntervalMs) ? timing.actualSampleIntervalMs : 10;\n"));
      response->print(F("const data = (filteredSamples || samples).map((y,i)=>({x:i*sampleIntervalMs,y:y}));\n"));
      response->print(F("drawChart('rpmChart', data);\n"));
      response->print(F("}\n"));
    }
    
    response->print(F("update();\n"));
    response->print(F("</script></body></html>\n"));
    request->send(response);
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
  
  // API endpoint for motor control - GET current status
  server.on("/api/motor", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"speed\":" + String(MotorController::getCurrentSpeed()) + ",";
    json += "\"lastUpdate\":" + String(MotorController::getLastUpdateTime()) + ",";
    json += "\"timestamp\":" + String(millis());
    json += "}";
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });
  
  // API endpoint for motor control - POST to set speed
  server.on("/api/motor/speed", HTTP_POST, [](AsyncWebServerRequest *request){
    String response = "{\"error\":\"No speed parameter provided\"}";
    int responseCode = 400;
    
    if (request->hasParam("speed", true)) {
      int speed = request->getParam("speed", true)->value().toInt();
      speed = constrain(speed, 0, 100);
      MotorController::setSpeed(speed);
      
      response = "{";
      response += "\"success\":true,";
      response += "\"speed\":" + String(speed) + ",";
      response += "\"timestamp\":" + String(millis());
      response += "}";
      responseCode = 200;
    }
    
    AsyncWebServerResponse *resp = request->beginResponse(responseCode, "application/json", response);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    request->send(resp);
  });
  
  // Combined API endpoint for all data
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"rpm\":{";
    json += "\"current\":" + String(RPMCounter::getCurrentRPM(), 1) + ",";
    json += "\"signalCount\":" + String(RPMCounter::getSignalCount()) + ",";
    json += "\"lastSignalTime\":" + String(RPMCounter::getLastSignalTime()) + ",";
    json += "\"timeBetweenSignalsMicros\":" + String(RPMCounter::getTimeBetweenSignals()) + ",";
    json += "\"timeBetweenSignalsMs\":" + String(RPMCounter::getTimeBetweenSignals() / 1000.0, 3);
    json += "},";
    json += "\"motor\":{";
    json += "\"speed\":" + String(MotorController::getCurrentSpeed()) + ",";
    json += "\"lastUpdate\":" + String(MotorController::getLastUpdateTime()) + ",";
    json += "\"running\":" + String(MotorController::getCurrentSpeed() > 0 ? "true" : "false");
    json += "},";
    if (motorTest != nullptr) {
      json += "\"motorTest\":{";
      json += "\"running\":" + String(motorTest->isRunning() ? "true" : "false");
      json += "},";
    }
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"timestamp\":" + String(millis());
    json += "}";
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });
  
  // Motor test start endpoint
  server.on("/api/motor-test/start", HTTP_POST, [](AsyncWebServerRequest *request){
    String response;
    int responseCode = 500;
    
    if (motorTest != nullptr) {
      if (!motorTest->isRunning()) {
        motorTest->start(TEST_TYPE_ACCELERATION);
        response = "{\"success\":true,\"message\":\"Motor test started\"}";
        responseCode = 200;
      } else {
        response = "{\"success\":false,\"message\":\"Test already running\"}";
        responseCode = 409; // Conflict
      }
    } else {
      response = "{\"success\":false,\"message\":\"Motor test not initialized\"}";
      responseCode = 500;
    }
    
    AsyncWebServerResponse *resp = request->beginResponse(responseCode, "application/json", response);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    request->send(resp);
  });
  
  // Motor test result endpoint
  server.on("/api/motor-test/result", HTTP_GET, [](AsyncWebServerRequest *request){
    String response;
    int responseCode = 200;
    
    if (motorTest != nullptr) {
      String result = motorTest->getResult();
      if (result.startsWith("{")) {
        // Valid JSON result
        response = result;
      } else {
        // Text message, wrap in JSON
        response = "{\"message\":\"" + result + "\"}";
      }
    } else {
      response = "{\"message\":\"Motor test not initialized\"}";
      responseCode = 500;
    }
    
    AsyncWebServerResponse *resp = request->beginResponse(responseCode, "application/json", response);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    request->send(resp);
  });
  
  // Motor test status endpoint
  server.on("/api/motor-test/status", HTTP_GET, [](AsyncWebServerRequest *request){
    String response;
    int responseCode = 200;
    
    if (motorTest != nullptr) {
      response = "{";
      response += "\"running\":" + String(motorTest->isRunning() ? "true" : "false") + ",";
      response += "\"timestamp\":" + String(millis());
      response += "}";
    } else {
      response = "{\"running\":false,\"error\":\"Motor test not initialized\"}";
      responseCode = 500;
    }
    
    AsyncWebServerResponse *resp = request->beginResponse(responseCode, "application/json", response);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    request->send(resp);
  });
  
  // Handle not found
  server.onNotFound([](AsyncWebServerRequest *request){
    handleNotFound(request);
  });
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