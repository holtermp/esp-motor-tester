#ifndef RPM_COUNTER_H
#define RPM_COUNTER_H

#include <Arduino.h>

class RPMCounter {
public:
    static void begin(uint8_t pin);
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
    static volatile bool signalPending;
    static volatile unsigned long signalCount;
    static volatile unsigned long lastSignalTime;
    static volatile unsigned long blockingTimestamp;
    static volatile float currentRPM; // Store current RPM calculation
    static volatile unsigned long lastIntervalMicros; // Time between last two signals in microseconds
    static uint8_t sensorPin;
    static unsigned long previousSignalTimeMicros; // For RPM calculation in microseconds
    
    // Minimum time between valid signals (debouncing)
    // For 21,000 RPM max = 350 RPS = 2.86ms between signals
    // Set debounce to 0.5ms to handle electrical noise while allowing high speeds
    static const unsigned long DEBOUNCE_TIME_US = 500; // 0.5ms in microseconds
};

#endif