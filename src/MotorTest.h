#ifndef MOTOR_TEST_H
#define MOTOR_TEST_H

#include <Arduino.h>

#define TEST_TYPE_ACCELERATION 1
#define TEST_TYPE_DECELERATION 2

#define NUM_SAMPLES 100
#define SLEEP_BETWEEN_SAMPLES_MS 10
#define NUM_TEST_RUNS 5
#define SLEEP_BETWEEN_TEST_RUNS_MS 2000

// Analysis parameters for time-to-top-RPM calculation
#define MOVING_AVERAGE_WINDOW 5     // Number of samples for moving average



class MotorTest {
private:
    bool running;
    u_short testType;
    float temp_samples[NUM_TEST_RUNS][NUM_SAMPLES];
    float samples[NUM_SAMPLES];
    u_short sampleIndex = 0;
    u_short testRunIndex = 0;
    
    
    // Timing measurement for accurate frequency calculation
    unsigned long testStartTime;
    unsigned long testEndTime;
    float actualSampleFrequency;
    
    // Analysis methods
    float calculateMovingAverage(u_short index, u_short window);
    float findMaxRPM();
    u_short findTimeToTopRPM_MovingAverageMethod();
    float calculateActualSampleFrequency();
    
public:
    MotorTest();
    
    void start(u_short testType);
    void update();
    bool isRunning();
    String getResult();
};

#endif // MOTOR_TEST_H