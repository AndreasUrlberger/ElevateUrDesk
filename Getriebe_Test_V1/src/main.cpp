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
#include <chrono>
#include <thread>

static constexpr const char *const gearboxName = "left";
static constexpr float gearboxSensorHeight = 0.0f;
static constexpr float gearboxMathematicalHeight = 0.0f;

Communication *communication;

#pragma region DEBUG
hw_timer_t *debugButtonTimerHandle{nullptr};
// Should be same as in DeskMotor.
long moveInputIntervalMS{20};
volatile bool debugButtonPressed{false};

void IRAM_ATTR onButtonPressedTimer()
{
  if (debugButtonPressed)
  {
    digitalWrite(5, HIGH);
    communication->moveUp();
    digitalWrite(5, LOW);
  }
}

// only for debug purposes, TODO: remove later
ICACHE_RAM_ATTR void buttonPress()
{
  // LOW is high for this button.
  bool isPressed = digitalRead(DebugButton1) == LOW;
  debugButtonPressed = isPressed;

  // This does not work as expected because the button bounces.
  if (isPressed)
  {
    // communication->moveTo(10000);
    // Start timer
    // timerRestart(debugButtonTimerHandle);
  }
  else
  {
    // Stop current task
    // timerStop(debugButtonTimerHandle);
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
#pragma endregion DEBUG

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
  pinMode(19, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(DebugButton1, INPUT_PULLUP); // set the digital pin as output

  // Initialize timer.
  // The prescaler is used to divide the base clock frequency of the ESP32’s timer. The ESP32’s timer uses the APB clock (APB_CLK) as its base clock, which is normally 80 MHz. By setting the prescaler to 8000, we are dividing the base clock frequency by 8000, resulting in a timer tick frequency of 10 kHz (80 MHz / 8000 = 10 kHz).
  debugButtonTimerHandle = timerBegin(1, 8000, true);
  timerAttachInterrupt(debugButtonTimerHandle, &onButtonPressedTimer, true);
  timerAlarmWrite(debugButtonTimerHandle, 100, true); // Every 10 ms.
  timerAlarmEnable(debugButtonTimerHandle);
  // timerStop(debugButtonTimerHandle);

  attachInterrupt(digitalPinToInterrupt(DebugButton1), buttonPress, CHANGE);

  Serial.println("Task1 is running on core " + String(xPortGetCoreID()) + ".");
}

void loop()
{
  bool receiveMessage{};
  auto message = communication->receiveMessage(receiveMessage);
}