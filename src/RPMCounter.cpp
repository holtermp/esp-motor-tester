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
volatile unsigned long RPMCounter::accelerationTestStartTime = 0;
volatile bool RPMCounter::accelerationTestActive = false;
volatile unsigned long RPMCounter::risingEdgeTime = 0;
volatile unsigned long RPMCounter::fallingEdgeTime = 0;
volatile bool RPMCounter::risingEdgeDetected = false;
volatile unsigned long RPMCounter::lastValidSignalLength = 0;
volatile unsigned long RPMCounter::consistentSignalCount = 0;

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
    accelerationTestStartTime = 0;
    accelerationTestActive = false;
    risingEdgeTime = 0;
    fallingEdgeTime = 0;
    risingEdgeDetected = false;
    
    // Configure pin as input with pull-up resistor
    pinMode(pin, INPUT_PULLUP);
    
    // Attach single interrupt to handle both edges
    attachInterrupt(digitalPinToInterrupt(pin), handleSignalChange, CHANGE);
    
    Serial.print("RPM Counter initialized on pin D");
    Serial.print(pin);
    Serial.println(" with dual-edge signal length filtering (600μs - 1.4ms)");
}

void RPMCounter::reset() {
    // Reset all volatile variables safely
    signalPending = false;
    lastSignalTime = 0;
    blockingTimestamp = 0;
    currentTimestamp = 0;
    previousTimestamp = 0;
    timestampReady = false;
    currentRPM = 0.0;
    lastIntervalMicros = 0;
    accelerationTestStartTime = 0;
    accelerationTestActive = false;
    risingEdgeTime = 0;
    fallingEdgeTime = 0;
    risingEdgeDetected = false;
    lastValidSignalLength = 0;
    consistentSignalCount = 0;
    
    // Note: we don't reset signalCount to preserve total count
    
    Serial.println("RPM Counter reset - all values cleared");
}

void IRAM_ATTR RPMCounter::handleSignalChange() {
    unsigned long now = micros();
    bool pinState = digitalRead(sensorPin);  // Read current pin state
    
    // Simple debounce: ignore signals that come too quickly
    if (now - blockingTimestamp < DEBOUNCE_TIME_US) {
        return;
    }
    
    if (pinState == HIGH) {
        // Rising edge detected
        risingEdgeTime = now;
        risingEdgeDetected = true;
    } else {
        // Falling edge detected
        fallingEdgeTime = now;
        
        // Only process falling edge if we have a valid rising edge
        if (!risingEdgeDetected) {
            return;
        }
        
        // Calculate signal length
        unsigned long signalLength = fallingEdgeTime - risingEdgeTime;
        
        // Filter by signal length - reject obvious noise
        if (signalLength < MIN_SIGNAL_LENGTH_US || signalLength > MAX_SIGNAL_LENGTH_US) {
            risingEdgeDetected = false;
            return;
        }
        
        // Additional consistency check: reject signals that vary too much from recent signals
        // This helps filter out multiple apertures or reflections
        if (lastValidSignalLength > 0) {
            unsigned long lengthDiff = (signalLength > lastValidSignalLength) ? 
                                     (signalLength - lastValidSignalLength) : 
                                     (lastValidSignalLength - signalLength);
            
            // Reject if signal length differs by more than 50% from the last valid signal
            if (lengthDiff > (lastValidSignalLength / 2)) {
                risingEdgeDetected = false;
                return;
            }
        }
        
        // Valid signal - update tracking
        lastValidSignalLength = signalLength;
        consistentSignalCount++;
        
        // Valid signal - update timestamps for RPM calculation
        blockingTimestamp = now;
        previousTimestamp = currentTimestamp;
        currentTimestamp = risingEdgeTime; // Use rising edge for timing consistency
        
        signalCount++;
        signalPending = true;
        risingEdgeDetected = false;
        
        // Mark that we have two timestamps ready for calculation
        if (previousTimestamp > 0) {
            timestampReady = true;
        }
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
                            lastIntervalMicros = interval;
                            currentRPM = calculatedRPM;                     
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

void RPMCounter::startAccelerationTest() {
    accelerationTestStartTime = micros();
    accelerationTestActive = true;
    Serial.println("Acceleration test timing started");
}

float RPMCounter::getAccelerationRPM() {
    if (!accelerationTestActive) {
        return 0.0; // No test running
    }
    
    // Use the real-time RPM calculation instead of trying to calculate from total count
    return getCurrentRPM();
}