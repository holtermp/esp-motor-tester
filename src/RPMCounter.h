#ifndef RPM_COUNTER_H
#define RPM_COUNTER_H

#include <Arduino.h>

class RPMCounter {
public:
    static void begin(uint8_t pin);
    static void update(); // Call this regularly in loop() to process pending signals
    static void reset(); // Reset all counters and RPM values
    static void startAccelerationTest(); // Mark start time for acceleration test
    static float getAccelerationRPM(); // Get RPM based on time since test start
    
    // ISR function - must be public and static for interrupt attachment
    static void IRAM_ATTR handleSignalChange();
    
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
    
    // Signal length filtering variables
    static volatile unsigned long risingEdgeTime;
    static volatile unsigned long fallingEdgeTime;
    static volatile bool risingEdgeDetected;
    
    // Signal consistency checking to detect multiple apertures
    static volatile unsigned long lastValidSignalLength;
    static volatile unsigned long consistentSignalCount;
    
    // Signal length limits for noise filtering
    // Optical disc: 2mm aperture on 30mm diameter disc = 2.1% duty cycle
    // At 30000 RPM: 42μs HIGH, at 1000 RPM: 1.26ms HIGH
    // Tightened range based on actual measurements to filter multiple apertures
    static const unsigned long MIN_SIGNAL_LENGTH_US = 600;  // 600μs minimum (more selective)
    static const unsigned long MAX_SIGNAL_LENGTH_US = 1400; // 1.4ms maximum (more selective)
    
    // Acceleration test timing
    static volatile unsigned long accelerationTestStartTime; // Test start time in microseconds
    static volatile bool accelerationTestActive; // Flag to track if test is active
    
    // Simple two-timestamp approach - capture in ISR, process in main thread
    static volatile unsigned long currentTimestamp;
    static volatile unsigned long previousTimestamp;
    static volatile bool timestampReady;
    
    // Minimum time between valid signals (debouncing)
    // Should be much shorter than our shortest valid signal (30μs)
    // Set to 10μs to handle electrical bounce without blocking valid signals
    static const unsigned long DEBOUNCE_TIME_US = 10; // 10μs in microseconds
    
    // Maximum reasonable RPM to filter out erroneous readings
    static constexpr float MAX_REASONABLE_RPM = 25000.0; // Reject readings above 25k RPM
    static constexpr float MIN_REASONABLE_RPM = 10.0;     // Reject readings below 10 RPM
};

#endif