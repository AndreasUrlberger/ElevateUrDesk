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

static constexpr char CMD_MOVE_UP = 'u';
static constexpr char CMD_MOVE_DOWN = 'd';
static constexpr char CMD_MOVE_TO = 'm';
static constexpr char CMD_EMERGENCY_STOP = 'e';

bool readAllI2cData{false};
size_t i2cDataLength{0u};
uint8_t i2cData[16u]{0u};

void i2cOnReceive(int numBytes)
{
  readAllI2cData = false;

  i2cDataLength = numBytes;
  for (size_t index = 0u; index < numBytes; index++)
  {
    i2cData[index] = Wire.read();
  }

  readAllI2cData = true;
  Serial.println();
}

void i2cOnRequest()
{
  if (!readAllI2cData)
  {
    Serial.println("Has not yet read all i2c data.");
    return;
  }

  // Handle commands.
  switch (i2cData[0])
  {
  case CMD_MOVE_UP:
    // Send current position as response.
    {
      uint32_t currentPosition{1234u};
      uint8_t *data = reinterpret_cast<uint8_t *>(&currentPosition);

      size_t responseLength{4u};
      size_t bytesWritten{0u};
      while (bytesWritten < responseLength)
      {
        bytesWritten += Wire.write(&(data[bytesWritten]), responseLength - bytesWritten);
      }
      communication->moveUp();
    }
    break;
  case CMD_MOVE_DOWN:
    // Send current position as response.
    {
      uint32_t currentPosition{1234u};
      uint8_t *data = reinterpret_cast<uint8_t *>(&currentPosition);

      size_t responseLength{4u};
      size_t bytesWritten{0u};
      while (bytesWritten < responseLength)
      {
        bytesWritten += Wire.write(&(data[bytesWritten]), responseLength - bytesWritten);
      }
      communication->moveDown();
    }
    break;
  case CMD_MOVE_TO:
  {
    Serial.println("I2C moveTo");
    uint32_t targetPosition{0u};
    // Get target position from i2c data.
    memcpy(&targetPosition, &(i2cData[1]), 4u);
    communication->moveTo(targetPosition);
    break;
  }
  case CMD_EMERGENCY_STOP:
    Serial.println("I2C emergencyStop");
    communication->emergencyStop();
    break;
  default:
    Serial.println("Unknown i2c command");
    break;
  }
}

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
  // debugButtonTimerHandle = timerBegin(1, 8000, true);
  // timerAttachInterrupt(debugButtonTimerHandle, &onButtonPressedTimer, true);
  // timerAlarmWrite(debugButtonTimerHandle, 100, true); // Every 10 ms.
  // timerAlarmEnable(debugButtonTimerHandle);
  // // timerStop(debugButtonTimerHandle);

  // attachInterrupt(digitalPinToInterrupt(DebugButton1), buttonPress, CHANGE);

  int i2cAddress = 0x08;
  int sdaPin = 21;
  int sclPin = 22;
  uint32_t freq = 100000;

  Serial.println("Initializing I2C bus");
  bool i2cSuccess = Wire.begin(i2cAddress, sdaPin, sclPin, freq);
  Serial.print("I2C bus initialized: ");
  Serial.println(i2cSuccess ? "true" : "false");

  Wire.onReceive(i2cOnReceive);
  Wire.onRequest(i2cOnRequest);

  Serial.println("Task1 is running on core " + String(xPortGetCoreID()) + ".");
}

void loop()
{
  communication->readControllerMessage();
}