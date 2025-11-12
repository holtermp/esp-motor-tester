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
    
    // Simple two-timestamp approach - capture in ISR, process in main thread
    static volatile unsigned long currentTimestamp;
    static volatile unsigned long previousTimestamp;
    static volatile bool timestampReady;
    
    // Minimum time between valid signals (debouncing)
    // For 21,000 RPM max = 350 RPS = 2.86ms between signals
    // Set debounce to 1ms to handle electrical noise and web server timing issues
    static const unsigned long DEBOUNCE_TIME_US = 1000; // 1ms in microseconds
    
    // Maximum reasonable RPM to filter out erroneous readings
    static constexpr float MAX_REASONABLE_RPM = 25000.0; // Reject readings above 25k RPM
    static constexpr float MIN_REASONABLE_RPM = 10.0;     // Reject readings below 10 RPM
};

#endif