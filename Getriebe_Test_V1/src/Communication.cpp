#include "Communication.hpp"
#include "Gearbox.hpp"
#include "Pinout.hpp"

Communication::Communication(std::string gearboxName, float gearboxSensorHeight, float gearboxMathematicalHeight) : gearbox(gearboxName, gearboxSensorHeight, gearboxMathematicalHeight)
{
}

void Communication::moveTo(const long targetPosition)
{
  gearbox.moveToPosition(targetPosition);
  gearbox.startMotor();
}

void Communication::moveUp(uint32_t penalty)
{
  gearbox.startMotor();
  gearbox.moveUp(penalty);
}

void Communication::moveDown(uint32_t penalty)
{
  gearbox.startMotor();
  gearbox.moveDown(penalty);
}

void Communication::emergencyStop()
{
  gearbox.stopMotor();
}

void Communication::initDeskMotor()
{
  gearbox.initMotor();
}

bool Communication::readControllerMessage()
{
  return false;
}

uint32_t Communication::getCurrentMotorPosition()
{
  return gearbox.getCurrentPosition();
}