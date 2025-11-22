#ifndef MOTOR_TEST_H
#define MOTOR_TEST_H

#include <Arduino.h>

#define TEST_TYPE_ACCELERATION 1
#define TEST_TYPE_DECELERATION 2

#define NUM_SAMPLES 100
#define SLEEP_BETWEEN_SAMPLES_MS 10



class MotorTest {
private:
    bool running;
    u_short testType;
    float samples[NUM_SAMPLES];
    u_short sampleIndex = 0;
public:
    MotorTest();
    
    void start(u_short testType);
    void update();
    bool isRunning();
    String getResult();
};

#endif // MOTOR_TEST_H