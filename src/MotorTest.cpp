#include "MotorTest.h"
#include "RPMCounter.h"
#include "MotorController.h"    

MotorTest::MotorTest() {
    running = false;
    sampleIndex = 0;
    testStartTime = 0;
    testEndTime = 0;
    actualSampleFrequency = 0.0;
}

void MotorTest::start(u_short testType) {
    this->testType = testType;
    running = true;
    sampleIndex = 0;
    testStartTime = millis();  // Record start time
    testEndTime = 0;
    MotorController::setSpeed(MotorController::SPEED_100);
}


//run this in the main loop without blocking while isRunning() is true
void MotorTest::update() {
    if (testType == TEST_TYPE_ACCELERATION) {
        if (sampleIndex < NUM_SAMPLES) {
            samples[sampleIndex] = RPMCounter::getCurrentRPM();
            sampleIndex++;
            delay(SLEEP_BETWEEN_SAMPLES_MS);
        } else {
            testEndTime = millis();  // Record end time
            actualSampleFrequency = calculateActualSampleFrequency();
            MotorController::setSpeed(MotorController::SPEED_OFF);
            running = false;
        }
    }
}

bool MotorTest::isRunning() {
    return running;        
}

String MotorTest::getResult() {
    if (!running && testType == TEST_TYPE_ACCELERATION) {
        // Calculate time-to-top-RPM using moving average method only
        u_short timeToTop_MovingAvg = findTimeToTopRPM_MovingAverageMethod();
        float maxRPM = findMaxRPM();
        
        // Calculate actual sample interval based on measured frequency
        float actualSampleIntervalMs = (actualSampleFrequency > 0) ? (1000.0 / actualSampleFrequency) : SLEEP_BETWEEN_SAMPLES_MS;
        
        String json = "{";
        json += "\"samples\":[";
        for (u_short i = 0; i < sampleIndex; i++) {
            json += String(samples[i]);
            if (i < sampleIndex - 1) {
                json += ",";
            }
        }
        json += "],";
        json += "\"timing\":{";
        json += "\"testDurationMs\":" + String(testEndTime - testStartTime) + ",";
        json += "\"sampleCount\":" + String(sampleIndex) + ",";
        json += "\"actualSampleFrequencyHz\":" + String(actualSampleFrequency, 3) + ",";
        json += "\"actualSampleIntervalMs\":" + String(actualSampleIntervalMs, 2) + ",";
        json += "\"targetSampleIntervalMs\":" + String(SLEEP_BETWEEN_SAMPLES_MS);
        json += "},";
        json += "\"analysis\":{";
        json += "\"maxRPM\":" + String(maxRPM, 1) + ",";
        json += "\"timeToTopRPM_ms\":" + String(timeToTop_MovingAvg * actualSampleIntervalMs, 1) + ",";
        json += "\"parameters\":{";
        json += "\"movingAverageWindow\":" + String(MOVING_AVERAGE_WINDOW);
        json += "}";
        json += "}";
        json += "}";
        return json;
    }
    return "Test still running or no data available";
}

// Calculate moving average for noise reduction
float MotorTest::calculateMovingAverage(u_short index, u_short window) {
    if (index < window - 1) return samples[index]; // Not enough data for moving average
    
    float sum = 0;
    for (u_short i = index - window + 1; i <= index; i++) {
        sum += samples[i];
    }
    return sum / window;
}

// Find the maximum RPM value in the dataset
float MotorTest::findMaxRPM() {
    float maxRPM = 0;
    for (u_short i = 0; i < sampleIndex; i++) {
        if (samples[i] > maxRPM) {
            maxRPM = samples[i];
        }
    }
    return maxRPM;
}

// Moving Average Method - Find when moving average reaches close to max RPM
u_short MotorTest::findTimeToTopRPM_MovingAverageMethod() {
    float maxRPM = findMaxRPM();
    float targetRPM = maxRPM * 0.95; // Look for when we reach 95% of max RPM
    
    for (u_short i = MOVING_AVERAGE_WINDOW - 1; i < sampleIndex; i++) {
        float movingAvg = calculateMovingAverage(i, MOVING_AVERAGE_WINDOW);
        if (movingAvg >= targetRPM) {
            return i;
        }
    }
    return sampleIndex - 1;
}

// Calculate the actual sample frequency based on measured test duration
float MotorTest::calculateActualSampleFrequency() {
    if (testEndTime > testStartTime && sampleIndex > 1) {
        float testDurationMs = testEndTime - testStartTime;
        float testDurationSec = testDurationMs / 1000.0;
        // Frequency = (samples - 1) / duration (since N samples means N-1 intervals)
        return (sampleIndex - 1) / testDurationSec;
    }
    return 0.0; // Invalid or no data
}