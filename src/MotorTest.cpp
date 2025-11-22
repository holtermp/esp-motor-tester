#include "MotorTest.h"
#include "RPMCounter.h"
#include "MotorController.h"    

MotorTest::MotorTest() {
    running = false;
    sampleIndex = 0;
}

void MotorTest::start(u_short testType) {
    this->testType = testType;
    running = true;
    sampleIndex = 0;
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
        String json = "{\"samples\":[";
        for (u_short i = 0; i < sampleIndex; i++) {
            json += String(samples[i]);
            if (i < sampleIndex - 1) {
                json += ",";
            }
        }
        json += "]}";
        return json;
    }
    return "Test still running or no data available";
}