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
void task2Code(void *parameter);
void startTask2();

int timerMs = 5000; // Number of milliseconds the gearbox waits until stopping

u_int8_t requestString[32]{};
u_int8_t replyString[32]{};

hw_timer_t *timer = NULL;

static constexpr const char *const gearboxName = "left";
static constexpr float gearboxSensorHeight = 0.0f;
static constexpr float gearboxMathematicalHeight = 0.0f;

TaskHandle_t task2Handle;
Communication communication{gearboxName, gearboxSensorHeight, gearboxMathematicalHeight};

// only for debug purposes, TODO: remove later
ICACHE_RAM_ATTR void buttonPress()
{
  DebugControls::debugButton1Press();
}

void IRAM_ATTR onTimer()
{
  communication.emergencyStop();
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

  // Get data from master, does not send anything back.
  Wire.onReceive(receiveEvent);
  // Send data to master, does not receive anything.
  Wire.onRequest(requestEvent);
  Serial.begin(115200);

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, timerMs * 1000, true);
  timerAlarmEnable(timer);

  if (debugMode)
  {
    pinMode(DebugButton1, INPUT_PULLUP); // set the digital pin as output
    attachInterrupt(digitalPinToInterrupt(DebugButton1), buttonPress, FALLING);
  }

  DebugControls::initDebugControls(&communication);

  startTask2();
}

void task2Setup()
{
  const int speed = 1000; // TODO set real value
  DeskMotor *const deskMotor = gearbox.getDeskMotor();
  deskMotor->run(speed);
}

void task2Loop()
{
  delay(1); // "Power Saving"
}

void loop()
{
  delay(1000);
  if (debugMode)
  {
    DebugControls::mainMenu();
  }
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
    if (debugMode)
    {
      Serial.println("Error: requestString is not 32 bytes long");
    }
    requestString[0] = 0xEE; // 0xEE is the code for transmission error
  }
  Communication::handleRequest(requestString, replyString);
}

void requestEvent()
{
  // Send answer back to the master board -> reply string
  // reply string should also contain current position (maybe read out directly here?)
  Wire.write("Hello from slave board!");
}

void startTask2()
{
  const int currentCoreId = xPortGetCoreID();
  const int newCoreId = currentCoreId == 0 ? 1 : 0;

  xTaskCreatePinnedToCore(
      task2Code,    /* Function to implement the task */
      "Task1",      /* Name of the task */
      10000,        /* Stack size in words, no idea what size to use */
      NULL,         /* Task input parameter */
      0,            /* Priority of the task */
      &task2Handle, /* Task handle. */
      newCoreId);   /* Core where the task should run */
}

void task2Code(void *parameter)
{
  task2Setup();

  while (true)
  {
    task2Loop();
  }
}