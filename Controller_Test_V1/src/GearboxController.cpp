#include "GearboxController.hpp"

bool communicate(int address, uint8_t (&message)[32], uint8_t (&outReply)[32])
{
  bool success = false;
  // sending message
  Wire.beginTransmission(address);
  Wire.write(&message[0], 32);
  Wire.endTransmission();
  // requesting answer
  delay(1);
  Wire.requestFrom(address, 32);
  int i = 0;
  while (Wire.available() > 0)
  {
    outReply[i] = Wire.read();
    i++;
  }
  if (i == 31)
  {
    success = true;
  }
  return success;
}