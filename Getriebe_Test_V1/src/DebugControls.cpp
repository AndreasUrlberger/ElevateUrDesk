#include "DebugControls.hpp"
#include "TextInput.hpp"

void DebugControls::mainMenu() // main menu for Gearbox debug mode
{
  Serial.println("Debugmode active:");
  Serial.println("1. I2C-Debug Mode");
  Serial.println("2. OTA Update Mode");
  Serial.println("3. WLED Debug Mode");
  Serial.println("4. Monitor Debug Mode");
  Serial.println("5. DeskMotor Debug Mode");
  int debugFeature = TextInput::getIntInput();
  handleDebugMode(debugFeature);
}

void DebugControls::handleDebugMode(int debugFeature)
{
  switch (debugFeature)
  {
  case 1: // mode for debugging I2C-Communication
    Serial.println("I2C-Debug Mode activated");
    break;
  case 2:
    Serial.println("OTA Update Mode selection activated");
    break;
  case 3:
    Serial.println("WLED Debug Mode activated");
    break;
  case 4:
    Serial.println("Monitor Debug Mode activated");
    break;
  case 5: // mode for debugging motor control
    Serial.println("DeskMotor Debug Mode activated");
    motorDebug();
    break;
  default:
    Serial.println("unknown debug mode");
    break;
  }
}

void DebugControls::motorDebug()
{
  Serial.println("choose function of motor to debug:");
  Serial.println("1. set motor acceleration, speed and steps");
  Serial.println("2. custom motor test");
  switch (TextInput::getIntInput())
  {
  case 1:
    // simple motor test
    break;
  case 2:
    // custom motor test
    break;
  default:
    Serial.println("unknown motor debug mode");
    break;
  }
  // int debugFeature = TextInput::getIntInput();
}

void DebugControls::customMotorTest()
{
  // custom motor test
}

void DebugControls::I2Cdebug()
{
  // enable communication with slave devices
  // first: select address of slave device
  // second: define data to send to slave device
  // third: display answer of I2C slave device
}