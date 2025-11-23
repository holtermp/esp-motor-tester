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
volatile unsigned long RPMCounter::risingEdgeTime = 0;
volatile unsigned long RPMCounter::fallingEdgeTime = 0;
volatile bool RPMCounter::risingEdgeDetected = false;
volatile unsigned long RPMCounter::lastValidSignalLength = 0;
volatile unsigned long RPMCounter::lastOutputTime = 0;
volatile unsigned long RPMCounter::consistentSignalCount = 0;
unsigned int RPMCounter::timestampIndex = 0;
volatile unsigned long RPMCounter::signalTimestampsWork[BUFFER_SIZE] = {0};
volatile unsigned long RPMCounter::signalTimestampsRead[BUFFER_SIZE] = {0};

void RPMCounter::begin(uint8_t pin)
{
    sensorPin = pin;
    signalPending = false;
    signalCount = 0;
    lastSignalTime = 0;
    currentRPM = 0.0;
    lastOutputTime = 0;

    timestampIndex = 0;
    for (unsigned int i = 0; i < BUFFER_SIZE; i++)
        signalTimestampsWork[i] = 0;

    // Configure pin as input with pull-up resistor
    pinMode(pin, INPUT_PULLUP);

    // Attach single interrupt to handle both edges
    attachInterrupt(digitalPinToInterrupt(pin), handleSignal, FALLING);

    Serial.print("RPM Counter initialized on pin D");
    Serial.print(pin);
}

void IRAM_ATTR RPMCounter::handleSignal()
{
    unsigned long now = micros();

    // Simple debounce: ignore signals that come too quickly
    if (now - blockingTimestamp < DEBOUNCE_TIME_US)
    {
        return;
    }
    // Valid signal - update timestamps for RPM calculation
    blockingTimestamp = now;

    signalTimestampsWork[timestampIndex] = now;
    timestampIndex = (timestampIndex + 1) % BUFFER_SIZE;
}

void RPMCounter::update()
{
    unsigned long earliest = 0;
    unsigned long latest = 0;
    unsigned long now = micros();
    memcpy(
        (void *)&signalTimestampsRead[0],
        (const void *)&signalTimestampsWork[0],
        BUFFER_SIZE * sizeof(unsigned long));
    for (unsigned int i = 0; i < BUFFER_SIZE; i++)
    {
        unsigned long ts = signalTimestampsRead[i];
        if (ts == 0)
            continue; // Skip uninitialized entries
        if (earliest == 0 || ts < earliest)
            earliest = ts;
        if (ts > latest)
            latest = ts;
    }
    if (earliest != 0 && latest != earliest)
    {
        unsigned long timeDiff = latest - earliest;
        unsigned int signalCountLocal = 0;
        for (unsigned int i = 0; i < BUFFER_SIZE; i++)
        {
            if (signalTimestampsRead[i] != 0)
                signalCountLocal++;
        }
        if (signalCountLocal < 2)
            return; // Need at least two signals to calculate RPM
        float rpm = (signalCountLocal - 1) * 60000000.0f / timeDiff;
        // rpm=0 if values too old
        if (now - latest > 1000UL * 1000UL)
        {
            for (unsigned int i = 0; i < BUFFER_SIZE; i++)
                signalTimestampsWork[i] = 0;
            currentRPM = 0.0;
        }
        // Filter out unreasonable RPM readings
        else if (rpm <= MAX_REASONABLE_RPM)
        {
            currentRPM = rpm;
            lastIntervalMicros = (latest - earliest) / (signalCountLocal - 1);
            lastSignalTime = now;
            signalPending = true;
        }
        if ((lastOutputTime == 0 || now - lastOutputTime >= 1000UL * 1000UL) && currentRPM > 0.0)
        {
            Serial.print("RPM: ");
            Serial.print(currentRPM);
            Serial.print(" | ");
            Serial.print(signalCountLocal);
            Serial.print(" signals counted in ");
            Serial.print(latest - earliest);
            Serial.println(" us");

            lastOutputTime = now;
        }
    }
}

unsigned long RPMCounter::getSignalCount()
{
    return signalCount;
}

unsigned long RPMCounter::getLastSignalTime()
{
    return lastSignalTime;
}

bool RPMCounter::hasPendingSignal()
{
    return signalPending;
}

float RPMCounter::getCurrentRPM()
{
    // Check if the reading is too old (more than 2 seconds = motor likely stopped)
    if (millis() - lastSignalTime > 2000)
    {
  //      return 0.0;
    }

    return currentRPM;
}

unsigned long RPMCounter::getTimeBetweenSignals()
{
    return lastIntervalMicros;
}