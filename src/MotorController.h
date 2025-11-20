#ifndef MOTOR_TESTER_MOTOR_CONTROLLER_H
#define MOTOR_TESTER_MOTOR_CONTROLLER_H

#include <Arduino.h>

class MotorController {
  public:
    static void begin();
    static void setSpeed(int percentage); // 0-100%
    static void stop();
    static int getCurrentSpeed();
    static bool isRunning();
    static unsigned long getLastUpdateTime();
    
    // Acceleration test functionality
    static void startAccelerationTest();
    static bool isAccelerationTestRunning();
    static void updateAccelerationTest(); // Call in main loop
    static unsigned long getAccelerationTestResult(); // Returns time in ms, 0 if test not complete
    
    // Predefined speed levels
    static const int SPEED_OFF = 0;
    static const int SPEED_25 = 25;
    static const int SPEED_50 = 50;
    static const int SPEED_75 = 75;
    static const int SPEED_100 = 100;
    
  private:
    // L298N pin definitions
    static const int IN3_PIN = D1;  // Direction control 1
    static const int IN4_PIN = D2;  // Direction control 2
    static const int ENB_PIN = D3;  // PWM speed control (Enable B)
    
    static int currentSpeed;
    static bool motorRunning;
    static unsigned long lastUpdateTime;
    
    // Acceleration test variables
    static bool accelerationTestActive;
    static unsigned long testStartTime;
    static unsigned long testCompletionTime;
    static float targetRPM;
    static bool targetRPMReached;
    
    // Multi-test sequence variables
    static const float RPM_TARGETS[4]; // Array of target RPMs
    static int currentTestIndex;
    static unsigned long targetTimes[4]; // Store time for each target
    static bool allTestsComplete;
    static bool waitingBetweenTests;
    static unsigned long pauseStartTime;
    
    static void updateMotor();
    static int speedToPWM(int percentage);
};

#endif