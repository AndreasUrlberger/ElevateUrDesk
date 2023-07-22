#include <Arduino.h>
#include <Wire.h>
#include <string>

#include "Gearbox.hpp"
#include "Pinout.hpp"
#include "Communication.hpp"
#include <esp_task_wdt.h>
#include <chrono>
#include <thread>
#include "Pinout.hpp"
#include "MotorTimer.hpp"

static constexpr float gearboxSensorHeight = 0.0f;
static constexpr float gearboxMathematicalHeight = 0.0f;

Communication communication{gearboxSensorHeight, gearboxMathematicalHeight};
MotorTimer motorTimer{communication.getGearbox()->getDeskMotor(), communication.getGearbox()->getSmallBrake(), communication.getGearbox()->getLargeBrake()};

void setup()
{
  Serial.begin(115200);
  esp_task_wdt_init(UINT32_MAX, false);
  esp_task_wdt_delete(NULL);


  // Initialize lightgate sensors.
  // pinMode(LIGHTGATE_LARGE_BRAKE_OPEN, INPUT_PULLUP);
  // pinMode(LIGHTGATE_LARGE_BRAKE_CLOSED, INPUT_PULLUP);

  Serial.print("Task1 is running on core ");
  Serial.println(xPortGetCoreID());

  Serial.println("WDT diabled on core 0");

  // Initialize an i2c bus.
}

void loop()
{
  // Read lightgate sensors.
  // bool largeBrakeIsOpen = digitalRead(LIGHTGATE_LARGE_BRAKE_OPEN);
  // bool largeBrakeIsClosed = digitalRead(LIGHTGATE_LARGE_BRAKE_CLOSED);

  // Serial.print("Large brake open: ");
  // Serial.print(largeBrakeIsOpen ? "true" : "false");
  // Serial.print(" | Large brake closed: ");
  // Serial.println(largeBrakeIsClosed ? "true" : "false");
}

void makeReadyToDrive()
{
  // Enable Motor (enable Relay)
  // Enable Motor Control (enable Relay)
  // Enable Motor via Motor Control (enable bit in Motor Control)

  // Release Brake
  //    - Check if Brake is released (brake open = HIGH, brake closed = LOW)
  //    - If (brake open = LOW AND brake closed = LOW)
  //        -> Error
  //    - While (brake open = LOW AND brake closed = HIGH)
  //        -> Drive Motor to release brake
  //    // Brake is now released

  // Ready to drive
}

void disableDriveMode()
{
  // Engage Brake
  //   - Check if brake is engaged (brake open = LOW, brake closed = HIGH)
  //   - If (brake open = LOW AND brake closed = LOW)
  //       -> Error
  //   - Lift motor a bit
  //   - While (brake open = HIGH AND brake closed = LOW)
  //       -> Drive Brake Motor to engage brake
  //   // Brake is now engaged

  // Disable Motor via Motor Control (disable bit in Motor Control)
  // Disable Motor Control (disable Relay)
  // Disable Motor (disable Relay)
}