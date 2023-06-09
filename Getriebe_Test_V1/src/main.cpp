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

#ifdef GEARBOX_LEFT
static constexpr int8_t I2C_ADDRESS = 0x33;
static constexpr const char *const GEARBOX_NAME = "left";
#else
static constexpr int8_t I2C_ADDRESS = 0x88;
static constexpr const char *const GEARBOX_NAME = "right";
#endif

static constexpr int I2C_SDA_PIN = 21;
static constexpr int I2C_SCL_PIN = 22;
static constexpr uint32_t I2C_FREQ = 100000;

// I2C Command Codes.
static constexpr char CMD_MOVE_UP = 'u';
static constexpr char CMD_MOVE_DOWN = 'd';
static constexpr char CMD_MOVE_TO = 'm';
static constexpr char CMD_EMERGENCY_STOP = 'e';
static constexpr char CMD_GET_POSITION = 'p';

static constexpr float gearboxSensorHeight = 0.0f;
static constexpr float gearboxMathematicalHeight = 0.0f;

Communication *communication;

#pragma region DEBUG

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

bool readAllI2cData{false};
size_t i2cDataLength{0u};
uint8_t i2cData[16u]{0u};

uint32_t getCurrentMotorPosition()
{
  return communication->getCurrentMotorPosition();
}

void cmdMoveUp()
{
  // Send current position as response.
  uint32_t currentPosition = getCurrentMotorPosition();
  uint8_t *data = reinterpret_cast<uint8_t *>(&currentPosition);

  size_t responseLength{4u};
  size_t bytesWritten{0u};
  while (bytesWritten < responseLength)
  {
    bytesWritten += Wire.write(&(data[bytesWritten]), responseLength - bytesWritten);
  }
  communication->moveUp();
}

void cmdMoveDown()
{
  // Send current position as response.
  uint32_t currentPosition = getCurrentMotorPosition();
  uint8_t *data = reinterpret_cast<uint8_t *>(&currentPosition);

  size_t responseLength{4u};
  size_t bytesWritten{0u};
  while (bytesWritten < responseLength)
  {
    bytesWritten += Wire.write(&(data[bytesWritten]), responseLength - bytesWritten);
  }
  communication->moveDown();
}

void cmdMoveTo()
{
  Serial.println("I2C moveTo");
  uint32_t targetPosition{0u};
  // Get target position from i2c data.
  memcpy(&targetPosition, &(i2cData[1]), 4u);
  communication->moveTo(targetPosition);
}

void cmdEmergencyStop()
{
  Serial.println("I2C emergencyStop");
  communication->emergencyStop();
}

void cmdGetPosition()
{
  // Send current position as response.
  uint32_t currentPosition = getCurrentMotorPosition();
  uint8_t *data = reinterpret_cast<uint8_t *>(&currentPosition);

  size_t responseLength{4u};
  size_t bytesWritten{0u};
  while (bytesWritten < responseLength)
  {
    bytesWritten += Wire.write(&(data[bytesWritten]), responseLength - bytesWritten);
  }
}

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
    cmdMoveUp();
    break;
  case CMD_MOVE_DOWN:
    cmdMoveDown();
    break;
  case CMD_MOVE_TO:
    cmdMoveTo();
    break;
  case CMD_EMERGENCY_STOP:
    cmdEmergencyStop();
    break;
  case CMD_GET_POSITION:
    cmdGetPosition();
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
  communication = new Communication(GEARBOX_NAME, gearboxSensorHeight, gearboxMathematicalHeight);

  // TODO Debug only
  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(DebugButton1, INPUT_PULLUP); // set the digital pin as output

  Serial.println("Initializing I2C bus");
  bool i2cSuccess = Wire.begin(I2C_ADDRESS, I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
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