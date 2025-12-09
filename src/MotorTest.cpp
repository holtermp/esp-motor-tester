#include "MotorTest.h"
#include "RPMCounter.h"
#include "MotorController.h"

MotorTest::MotorTest()
{
    running = false;
    sampleIndex = 0;
    testStartTime = 0;
    testEndTime = 0;
    actualSampleFrequency = 0.0;
}

void MotorTest::start(u_short testType)
{
    this->testType = testType;
    running = true;
    testRunIndex = 0;
    sampleIndex = 0;
    testStartTime = millis(); // Record start time
    testEndTime = 0;
    MotorController::setSpeed(MotorController::SPEED_100);
}

void MotorTest::startTestRun()
{
    RPMCounter::clear();
    sampleIndex = 0;
    testStartTime = millis(); // Record start time
    testEndTime = 0;
    MotorController::setSpeed(MotorController::SPEED_100);
}

// run this in the main loop without blocking while isRunning() is true
void MotorTest::update()
{
    if (testType == TEST_TYPE_ACCELERATION)
    {
        if (testRunIndex < NUM_TEST_RUNS)
        {
            if (sampleIndex == 0)
            {
                startTestRun();
            }
            if (sampleIndex < NUM_SAMPLES)
            {
                temp_samples[testRunIndex][sampleIndex] = RPMCounter::getCurrentRPM();
                sampleIndex++;
                delay(SLEEP_BETWEEN_SAMPLES_MS);
            }
            else
            {
                testEndTime = millis(); // Record end time
                actualSampleFrequency = calculateActualSampleFrequency();
                MotorController::setSpeed(MotorController::SPEED_OFF);
                testRunIndex++;
                if (testRunIndex < NUM_TEST_RUNS)
                {
                    delay(SLEEP_BETWEEN_TEST_RUNS_MS);
                }
                sampleIndex = 0; // Reset for next run
            }
        }
        else
            running = false;
    }
}

bool MotorTest::isRunning()
{
    return running;
}

String MotorTest::getResult()
{
    if (!running && testType == TEST_TYPE_ACCELERATION)
    {
        // Average samples across test runs
        for (u_short i = 0; i < NUM_SAMPLES; i++)
        {
            float sum = 0.0;
            for (u_short run = 0; run < NUM_TEST_RUNS; run++)
            {
                sum += temp_samples[run][i];
            }
            samples[i] = sum / NUM_TEST_RUNS;
        }
        
        // Apply outlier filtering: copy original data first, then filter in-place
        for (u_short i = 0; i < NUM_SAMPLES; i++) {
            filteredSamples[i] = samples[i];  // Copy original data
        }
        removeOutliers(filteredSamples, NUM_SAMPLES);  // Apply spike filtering
        
        // Calculate time-to-top-RPM using moving average method only
        u_short topRpmIndex = avgTopRpmReachedIdx();
        float maxRPM = samples[topRpmIndex];
        
        // Calculate actual sample interval based on measured frequency
        float actualSampleIntervalMs = (actualSampleFrequency > 0) ? (1000.0 / actualSampleFrequency) : SLEEP_BETWEEN_SAMPLES_MS;
        
        float timeToMaxRpm = topRpmIndex*actualSampleIntervalMs;

        String json = "{";
        json += "\"samples\":[";
        for (u_short i = 0; i < NUM_SAMPLES; i++)
        {
            json += String(samples[i]);
            if (i < NUM_SAMPLES - 1)
            {
                json += ",";
            }
        }
        json += "],";
        json += "\"filteredSamples\":[";
        for (u_short i = 0; i < NUM_SAMPLES; i++)
        {
            json += String(filteredSamples[i]);
            if (i < NUM_SAMPLES - 1)
            {
                json += ",";
            }
        }
        json += "],";
        json += "\"timing\":{";
        json += "\"testDurationMs\":" + String(testEndTime - testStartTime) + ",";
        json += "\"sampleCount\":" + String(NUM_SAMPLES) + ",";
        json += "\"actualSampleFrequencyHz\":" + String(actualSampleFrequency, 3) + ",";
        json += "\"actualSampleIntervalMs\":" + String(actualSampleIntervalMs, 2) + ",";
        json += "\"targetSampleIntervalMs\":" + String(SLEEP_BETWEEN_SAMPLES_MS);
        json += "},";
        json += "\"analysis\":{";
        json += "\"maxRPM\":" + String(maxRPM, 1) + ",";
        json += "\"timeToTopRPM_ms\":" + String(timeToMaxRpm) + ",";
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
float MotorTest::calculateMovingAverage(u_short index, u_short window)
{
    if (index < window - 1)
        return samples[index]; // Not enough data for moving average

    float sum = 0;
    for (u_short i = index - window + 1; i <= index; i++)
    {
        sum += samples[i];
    }
    return sum / window;
}

// Find the maximum RPM value in the dataset
float MotorTest::findMaxRPM()
{
    float maxRPM = 0;
    for (u_short i = 0; i < NUM_SAMPLES; i++)
    {
        if (samples[i] > maxRPM)
        {
            maxRPM = samples[i];
        }
    }
    return maxRPM;
}

// Moving Average Method - Find when moving average reaches close to max RPM
u_short MotorTest::avgTopRpmReachedIdx()
{
    float maxRPM = findMaxRPM();
    float targetRPM = maxRPM * 0.95; // Look for when we reach 95% of max RPM

    for (u_short i = MOVING_AVERAGE_WINDOW - 1; i < NUM_SAMPLES; i++)
    {
        float movingAvg = calculateMovingAverage(i, MOVING_AVERAGE_WINDOW);
        if (movingAvg >= targetRPM)
        {
            return i;
        }
    }
    //fallback
    return NUM_SAMPLES - 1;
}

// Calculate the actual sample frequency based on measured test duration
float MotorTest::calculateActualSampleFrequency()
{
    if (testEndTime > testStartTime )
    {
        float testDurationMs = testEndTime - testStartTime;
        float testDurationSec = testDurationMs / 1000.0;
        // Frequency = (samples - 1) / duration (since N samples means N-1 intervals)
        return (NUM_SAMPLES - 1) / testDurationSec;
    }
    return 0.0; // Invalid or no data
}

// Outlier filtering using Modified Z-Score method with MAD
void MotorTest::filterOutliers(float* inputSamples, float* outputSamples, u_short size)
{
    // Copy input to output first
    for (u_short i = 0; i < size; i++) {
        outputSamples[i] = inputSamples[i];
    }
    
    // Calculate median
    float median = calculateMedian(inputSamples, size);
    
    // Calculate MAD (Median Absolute Deviation)
    float mad = calculateMAD(inputSamples, size, median);
    
    // Replace outliers with median
    float threshold = 3.5; // Threshold for outlier detection
    for (u_short i = 0; i < size; i++) {
        if (mad > 0) {
            float modifiedZScore = 0.6745 * (inputSamples[i] - median) / mad;
            if (abs(modifiedZScore) > threshold) {
                outputSamples[i] = median; // Replace outlier with median
            }
        }
    }
}

float MotorTest::calculateMedian(float* values, u_short size)
{
    // Create a copy for sorting
    float sorted[NUM_SAMPLES];
    for (u_short i = 0; i < size; i++) {
        sorted[i] = values[i];
    }
    
    // Simple bubble sort
    for (u_short i = 0; i < size - 1; i++) {
        for (u_short j = 0; j < size - i - 1; j++) {
            if (sorted[j] > sorted[j + 1]) {
                float temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }
    
    // Return median
    if (size % 2 == 0) {
        return (sorted[size/2 - 1] + sorted[size/2]) / 2.0;
    } else {
        return sorted[size/2];
    }
}

float MotorTest::calculateMAD(float* values, u_short size, float median)
{
    float deviations[NUM_SAMPLES];
    for (u_short i = 0; i < size; i++) {
        deviations[i] = abs(values[i] - median);
    }
    return calculateMedian(deviations, size);
}

// Remove outliers using spike detection instead of aggressive statistical filtering
// This preserves the acceleration curve while removing obvious spikes
void MotorTest::removeOutliers(float* data, u_short size) {
    if (size < 5) return;
    
    // Parameters for spike detection - more aggressive
    const float SPIKE_THRESHOLD = 1.3;  // Lower threshold for better detection
    const u_short WINDOW = 1;           // Look at immediate neighbors only
    
    // Apply spike detection and removal (multiple passes)
    for (u_short pass = 0; pass < 5; pass++) {
        for (u_short i = WINDOW; i < size - WINDOW; i++) {
            float current = data[i];
            float prev = data[i-1];
            float next = data[i+1];
            float neighborAvg = (prev + next) / 2.0;
            
            // Check if current point is a significant spike
            // More aggressive: any point significantly higher than neighbors
            if (current > neighborAvg * SPIKE_THRESHOLD && neighborAvg > 1000) {
                // Replace spike with neighbor average
                data[i] = neighborAvg;
            }
            
            // Also check for extreme values (>50000 RPM are clearly wrong)
            if (current > 50000) {
                data[i] = neighborAvg;
            }
        }
    }
}