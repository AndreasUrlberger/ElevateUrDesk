#include <Arduino.h>
#include <Wire.h>
#include <string>

#include "Pinout.hpp"
#include "TextInput.hpp"
#include "DebugControls.hpp"

char requestString[32]{};

void setup()
{
  // Wire.begin(GearboxAddress);
  Wire.begin(GearboxCommunicationSDA, GearboxCommunicationSCL);
  Serial.begin(115200);
}

void loop()
{
  delay(1000);
  if (debugMode)
  {
    Serial.println("Debug mode active\n");
    Serial.println("Input a number to test the following:");
    Serial.println("1: I2c-Communication \n2: ");
    int debugFeature = TextInput::getIntInput();
    DebugControls::handleDebugMode(debugFeature);
  }
}