#include "MotorController.h"
#include "RPMCounter.h"

// Static member definitions
int MotorController::currentSpeed = 0;
bool MotorController::motorRunning = false;
unsigned long MotorController::lastUpdateTime = 0;
bool MotorController::accelerationTestActive = false;
unsigned long MotorController::testStartTime = 0;
unsigned long MotorController::testCompletionTime = 0;
float MotorController::targetRPM = 0.0;
bool MotorController::targetRPMReached = false;

// Multi-test sequence variables
const float MotorController::RPM_TARGETS[4] = {15000.0, 16000.0, 17000.0, 18000.0};
int MotorController::currentTestIndex = 0;
unsigned long MotorController::targetTimes[4] = {0, 0, 0, 0};
bool MotorController::allTestsComplete = false;
bool MotorController::waitingBetweenTests = false;
unsigned long MotorController::pauseStartTime = 0;

void MotorController::begin() {
  // Initialize the motor control pins
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  pinMode(ENB_PIN, OUTPUT);
  
  // Set PWM frequency for motor control (higher frequency for smoother operation)
  // ESP8266 default is 1000Hz, motors typically work better with 10-20kHz
  analogWriteFreq(1000); // 10kHz PWM frequency
  
  // Stop motor initially
  stop();
  
  Serial.println("Motor Controller initialized");
  Serial.println("L298N connections:");
  Serial.println("  IN3 -> D1");
  Serial.println("  IN4 -> D2");
  Serial.println("  ENB -> D3 (PWM)");
  Serial.println("  Motor: OUT3(+) and OUT4(-)");
}

void MotorController::setSpeed(int percentage) {
  // Clamp percentage to valid range
  percentage = constrain(percentage, 0, 100);
  
  currentSpeed = percentage;
  lastUpdateTime = millis();
  
  if (percentage == 0) {
    stop();
  } else {
    motorRunning = true;
    updateMotor();
  }
  
  Serial.print("Motor speed set to: ");
  Serial.print(percentage);
  Serial.print("% (PWM: ");
  Serial.print(speedToPWM(percentage));
  Serial.println(")");
}

void MotorController::stop() {
  currentSpeed = 0;
  motorRunning = false;
  lastUpdateTime = millis();
  
  // Stop the motor by setting both direction pins LOW
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, LOW);
  analogWrite(ENB_PIN, 0);
  
  Serial.println("Motor stopped");
}

int MotorController::getCurrentSpeed() {
  return currentSpeed;
}

bool MotorController::isRunning() {
  return motorRunning;
}

unsigned long MotorController::getLastUpdateTime() {
  return lastUpdateTime;
}

void MotorController::updateMotor() {
  if (motorRunning && currentSpeed > 0) {
    // Set direction for forward rotation
    // IN3 = HIGH, IN4 = LOW for forward direction
    digitalWrite(IN3_PIN, HIGH);
    digitalWrite(IN4_PIN, LOW);
    
    // Set PWM speed
    int pwmValue = speedToPWM(currentSpeed);
    analogWrite(ENB_PIN, pwmValue);
    
    Serial.print("Motor running: ");
    Serial.print(currentSpeed);
    Serial.print("% -> PWM: ");
    Serial.print(pwmValue);
    Serial.print(" (IN3=HIGH, IN4=LOW, ENB=PWM)");
    Serial.println();
  }
}

int MotorController::speedToPWM(int percentage) {
  // Convert percentage (0-100) to PWM value (0-255)
  // ESP8266 analogWrite uses 0-255 range (8-bit), not 10-bit!
  
  if (percentage == 0) {
    return 0;
  }
  
  // L298N has voltage drop (~1.4-2V), so we need higher PWM for same effective voltage
  // Map 1-100% to a higher PWM range to compensate
  // Minimum PWM value to overcome L298N voltage drop
  int minPWM = 50;  // Start motor movement
  int maxPWM = 255; // Full speed
  
  return map(percentage, 1, 100, minPWM, maxPWM);
}

void MotorController::startAccelerationTest() {
  // Don't use blocking delay() - this causes stack overflow in async web server!
  // First stop the motor to ensure clean test start
  stop();
  
  // Reset test sequence variables
  accelerationTestActive = true;
  targetRPMReached = false;
  testCompletionTime = 0;
  
  // Reset multi-test sequence variables
  currentTestIndex = 0;
  allTestsComplete = false;
  waitingBetweenTests = false;
  pauseStartTime = 0;
  for (int i = 0; i < 4; i++) {
    targetTimes[i] = 0;
  }
  
  // Start with first target RPM
  targetRPM = RPM_TARGETS[0];
  
  // Record start time for the 2-second pause before first test
  testStartTime = millis();
  
  Serial.println("=== MULTI-TEST ACCELERATION SEQUENCE STARTED ===");
  Serial.println("Tests: 0->15k, 0->16k, 0->17k, 0->18k RPM");
  Serial.println("Each test: 2s pause + 0% -> 100% -> target RPM");
  Serial.print("Test 1/4: Target ");
  Serial.print(targetRPM);
  Serial.println(" RPM");
  Serial.println("Motor: Stopping for 2s...");
}

bool MotorController::isAccelerationTestRunning() {
  return accelerationTestActive;
}

void MotorController::updateAccelerationTest() {
  if (!accelerationTestActive) return;
  
  unsigned long currentTime = millis();
  
  // Handle pause before any test (including first test)
  if (waitingBetweenTests || (currentTestIndex == 0 && pauseStartTime == 0)) {
    // Initialize pause timing for first test if not set
    if (currentTestIndex == 0 && pauseStartTime == 0) {
      pauseStartTime = currentTime;
      waitingBetweenTests = true;
    }
    
    if (currentTime - pauseStartTime < 2000) {
      return; // Still in 2-second pause
    }
    
    // Pause complete, start the motor for current test
    waitingBetweenTests = false;
    
    // CRITICAL: Reset RPM counter to ensure clean start
    
    setSpeed(100);
    testStartTime = currentTime; // Reset timer for acceleration measurement
    
    // Start the acceleration test timing in RPM counter
    RPMCounter::startAccelerationTest();
    
    Serial.print("Starting test ");
    Serial.print(currentTestIndex + 1);
    Serial.print("/4: 0% -> 100% -> ");
    Serial.print(targetRPM);
    Serial.println(" RPM");
    return;
  }
  
  // Get current RPM from RPMCounter (using acceleration test method)
  float currentRPM = RPMCounter::getAccelerationRPM();
  
  // Check if we've reached the current target RPM
  if (!targetRPMReached && currentRPM >= targetRPM) {
    unsigned long accelerationTime = currentTime - testStartTime;
    targetTimes[currentTestIndex] = accelerationTime;
    targetRPMReached = true;
    
    Serial.print("✓ Test ");
    Serial.print(currentTestIndex + 1);
    Serial.print("/4 complete: 0 -> ");
    Serial.print(targetRPM);
    Serial.print(" RPM in ");
    Serial.print(accelerationTime);
    Serial.print(" ms. (Measured: ");
    Serial.print(currentRPM);
    Serial.println(" RPM)");

    
    // Stop motor and prepare for next test
    stop();
    
    // Move to next test
    currentTestIndex++;
    
    if (currentTestIndex < 4) {
      // Prepare next test
      targetRPM = RPM_TARGETS[currentTestIndex];
      targetRPMReached = false;
      waitingBetweenTests = true;
      pauseStartTime = currentTime;
      
      Serial.print("Next test ");
      Serial.print(currentTestIndex + 1);
      Serial.print("/4: Target ");
      Serial.print(targetRPM);
      Serial.println(" RPM");
      Serial.println("Motor: Stopping for 2s...");
    } else {
      // All tests complete
      allTestsComplete = true;
      accelerationTestActive = false;
      
      Serial.println("=== ALL ACCELERATION TESTS COMPLETE ===");
      for (int i = 0; i < 4; i++) {
        Serial.print("Test ");
        Serial.print(i + 1);
        Serial.print(" (0 -> ");
        Serial.print(RPM_TARGETS[i]);
        Serial.print(" RPM): ");
        if (targetTimes[i] > 0) {
          Serial.print(targetTimes[i]);
          Serial.println(" ms");
        } else {
          Serial.println("Failed");
        }
      }
      Serial.println("============================================");
    }
  }
  
  // Timeout for individual test (10 seconds acceleration)
  if (!waitingBetweenTests && currentSpeed > 0 && currentTime - testStartTime > 10000) {
    float timeoutRPM = RPMCounter::getAccelerationRPM();
    Serial.print("✗ Test ");
    Serial.print(currentTestIndex + 1);
    Serial.print("/4 timeout: Max RPM ");
    Serial.println(timeoutRPM);
    
    // Stop motor and move to next test
    stop();
    targetTimes[currentTestIndex] = 0; // Mark as failed
    currentTestIndex++;
    
    if (currentTestIndex < 4) {
      targetRPM = RPM_TARGETS[currentTestIndex];
      targetRPMReached = false;
      waitingBetweenTests = true;
      pauseStartTime = currentTime;
      
      Serial.print("Next test ");
      Serial.print(currentTestIndex + 1);
      Serial.print("/4: Target ");
      Serial.print(targetRPM);
      Serial.println(" RPM");
    } else {
      accelerationTestActive = false;
      
      Serial.println("=== ALL TESTS COMPLETE (WITH TIMEOUTS) ===");
      for (int i = 0; i < 4; i++) {
        Serial.print("Test ");
        Serial.print(i + 1);
        Serial.print(" (0 -> ");
        Serial.print(RPM_TARGETS[i]);
        Serial.print(" RPM): ");
        if (targetTimes[i] > 0) {
          Serial.print(targetTimes[i]);
          Serial.println(" ms");
        } else {
          Serial.println("Failed/Timeout");
        }
      }
    }
  }
}

unsigned long MotorController::getAccelerationTestResult() {
  if (testCompletionTime > 0) {
    return testCompletionTime - testStartTime;
  }
  return 0; // Test not complete or failed
}