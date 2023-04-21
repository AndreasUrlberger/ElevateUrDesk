#include <Arduino.h>
#include <Wire.h>
#include <string>

#include "Example.hpp"
#include "Gearbox.hpp"
#include "Pinout.hpp"
#include "Communication.hpp"
#include "DebugControls.hpp"
#include "TextInput.hpp"
#include <esp_task_wdt.h>

static constexpr const char *const gearboxName = "left";
static constexpr float gearboxSensorHeight = 0.0f;
static constexpr float gearboxMathematicalHeight = 0.0f;

Communication *communication;

// only for debug purposes, TODO: remove later
ICACHE_RAM_ATTR void buttonPress()
{
  // LOW is high for this button.
  bool isPressed = digitalRead(DebugButton1) == LOW;

  if (isPressed)
  {
    communication->moveTo(10000);
  }
}

class MainLogUtil
{
private:
  MainLogUtil() = delete;
  ~MainLogUtil() = delete;

public:
  static void logStuff()
  {
    Gearbox *gearbox = &(communication->gearbox);
    DeskMotor *deskMotor = &(gearbox->deskMotor);
    // Print if deskMotor is running
    Serial.print("DeskMotor is running: ");
    Serial.println(deskMotor->isRunning);
    // Print current target position
    Serial.print("Current target position: ");
    Serial.println(deskMotor->deskMotor.targetPosition());
    // Print current position
    Serial.print("Current position: ");
    Serial.println(deskMotor->deskMotor.currentPosition());
    // Print distance to go
    Serial.print("Distance to go: ");
    Serial.println(deskMotor->deskMotor.distanceToGo());
  }
};

void setup()
{
  Serial.begin(115200);
  esp_task_wdt_init(UINT32_MAX, false);
  esp_task_wdt_delete(NULL);

  Serial.println("WDT diabled on core 0");

  // Cannot initialize earlier as the Serial.begin() call is required.
  communication = new Communication(gearboxName, gearboxSensorHeight, gearboxMathematicalHeight);

  // TODO Debug only
  pinMode(18, OUTPUT);
  pinMode(DebugButton1, INPUT_PULLUP); // set the digital pin as output
  attachInterrupt(digitalPinToInterrupt(DebugButton1), buttonPress, CHANGE);

  Serial.println("Task1 is running on core " + String(xPortGetCoreID()) + ".");
}

void loop()
{
  delay(10);
  // int input = TextInput::getIntInput();
  // // LogStuff
  // MainLogUtil::logStuff();
}