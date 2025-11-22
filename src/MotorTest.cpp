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
        // Calculate time-to-top-RPM using different methods  
        u_short timeToTop_Threshold = findTimeToTopRPM_ThresholdMethod();
        u_short timeToTop_MovingAvg = findTimeToTopRPM_MovingAverageMethod();
        u_short timeToTop_Settling = findTimeToTopRPM_SettlingMethod();
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
        json += "\"timeToTopRPM_ms\":{";
        json += "\"thresholdMethod\":" + String(timeToTop_Threshold * actualSampleIntervalMs, 1) + ",";
        json += "\"movingAverageMethod\":" + String(timeToTop_MovingAvg * actualSampleIntervalMs, 1) + ",";
        json += "\"settlingMethod\":" + String(timeToTop_Settling * actualSampleIntervalMs, 1);
        json += "},";
        json += "\"parameters\":{";
        json += "\"thresholdPercent\":" + String(TOP_RPM_THRESHOLD_PERCENT) + ",";
        json += "\"settlingTolerance\":" + String(SETTLING_TOLERANCE_PERCENT) + ",";
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

// Method 1: Threshold Method - Find when RPM first reaches X% of max
u_short MotorTest::findTimeToTopRPM_ThresholdMethod() {
    float maxRPM = findMaxRPM();
    float threshold = maxRPM * (TOP_RPM_THRESHOLD_PERCENT / 100.0);
    
    for (u_short i = 0; i < sampleIndex; i++) {
        if (samples[i] >= threshold) {
            return i; // Return sample index (time = index * SLEEP_BETWEEN_SAMPLES_MS)
        }
    }
    return sampleIndex - 1; // If never reached, return last sample
}

// Method 2: Moving Average Method - Find when moving average reaches threshold
u_short MotorTest::findTimeToTopRPM_MovingAverageMethod() {
    float maxRPM = findMaxRPM();
    float threshold = maxRPM * (TOP_RPM_THRESHOLD_PERCENT / 100.0);
    
    for (u_short i = MOVING_AVERAGE_WINDOW - 1; i < sampleIndex; i++) {
        float movingAvg = calculateMovingAverage(i, MOVING_AVERAGE_WINDOW);
        if (movingAvg >= threshold) {
            return i;
        }
    }
    return sampleIndex - 1;
}

// Method 3: Settling Method - Find when RPM stays within tolerance for consecutive samples
u_short MotorTest::findTimeToTopRPM_SettlingMethod() {
    float maxRPM = findMaxRPM();
    float targetRPM = maxRPM * (TOP_RPM_THRESHOLD_PERCENT / 100.0);
    float tolerance = targetRPM * (SETTLING_TOLERANCE_PERCENT / 100.0);
    
    u_short consecutiveCount = 0;
    
    for (u_short i = 0; i < sampleIndex; i++) {
        float movingAvg = calculateMovingAverage(i, min(MOVING_AVERAGE_WINDOW, i + 1));
        
        // Check if within tolerance
        if (abs(movingAvg - targetRPM) <= tolerance) {
            consecutiveCount++;
            if (consecutiveCount >= SETTLING_WINDOW) {
                return i - SETTLING_WINDOW + 1; // Return when settling started
            }
        } else {
            consecutiveCount = 0; // Reset counter if out of tolerance
        }
    }
    return sampleIndex - 1; // If never settled
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