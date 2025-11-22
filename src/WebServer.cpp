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
    const char* html = R"(<!DOCTYPE html>
<html><head><title>ESP Motor Tester</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>
<style>
body{font-family:Arial;margin:0;padding:20px;background:#f0f0f0}
.container{max-width:1000px;margin:0 auto;background:white;padding:20px;border-radius:10px}
h1{color:#333;text-align:center}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:20px;margin:20px 0}
.box{background:#f8f9fa;padding:15px;border-radius:5px}
.status{text-align:center;background:#d4edda;color:#155724;padding:10px;border-radius:5px;margin:20px 0}
.control{text-align:center;margin:20px 0;padding:15px;background:#e7f3ff;border-radius:5px}
.motor{text-align:center;margin:20px 0;padding:15px;background:#f8f9fa;border-radius:5px;border:2px solid #28a745}
.graph{margin:20px 0;padding:15px;background:#ffffff;border-radius:5px;border:1px solid #ddd}
.buttons{display:flex;justify-content:center;gap:10px;margin:15px 0;flex-wrap:wrap}
.btn{padding:8px 12px;background:#fff;border:2px solid #28a745;border-radius:5px;cursor:pointer}
.btn.active{background:#28a745;color:white}
.rpm{font-size:1.5em;font-weight:bold;color:#007bff}
#chartContainer{height:400px;display:none;margin-top:20px}
</style></head><body>
<div class='container'>
<h1>ESP Motor Tester</h1>
<div class='status'>System Status: <span id='status'>Loading...</span></div>
<div class='control'>
<label><input type='checkbox' id='live' onclick='toggleLive()'> Live Updates (2s)</label>
<div>RPM: <span id='rpm' class='rpm'>--</span></div>
</div>
<div class='motor'>
<h3>Motor Control</h3>
<div>Speed: <span id='speed'>0</span>%</div>
<div class='buttons'>
<button class='btn' onclick='setSpeed(0)'>OFF</button>
<button class='btn' onclick='setSpeed(25)'>25%</button>
<button class='btn' onclick='setSpeed(50)'>50%</button>
<button class='btn' onclick='setSpeed(75)'>75%</button>
<button class='btn' onclick='setSpeed(100)'>100%</button>
</div>
<div style='margin-top:15px;'>
<button class='btn' onclick='startAccelerationTest()' style='background:#ff6b35;border-color:#ff6b35;color:white;'>Acceleration Test</button>
<button class='btn' onclick='startMotorTest()' style='background:#007bff;border-color:#007bff;color:white;margin-left:10px;'>Start Motor Test</button>
<button class='btn' onclick='getTestResult()' style='background:#28a745;border-color:#28a745;color:white;margin-left:10px;'>Get Test Result</button>
<div id='testResult' style='margin-top:10px;font-weight:bold;'></div>
</div></div>
<div class='graph' id='chartContainer'>
<h3>Motor Test Results</h3>
<canvas id='rpmChart' width='800' height='400'></canvas>
</div>
<div class='grid'>
<div class='box'><h3>Device</h3><div id='device'>Loading...</div></div>
<div class='box'><h3>RPM Sensor</h3><div id='sensor'>Loading...</div></div>
<div class='box'><h3>Motor</h3><div id='motor'>Loading...</div></div>
<div class='box'><h3>Network</h3><div id='network'>Loading...</div></div>
</div></div>
<script>
let interval=null;
let rpmChart=null;
function toggleLive(){
let cb=document.getElementById('live');
if(cb.checked){interval=setInterval(update,2000);update();}
else{clearInterval(interval);interval=null;}
}
function update(){
fetch('/api/status').then(r=>r.json()).then(d=>{
document.getElementById('status').textContent='Online';
document.getElementById('rpm').textContent=d.rpm.current+' RPM';
document.getElementById('speed').textContent=d.motor.speed;
document.getElementById('device').innerHTML='Free Heap: '+d.freeHeap+'<br>IP: '+d.ip;
document.getElementById('sensor').innerHTML='Pin: D4<br>RPM: '+d.rpm.current+'<br>Signals: '+d.rpm.signalCount;
document.getElementById('motor').innerHTML='Speed: '+d.motor.speed+'%<br>Running: '+(d.motor.running?'Yes':'No')+'<br>PWM: 0-255';
document.getElementById('network').innerHTML='SSID: '+d.ssid+'<br>Signal: '+d.rssi+' dBm'+(d.motorTest?' Test: '+(d.motorTest.running?'Running':'Idle'):'');
updateBtns(d.motor.speed);
}).catch(e=>{document.getElementById('status').textContent='Error';});}
function setSpeed(s){
let fd=new FormData();fd.append('speed',s);
fetch('/api/motor/speed',{method:'POST',body:fd})
.then(r=>r.json()).then(d=>{
if(d.success){document.getElementById('speed').textContent=s;updateBtns(s);}
}).catch(e=>console.log(e));}
function updateBtns(speed){
document.querySelectorAll('.btn').forEach(b=>b.classList.remove('active'));
let btns=document.querySelectorAll('.btn');
let speeds=[0,25,50,75,100];
let idx=speeds.indexOf(speed);
if(idx>=0)btns[idx].classList.add('active');
}
function startAccelerationTest(){
document.getElementById('testResult').innerHTML='<span style="color:orange">Running acceleration test...</span>';
fetch('/api/motor/acceleration-test',{method:'POST'})
.then(r=>r.json()).then(d=>{
if(d.success){
document.getElementById('testResult').innerHTML='<span style="color:green">Test started! Check serial output for results.</span>';
setTimeout(()=>{document.getElementById('testResult').innerHTML='';},5000);
}else{
document.getElementById('testResult').innerHTML='<span style="color:red">Test failed to start</span>';
}
}).catch(e=>{
document.getElementById('testResult').innerHTML='<span style="color:red">Error starting test</span>';
console.log(e);
});}
function startMotorTest(){
document.getElementById('testResult').innerHTML='<span style="color:orange">Starting motor test...</span>';
fetch('/api/motor-test/start',{method:'POST'})
.then(r=>r.json()).then(d=>{
if(d.success){
document.getElementById('testResult').innerHTML='<span style="color:green">Motor test started! It will take ~5 seconds.</span>';
}else{
document.getElementById('testResult').innerHTML='<span style="color:red">Failed to start motor test</span>';
}
}).catch(e=>{
document.getElementById('testResult').innerHTML='<span style="color:red">Error starting motor test</span>';
console.log(e);
});}
function getTestResult(){
document.getElementById('testResult').innerHTML='<span style="color:blue">Getting test result...</span>';
fetch('/api/motor-test/result')
.then(r=>r.json()).then(d=>{
if(d.samples){
let resultText = '<span style="color:green">Test completed with '+d.samples.length+' samples.</span>';
if(d.analysis){
resultText += '<br><strong>Time to Top RPM:</strong><br>';
resultText += '• Threshold Method: '+d.analysis.timeToTopRPM_ms.thresholdMethod+'ms<br>';
resultText += '• Moving Average: '+d.analysis.timeToTopRPM_ms.movingAverageMethod+'ms<br>';
resultText += '• Settling Method: '+d.analysis.timeToTopRPM_ms.settlingMethod+'ms<br>';
resultText += '<small>Max RPM: '+d.analysis.maxRPM+' | Threshold: '+d.analysis.parameters.thresholdPercent+'%</small>';
if(d.timing){
resultText += '<br><small>Actual frequency: '+d.timing.actualSampleFrequencyHz+'Hz ('+d.timing.actualSampleIntervalMs+'ms/sample)</small>';
}
}
document.getElementById('testResult').innerHTML=resultText;
console.log('Motor Test Results:',d);
displayChart(d.samples, d.analysis, d.timing);
}else{
document.getElementById('testResult').innerHTML='<span style="color:orange">'+d.message+'</span>';
}
}).catch(e=>{
document.getElementById('testResult').innerHTML='<span style="color:red">Error getting test result</span>';
console.log(e);
});
}
function displayChart(samples, analysis, timing){
const ctx=document.getElementById('rpmChart').getContext('2d');
document.getElementById('chartContainer').style.display='block';

// Use actual timing data if available, otherwise fallback to target interval
const sampleIntervalMs = (timing && timing.actualSampleIntervalMs) ? 
                         timing.actualSampleIntervalMs : 10;
const labels=samples.map((v,i)=>(i*sampleIntervalMs).toFixed(1)+'ms');

// Prepare datasets
let datasets=[{
label:'RPM',
data:samples,
borderColor:'#007bff',
backgroundColor:'rgba(0,123,255,0.1)',
borderWidth:2,
fill:true,
tension:0.1
}];

// Add analysis markers if available
if(analysis && analysis.timeToTopRPM_ms){
const thresholdIndex = Math.floor(analysis.timeToTopRPM_ms.thresholdMethod/sampleIntervalMs);
const movingAvgIndex = Math.floor(analysis.timeToTopRPM_ms.movingAverageMethod/sampleIntervalMs);
const settlingIndex = Math.floor(analysis.timeToTopRPM_ms.settlingMethod/sampleIntervalMs);
const maxRPM = analysis.maxRPM;
const threshold = maxRPM * (analysis.parameters.thresholdPercent/100);

// Add threshold line
datasets.push({
label:'95% Threshold ('+threshold.toFixed(0)+' RPM)',
data:new Array(samples.length).fill(threshold),
borderColor:'#ff6b35',
backgroundColor:'rgba(255,107,53,0.1)',
borderWidth:1,
borderDash:[5,5],
fill:false,
pointRadius:0
});
}

if(rpmChart){rpmChart.destroy();}
rpmChart=new Chart(ctx,{
type:'line',
data:{
labels:labels,
datasets:datasets
},
options:{
responsive:true,
maintainAspectRatio:false,
scales:{
x:{
display:true,
title:{display:true,text:'Time (ms)'},
ticks:{maxTicksLimit:10}
},
y:{
display:true,
title:{display:true,text:'RPM'},
beginAtZero:true
}
},
plugins:{
title:{
display:true,
text:'Motor Test - RPM vs Time'+(timing?' ('+timing.actualSampleFrequencyHz.toFixed(1)+'Hz sampling)':''),
font:{size:16}
},
legend:{display:true,position:'top'}
},
animation:{duration:1000}
}
});
}
update();
</script></body></html>)";
    request->send(200, "text/html", html);
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
  
  // Acceleration test endpoint
  server.on("/api/motor/acceleration-test", HTTP_POST, [](AsyncWebServerRequest *request){
    String response = "{currently not impelemented}";    
    AsyncWebServerResponse *resp = request->beginResponse(500, "application/json", response);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    request->send(resp);
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