#include "RPMCounter.h"

// Static member definitions
volatile bool RPMCounter::signalPending = false;
volatile unsigned long RPMCounter::signalCount = 0;
volatile unsigned long RPMCounter::lastSignalTime = 0;
volatile unsigned long RPMCounter::blockingTimestamp = 0;
volatile float RPMCounter::currentRPM = 0.0;
volatile unsigned long RPMCounter::lastIntervalMicros = 0;
uint8_t RPMCounter::sensorPin = 0;
unsigned long RPMCounter::previousSignalTimeMicros = 0;

void RPMCounter::begin(uint8_t pin) {
    sensorPin = pin;
    signalPending = false;
    signalCount = 0;
    lastSignalTime = 0;
    blockingTimestamp = 0;
    previousSignalTimeMicros = 0;
    currentRPM = 0.0;
    lastIntervalMicros = 0;
    
    // Configure pin as input with pull-up resistor
    pinMode(pin, INPUT_PULLUP);
    
    // Attach interrupt on rising edge (signal goes from LOW to HIGH)
    attachInterrupt(digitalPinToInterrupt(pin), handleSignal, RISING);
    
    Serial.print("RPM Counter initialized on pin D");
    Serial.print(pin);
    Serial.println(" with 0.5ms debounce and microsecond precision for high-speed motors");
}

void IRAM_ATTR RPMCounter::handleSignal() {
    unsigned long now = micros(); // Use microseconds for high precision
    unsigned long timeSinceLastSignal = now - blockingTimestamp;
    
    // Debounce: ignore signals that come too quickly (electrical noise)
    if (timeSinceLastSignal > DEBOUNCE_TIME_US) {
        blockingTimestamp = now;
        
        // Calculate RPM if we have a previous signal
        if (previousSignalTimeMicros > 0) {
            lastIntervalMicros = now - previousSignalTimeMicros;
            if (lastIntervalMicros > 0) {
                // Calculate RPM using microseconds: 60,000,000 microseconds = 1 minute
                currentRPM = 60000000.0 / lastIntervalMicros;
            }
        }
        
        previousSignalTimeMicros = now;
        lastSignalTime = millis(); // Keep this in millis for display purposes
        signalCount++;
        signalPending = true;
    }
}

void RPMCounter::update() {
    // Check if there's a pending signal to process (optional logging)
    if (signalPending) {
        signalPending = false;
        
        // Only print occasionally to avoid slowing down the system
        static unsigned long lastPrintTime = 0;
        if (millis() - lastPrintTime > 1000) { // Print max once per second
            Serial.print("RPM Update - Count: ");
            Serial.print(signalCount);
            Serial.print(", Current RPM: ");
            Serial.print(currentRPM, 1);
            Serial.print(", Last interval: ");
            Serial.print(lastIntervalMicros / 1000.0, 2); // Convert to ms with 2 decimal places
            Serial.println(" ms");
            lastPrintTime = millis();
        }
    }
    
    // Check if RPM data is stale (motor stopped)
    if (millis() - lastSignalTime > 2000) {
        currentRPM = 0.0;
    }
}

unsigned long RPMCounter::getSignalCount() {
    return signalCount;
}

unsigned long RPMCounter::getLastSignalTime() {
    return lastSignalTime;
}

bool RPMCounter::hasPendingSignal() {
    return signalPending;
}

float RPMCounter::getCurrentRPM() {
    // Check if the reading is too old (more than 2 seconds = motor likely stopped)
    if (millis() - lastSignalTime > 2000) {
        return 0.0;
    }
    
    return currentRPM;
}

unsigned long RPMCounter::getTimeBetweenSignals() {
    return lastIntervalMicros;
}