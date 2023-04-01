#include <Arduino.h>
#include <Wire.h>
#include <string>

#include "Example.hpp"
#include "Gearbox.hpp"
#include "Pinout.hpp"
#include "Communication.hpp"
#include "DebugControls.hpp"

void receiveEvent(int numBytes);
void requestEvent();

int timerMs = 5000; // Number of milliseconds the gearbox waits until stopping

u_int8_t requestString[32]{};
u_int8_t replyString[32]{};

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer()
{
  portENTER_CRITICAL_ISR(&timerMux);
  GearboxRun = false;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void setup()
{
  int gearboxID = 1; // ID is 1 (left) or 2 (right)
  uint8_t GearboxAddress{};
  if (gearboxID == 1) // GearboxName is defined, used for prints
  {
    std::string GearboxName = "left";
    GearboxAddress = 0x50; // I2C address of left board
  }
  else
  {
    std::string GearboxName = "right";
    GearboxAddress = 0x51; // I2C address of right board
  }

  Wire.begin(GearboxAddress);

  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  Serial.begin(115200);

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, timerMs * 1000, true);
  timerAlarmEnable(timer);
}

void loop()
{
  delay(1000);
  if (DebugMode == true)
  {
    DebugControls::mainMenu();
  }
  /*
  if (GearboxRun == false)
  {
    Serial.println("Gearbox Stopped, restarting timer");
    GearboxRun = true;
    timerAlarmWrite(timer, timerMs * 1000, true);
    timerAlarmEnable(timer);
  }
  else
  {
    Serial.println("Gearbox still running");
  }
  */
}

void receiveEvent(int numBytes)
{
  // Read the message from the master board

  memset(requestString, 0, 32); // Clear the requestString (all 32 bytes to 0)

  int i = 0;
  while (Wire.available() > 0)
  {
    requestString[i] = Wire.read();
    i++;
  }
  // check if requestString is 32 bytes long
  if (i == 31)
  {
    // do nothing
  }
  else
  {
    if (DebugMode == true)
    {
      Serial.println("Error: requestString is not 32 bytes long");
    }
    requestString[i] = 0xEE; // 0xEE is the code for transmission error
  }
  Communication::handleRequest(requestString, replyString);
}

void requestEvent()
{
  // Send a message back to the master board
  Wire.write("Hello from slave board!");
}
