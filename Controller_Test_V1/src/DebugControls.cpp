#include "DebugControls.hpp"

void DebugControls::handleDebugMode(int debugFeature)
{
  switch (debugFeature)
  {
  case 1: // mode for debugging I2C-Communication
    Serial.println("I2C-Debug Mode");
    break;
  case 2:
    Serial.println("OTA Update Mode");
    break;
  case 3:
    Serial.println("WLED Debug Mode");
    break;
  case 4:
    Serial.println("Monitor Debug Mode");
    break;
  default:
    Serial.println("unknown debug mode");
    break;
  }
}

void DebugControls::I2Cdebug()
{
  // decide on debug mode

  // I2C-Communication Mode
  // enable communication with slave devices
  // first: select address of slave device
  // second: define data to send to slave device
  // third: display answer of I2C slave device

  // direct control of selected slave device
  // passthrough of slave-specific debug modes
}