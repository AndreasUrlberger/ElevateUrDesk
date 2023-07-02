#include "Communication.hpp"
#include "Gearbox.hpp"
#include "Pinout.hpp"

Communication::Communication(float gearboxSensorHeight, float gearboxMathematicalHeight) : gearbox(GEARBOX_NAME, gearboxSensorHeight, gearboxMathematicalHeight)
{
  instance = this;

  Serial.println("Initializing I2C bus");
  bool i2cSuccess = Wire.begin(I2C_ADDRESS, I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
  Serial.print("I2C bus initialized: ");
  Serial.println(i2cSuccess ? "true" : "false");

  // Setup I2C callbacks for communication with general controller.
  Wire.onReceive([](int numBytes)
                 { Communication::instance->genCtrlOnReceiveI2C(numBytes); });
  Wire.onRequest([]()
                 { Communication::instance->genCtrlOnRequestI2C(); });
}

void Communication::performMoveTo(const long targetPosition)
{
  gearbox.moveToPosition(targetPosition);
  gearbox.startMotor();
}

void Communication::performMoveUp(uint32_t penalty)
{
  gearbox.startMotor();
  gearbox.moveUp(penalty);
}

void Communication::performMoveDown(uint32_t penalty)
{
  gearbox.startMotor();
  gearbox.moveDown(penalty);
}

void Communication::performEmergencyStop()
{
  gearbox.stopMotor();
}

bool Communication::checkForGearboxDeviation(uint32_t currentPosition)
{
  const bool tooHigh = currentPosition > (otherGearboxPosition + MAX_GEARBOX_DEVIATION);
  const bool tooLow = otherGearboxPosition > (currentPosition + MAX_GEARBOX_DEVIATION);
  if (tooHigh || tooLow)
  {
    performEmergencyStop();
    return false;
  }

  return true;
}

uint32_t Communication::calculateCorrection(uint32_t deviation)
{
  const float correction = static_cast<float>(deviation - MAX_SOFT_GEARBOX_DEVIATION) / static_cast<float>(MAX_GEARBOX_DEVIATION - MAX_SOFT_GEARBOX_DEVIATION) * MAX_GEARBOX_DEVIATION;
  const uint32_t iCorrection = static_cast<uint32_t>(round(correction));
  return iCorrection;
}

void Communication::genCtrlOnReceiveI2C(int numBytes)
{
  readAllI2cData = false;

  i2cDataLength = numBytes;
  for (size_t index = 0u; index < numBytes; index++)
  {
    i2cData[index] = Wire.read();
  }

  readAllI2cData = true;
}

void Communication::genCtrlOnRequestI2C()
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
    genCtrlMoveUp();
    break;
  case CMD_MOVE_DOWN:
    genCtrlMoveDown();
    break;
  case CMD_MOVE_TO:
    genCtrlMoveTo();
    break;
  case CMD_EMERGENCY_STOP:
    genCtrlEmergencyStop();
    break;
  case CMD_GET_POSITION:
    genCtrlGetPosition();
    break;
  default:
    Serial.println("Unknown i2c command");
    break;
  }
}

void Communication::genCtrlMoveUp()
{
  constexpr size_t RESPONSE_LENGTH{5u};
  // Send current position and brake state as response.
  uint32_t currentPosition = gearbox.getCurrentPosition();
  uint8_t data[RESPONSE_LENGTH]{0u};
  memcpy(&(data[0u]), &currentPosition, 4u);
  data[4u] = gearbox.getCurrentBrakeState();

  size_t bytesWritten{0u};
  while (bytesWritten < RESPONSE_LENGTH)
  {
    bytesWritten += Wire.write(&(data[bytesWritten]), RESPONSE_LENGTH - bytesWritten);
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
    performMoveUp(correction);
  }
  else
  {
    // Gearboxes are close enough to each other, continue normally.
    performMoveUp(0u);
  }
}

void Communication::genCtrlMoveDown()
{
  constexpr size_t RESPONSE_LENGTH{5u};
  // Send current position and brake state as response.
  uint32_t currentPosition = gearbox.getCurrentPosition();
  uint8_t data[RESPONSE_LENGTH]{0u};
  memcpy(&(data[0u]), &currentPosition, 4u);
  data[4u] = gearbox.getCurrentBrakeState();

  size_t bytesWritten{0u};
  while (bytesWritten < RESPONSE_LENGTH)
  {
    bytesWritten += Wire.write(&(data[bytesWritten]), RESPONSE_LENGTH - bytesWritten);
  }

  // Get position of other gearbox from i2c data.
  memcpy(&otherGearboxPosition, &(i2cData[1u]), 4u);

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
    performMoveDown(correction);
  }
  else
  {
    // Gearboxes are close enough to each other, continue normally.
    performMoveDown(0u);
  }
}

void Communication::genCtrlMoveTo()
{
  Serial.println("I2C moveTo");

  constexpr size_t RESPONSE_LENGTH{5u};
  // Send current position and brake state as response.
  uint32_t currentPosition = gearbox.getCurrentPosition();
  uint8_t data[RESPONSE_LENGTH]{0u};
  memcpy(&(data[0u]), &currentPosition, 4u);
  data[4u] = gearbox.getCurrentBrakeState();

  size_t bytesWritten{0u};
  while (bytesWritten < RESPONSE_LENGTH)
  {
    bytesWritten += Wire.write(&(data[bytesWritten]), RESPONSE_LENGTH - bytesWritten);
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
    performEmergencyStop();
  }
  else
  {
    performMoveTo(targetPosition);
  }
}

void Communication::genCtrlEmergencyStop()
{
  constexpr size_t RESPONSE_LENGTH{5u};
  // Send current position and brake state as response.
  uint32_t currentPosition = gearbox.getCurrentPosition();
  uint8_t data[RESPONSE_LENGTH]{0u};
  memcpy(&(data[0u]), &currentPosition, 4u);
  data[4u] = gearbox.getCurrentBrakeState();

  size_t bytesWritten{0u};
  while (bytesWritten < RESPONSE_LENGTH)
  {
    bytesWritten += Wire.write(&(data[bytesWritten]), RESPONSE_LENGTH - bytesWritten);
  }

  Serial.println("I2C emergencyStop");
  performEmergencyStop();
}

void Communication::genCtrlGetPosition()
{
  constexpr size_t RESPONSE_LENGTH{5u};
  // Send current position and brake state as response.
  uint32_t currentPosition = gearbox.getCurrentPosition();
  uint8_t data[RESPONSE_LENGTH]{0u};
  memcpy(&(data[0u]), &currentPosition, 4u);
  data[4u] = gearbox.getCurrentBrakeState();

  size_t bytesWritten{0u};
  while (bytesWritten < RESPONSE_LENGTH)
  {
    bytesWritten += Wire.write(&(data[bytesWritten]), RESPONSE_LENGTH - bytesWritten);
  }

  // Get position of other gearbox from i2c data.
  memcpy(&otherGearboxPosition, &(i2cData[1]), 4u);
}