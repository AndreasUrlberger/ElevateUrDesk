#include <Arduino.h>
#include "SimpleHT16K33.hpp"

#define LED_I2C_ADDR 0x70
#define LED_I2C_SDA 21
#define LED_I2C_SCL 22

HT16K33 HT;

void setupDemo();
void loopDemo();

void setup()
{
  setupDemo();
}

void loop()
{
  loopDemo();
}

void setupDemo()
{
  Serial.begin(115200);
  Serial.println(F("ht16k33 light test v0.01"));
  Serial.println();
  // initialize everything, 0x00 is the i2c address for the first one (0x70 is added in the class).
  HT.begin(0x00, LED_I2C_SDA, LED_I2C_SCL);
  HT.setBrightness(0);
  HT.clearAll();
  HT.displayOn();
}

void loopDemo()
{
  uint8_t led;

  // flash the LEDs, first turn them on
  Serial.println("Turn on all LEDs");
  for (size_t led = 0; led < 128; led++)
  {
    HT.setLedNow(led);
    delay(50);
  } // for led

  // Next clear them
  Serial.println("Clear all LEDs");
  for (size_t led = 0; led < 128; led++)
  {
    HT.clearLedNow(led);
    delay(50);
  } // for led

  // One by one
  Serial.println("One by one");
  for (size_t row = 0; row < 2; row++)
  {
    for (size_t col = 0; col < 8; col++)
    {
      HT.setLedNow(row + col * 16);
      delay(500);
      HT.clearLedNow(row + col * 16);
    }
  } // for row
}
