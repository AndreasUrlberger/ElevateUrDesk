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
static constexpr uint32_t I2C_FREQ = 100000u;

// I2C Command Codes.
static constexpr char CMD_MOVE_UP = 'u';
static constexpr char CMD_MOVE_DOWN = 'd';
static constexpr char CMD_MOVE_TO = 'm';
static constexpr char CMD_EMERGENCY_STOP = 'e';
static constexpr char CMD_GET_POSITION = 'p';

static constexpr float gearboxSensorHeight = 0.0f;
static constexpr float gearboxMathematicalHeight = 0.0f;

static constexpr uint32_t MAX_GEARBOX_DEVIATION = 1000u;
static constexpr uint32_t MAX_SOFT_GEARBOX_DEVIATION = 400u;

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

uint32_t otherGearboxPosition{0u};

uint32_t getCurrentMotorPosition()
{
  return communication->getCurrentMotorPosition();
}

/**
 * Checks if the two gearboxes deviate too fare from each other and performs an emergency stop if they do. Returns true if the gearboxes are close enough to each other.bool checkForGearboxDeviation(uint32_t currentPosition)
 */
bool checkForGearboxDeviation(uint32_t currentPosition)
{
  const bool tooHigh = currentPosition > (otherGearboxPosition + MAX_GEARBOX_DEVIATION);
  const bool tooLow = otherGearboxPosition > (currentPosition + MAX_GEARBOX_DEVIATION);
  if (tooHigh || tooLow)
  {
    communication->emergencyStop();
    return false;
  }

  return true;
}

uint32_t calculateCorrection(uint32_t deviation)
{
  const float correction = static_cast<float>(deviation - MAX_SOFT_GEARBOX_DEVIATION) / static_cast<float>(MAX_GEARBOX_DEVIATION - MAX_SOFT_GEARBOX_DEVIATION) * MAX_GEARBOX_DEVIATION;
  const uint32_t iCorrection = static_cast<uint32_t>(round(correction));
  return iCorrection;
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

  // Get position of other gearbox from i2c data.
  memcpy(&otherGearboxPosition, &(i2cData[1u]), 4u);

  // TODO DEBUGGING ONLY
  const int32_t deviation = static_cast<int32_t>(currentPosition) - static_cast<int32_t>(otherGearboxPosition);
  Serial.print("MoveUp: Deviation is ");
  Serial.println(deviation);

  // Compare this gearbox's current target position with the other gearbox's current target position.
  // If the deviation is larger than the hard limit, stop the movement.
  if (!checkForGearboxDeviation(currentPosition))
  {
    // Deviation is too large, emergency stop applied.
    Serial.println("MoveUp: Emergency stop applied due to deviation.");
    return;
  }

  // If the deviation is larger than the soft limit and this gearbox is ahead of the other, subtract the difference from the current target position.
  if (currentPosition > (otherGearboxPosition + MAX_SOFT_GEARBOX_DEVIATION))
  {
    const uint32_t deviation = currentPosition - otherGearboxPosition;
    const uint32_t correction = calculateCorrection(deviation);
    Serial.print("MoveUp Soft Limit: Correction is ");
    Serial.println(correction);
    // Add argument to moveUp function specifying the distance to subtract from the current target position.
    communication->moveUp(correction);
  }
  else
  {
    // Gearboxes are close enough to each other, continue normally.
    communication->moveUp(0u);
  }
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

  // Get position of other gearbox from i2c data.
  memcpy(&otherGearboxPosition, &(i2cData[1u]), 4u);

  // TODO DEBUGGING ONLY
  const int32_t deviation = static_cast<int32_t>(currentPosition) - static_cast<int32_t>(otherGearboxPosition);
  Serial.print("MoveDown: Deviation is ");
  Serial.println(deviation);

  // Compare this gearbox's current target position with the other gearbox's current target position.
  // If the deviation is larger than the hard limit, stop the movement.
  if (!checkForGearboxDeviation(currentPosition))
  {
    // Deviation is too large, emergency stop applied.
    Serial.println("MoveDown: Emergency stop applied due to deviation.");
    return;
  }
  // If the deviation is larger than the soft limit and this gearbox is ahead of the other, subtract the difference from the current target position.
  if (otherGearboxPosition > (currentPosition + MAX_SOFT_GEARBOX_DEVIATION))
  {
    const uint32_t deviation = otherGearboxPosition - currentPosition;
    const uint32_t correction = calculateCorrection(deviation);
    // Add argument to moveDown function specifying the distance to subtract from the current target position.
    Serial.print("MoveDown Soft Limit: Correction is ");
    Serial.println(correction);
    communication->moveDown(correction);
  }
  else
  {
    // Gearboxes are close enough to each other, continue normally.
    communication->moveDown(0u);
  }
}

void cmdMoveTo()
{
  Serial.println("I2C moveTo");
  // Send current position as response.
  uint32_t currentPosition = getCurrentMotorPosition();
  uint8_t *data = reinterpret_cast<uint8_t *>(&currentPosition);

  size_t responseLength{4u};
  size_t bytesWritten{0u};
  while (bytesWritten < responseLength)
  {
    bytesWritten += Wire.write(&(data[bytesWritten]), responseLength - bytesWritten);
  }

  // Get position of other gearbox from i2c data.
  memcpy(&otherGearboxPosition, &(i2cData[1u]), 4u);
  uint32_t targetPosition{0u};
  // Get target position from i2c data.
  memcpy(&targetPosition, &(i2cData[5u]), 4u);

  // TODO DEBUGGING ONLY
  const int32_t deviation = static_cast<int32_t>(currentPosition) - static_cast<int32_t>(otherGearboxPosition);
  Serial.print("MoveTo: Deviation is ");
  Serial.println(deviation);

  // Check that current position is not too far away from current position of other gearbox.
  // TODO If current position is ahead of other gearbox, try to throttle the movement a bit. (Probably not necessary, just stop if too far away.)
  if ((currentPosition > (otherGearboxPosition + MAX_GEARBOX_DEVIATION)) || (otherGearboxPosition > (currentPosition + MAX_GEARBOX_DEVIATION)))
  {
    Serial.println("I2C moveTo: Too far away from other gearbox, stopping.");
    communication->emergencyStop();
  }
  else
  {
    communication->moveTo(targetPosition);
  }
}

void cmdEmergencyStop()
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

  // Get position of other gearbox from i2c data.
  memcpy(&otherGearboxPosition, &(i2cData[1]), 4u);
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