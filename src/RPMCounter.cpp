#include "RPMCounter.h"

// Static member definitions
volatile bool RPMCounter::signalPending = false;
volatile unsigned long RPMCounter::signalCount = 0;
volatile unsigned long RPMCounter::lastSignalTime = 0;
volatile unsigned long RPMCounter::blockingTimestamp = 0;
volatile float RPMCounter::currentRPM = 0.0;
volatile unsigned long RPMCounter::lastIntervalMicros = 0;
uint8_t RPMCounter::sensorPin = 0;
volatile unsigned long RPMCounter::currentTimestamp = 0;
volatile unsigned long RPMCounter::previousTimestamp = 0;
volatile bool RPMCounter::timestampReady = false;

void RPMCounter::begin(uint8_t pin) {
    sensorPin = pin;
    signalPending = false;
    signalCount = 0;
    lastSignalTime = 0;
    blockingTimestamp = 0;
    currentTimestamp = 0;
    previousTimestamp = 0;
    timestampReady = false;
    currentRPM = 0.0;
    lastIntervalMicros = 0;
    
    // Configure pin as input with pull-up resistor
    pinMode(pin, INPUT_PULLUP);
    
    // Attach interrupt on rising edge (signal goes from LOW to HIGH)
    attachInterrupt(digitalPinToInterrupt(pin), handleSignal, RISING);
    
    Serial.print("RPM Counter initialized on pin D");
    Serial.print(pin);
    Serial.println(" with 1ms debounce and bounds checking (10-25000 RPM)");
}

void IRAM_ATTR RPMCounter::handleSignal() {
    unsigned long now = micros();
    
    // Simple debounce: ignore signals that come too quickly
    if (now - blockingTimestamp < DEBOUNCE_TIME_US) {
        return;
    }
    
    blockingTimestamp = now;
    
    // Just capture timestamps - do NO calculations in ISR
    previousTimestamp = currentTimestamp;
    currentTimestamp = now;
    
    signalCount++;
    signalPending = true;
    
    // Mark that we have two timestamps ready for calculation
    if (previousTimestamp > 0) {
        timestampReady = true;
    }
}

void RPMCounter::update() {
    // Check if there's a pending signal to process
    if (signalPending) {
        signalPending = false;
        
        // Update lastSignalTime here (safe to call millis() outside ISR)
        lastSignalTime = millis();
        
        // Calculate RPM if we have two timestamps
        if (timestampReady) {
            // Create local copies to avoid race conditions
            unsigned long current = currentTimestamp;
            unsigned long previous = previousTimestamp;
            
            if (current > previous) { // Sanity check for timer overflow
                unsigned long interval = current - previous;
                
                // Sanity check: interval must be reasonable
                // Min interval: 25000 RPM = 417 RPS = ~2400 microseconds
                // Max interval: 10 RPM = 0.167 RPS = 6,000,000 microseconds  
                if (interval >= 2400 && interval <= 6000000) {
                    // Calculate RPM: 60,000,000 microseconds = 1 minute
                    float calculatedRPM = 60000000.0 / interval;
                    
                    // Apply bounds checking to filter out erroneous readings
                    if (calculatedRPM >= MIN_REASONABLE_RPM && calculatedRPM <= MAX_REASONABLE_RPM) {
                        // Smart noise filtering: allow larger changes if motor speed might have changed
                        bool acceptReading = false;
                        
                        if (currentRPM == 0.0) {
                            // Always accept first reading
                            acceptReading = true;
                        } else {
                            // Calculate percentage difference
                            float percentDiff = abs(calculatedRPM - currentRPM) / currentRPM;
                            
                            // Accept if change is reasonable OR if we have multiple consistent readings
                            static float lastCalculatedRPM = 0.0;
                            static int consecutiveReadings = 0;
                            
                            if (percentDiff < 0.3) { // Accept changes up to 30%
                                acceptReading = true;
                                consecutiveReadings = 0;
                            } else if (abs(calculatedRPM - lastCalculatedRPM) / calculatedRPM < 0.1) {
                                // Two consecutive similar readings that differ from current - likely real change
                                consecutiveReadings++;
                                if (consecutiveReadings >= 2) {
                                    acceptReading = true;
                                    consecutiveReadings = 0;
                                }
                            } else {
                                consecutiveReadings = 0;
                            }
                            
                            lastCalculatedRPM = calculatedRPM;
                        }
                        
                        if (acceptReading) {
                            lastIntervalMicros = interval;
                            currentRPM = calculatedRPM;
                        }
                        // Else: reject this reading as likely noise, keep previous value
                    }
                }
            }
            
            timestampReady = false; // Reset flag
        }
        
        // Only print occasionally to avoid slowing down the system
        static unsigned long lastPrintTime = 0;
        if (millis() - lastPrintTime > 1000) { // Print max once per second
            Serial.print("RPM: ");
            Serial.print(currentRPM, 1);
            Serial.print(" (Count: ");
            Serial.print(signalCount);
            Serial.print(", Interval: ");
            Serial.print(lastIntervalMicros / 1000.0, 2); // Convert to ms with 2 decimal places
            Serial.println(" ms)");
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