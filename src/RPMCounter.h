#ifndef RPM_COUNTER_H
#define RPM_COUNTER_H

#include <Arduino.h>

#define BUFFER_SIZE 5 // Number of timestamps to store for RPM calculation, do not choose too large, as it will smear rapid changes
#define RPM_SENSOR_PIN D6 //D6 = D0, D7=A0 on Opto Breakout Board

class RPMCounter {
public:
    static void begin();
    static void update(); // Call this regularly in loop() to process pending signals
    
    // ISR function - must be public and static for interrupt attachment
    static void IRAM_ATTR handleSignal();
    
    // Getters for RPM data
    static unsigned long getSignalCount();
    static unsigned long getLastSignalTime();
    static bool hasPendingSignal();
    static float getCurrentRPM(); // Calculate current RPM based on recent signals
    static unsigned long getTimeBetweenSignals(); // Get last interval in microseconds
    
private:
    static volatile unsigned long signalTimestampsWork[BUFFER_SIZE];
    static volatile unsigned long signalTimestampsCopy[BUFFER_SIZE];
    static unsigned int timestampIndex;

    static volatile bool signalPending;
    static volatile unsigned long lastOutputTime;
    
    static volatile unsigned long signalCount;
    static volatile unsigned long lastSignalTime;
    static volatile unsigned long blockingTimestamp;
    static volatile float currentRPM; // Store current RPM calculation
    static volatile unsigned long lastIntervalMicros; // Time between last two signals in microseconds
    static uint8_t sensorPin;
    
    // Simple two-timestamp approach - capture in ISR, process in main thread
    static volatile unsigned long currentTimestamp;
    static volatile unsigned long previousTimestamp;
    static volatile bool timestampReady;
    
    // Minimum time between valid signals (debouncing)
    static const unsigned long DEBOUNCE_TIME_US = 100; // 10μs in microseconds
    
    // Maximum reasonable RPM to filter out erroneous readings
    static constexpr float MAX_REASONABLE_RPM = 60000.0; // Reject readings above 25k RPM
    
};

#endif