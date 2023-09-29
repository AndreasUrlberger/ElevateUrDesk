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
MotorTimer motorTimer{communication.getGearbox()->getDeskMotor(), communication.getGearbox()->getLargeBrake()};

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
}