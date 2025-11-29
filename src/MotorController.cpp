#include "MotorController.h"
#include "RPMCounter.h"

// Static member definitions
int MotorController::currentSpeed = 0;
unsigned long MotorController::lastUpdateTime = 0;

void MotorController::begin()
{
  // Initialize the motor control pins
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  pinMode(ENB_PIN, OUTPUT);

  // Set PWM frequency for motor control (higher frequency for smoother operation)
  // ESP8266 default is 1000Hz, motors typically work better with 10-20kHz
  analogWriteFreq(1000); // 1kHz PWM frequency

  // Stop motor initially
  setSpeed(0);

  Serial.println("Motor Controller initialized");
  Serial.println("L298N connections:");
  Serial.println("  IN3 -> D1");
  Serial.println("  IN4 -> D2");
  Serial.println("  ENB -> D3 (PWM)");
  Serial.println("  Motor: OUT3(+) and OUT4(-)");
}

void MotorController::setSpeed(int percentage)
{
  // Clamp percentage to valid range
  percentage = constrain(percentage, 0, 100);

  currentSpeed = percentage;
  lastUpdateTime = millis();

  if (percentage == 0)
  {
    currentSpeed = 0;
    lastUpdateTime = millis();

    // Stop the motor by setting both direction pins LOW
    digitalWrite(IN3_PIN, LOW);
    digitalWrite(IN4_PIN, LOW);
    analogWrite(ENB_PIN, 0);

    Serial.println("Motor stopped");
  }
  else
  {
    // Set direction for forward rotation
    // IN3 = HIGH, IN4 = LOW for forward direction
    digitalWrite(IN3_PIN, HIGH);
    digitalWrite(IN4_PIN, LOW);

    // Set PWM speed
    int pwmValue = speedToPWM(currentSpeed);
    analogWrite(ENB_PIN, pwmValue);

    Serial.print("Motor running: ");
    Serial.print(currentSpeed);
    Serial.print("% -> PWM: ");
    Serial.print(pwmValue);
    Serial.print(" (IN3=HIGH, IN4=LOW, ENB=PWM)");
    Serial.println();
  }

  Serial.print("Motor speed set to: ");
  Serial.print(percentage);
  Serial.print("% (PWM: ");
  Serial.print(speedToPWM(percentage));
  Serial.println(")");
}

int MotorController::getCurrentSpeed()
{
  return currentSpeed;
}

unsigned long MotorController::getLastUpdateTime()
{
  return lastUpdateTime;
}

int MotorController::speedToPWM(int percentage)
{
  // Convert percentage (0-100) to PWM value (0-255)
  // ESP8266 analogWrite uses 0-255 range (8-bit), not 10-bit!

  if (percentage == 0)
  {
    return 0;
  }
  int minPWM = 25;  // Start motor movement
  int maxPWM = 255; // Full speed

  return map(percentage, 1, 100, minPWM, maxPWM);
}
