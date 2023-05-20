#include <Arduino.h>
#include <Wire.h>
// Requires AS5600 by Rob Tillaart
#include <AS5600.h>
// Requires FreeRTOS by Richard Barry.
#include "FreeRTOS.h"
#include "semphr.h"

#define Uart Serial1
// Using micropython addresses.
static const uint8_t BUTTON_PIN{29u};
static const uint8_t NUMBER_OF_ENCODER_SLOTS{11u};
static const uint16_t ENCODER_RESOLUTION{4096u};
static const uint16_t ENCODER_ANGLE_OFFSET{3946u}; // - 50
// Hysteresis for the encoder slots in percent.
static const float ENCODER_SLOT_HYSTERESIS{0.05f};

AS5600 encoder;
// We use 255 as a default value to not influence the calculation of the first slot (Hysteresis).
uint8_t currentEndcoderSlot{255u};
// Already offset by ENCODER_ANGLE_OFFSET.
uint16_t currentEndcoderAngle{0u};
// Mutex for UART communication.
SemaphoreHandle_t uartMutex;

bool currentButtonState{false};

// Interrupt routine for the click of a button.
void buttonPress()
{
  noInterrupts();
  bool isPressed = digitalRead(BUTTON_PIN) == LOW;
  if(currentButtonState != isPressed){
    currentButtonState = isPressed;
    sendButtonStateChange(0u, isPressed);
  }
  interrupts();
}

void sendUartBytes(const char *buffer, size_t length)
{
  size_t bytesWritten{0};

  xSemaphoreTake(uartMutex, portMAX_DELAY);
  while (bytesWritten < length)
  {
    bytesWritten += Uart.write(&(buffer[bytesWritten]), length - bytesWritten);
  }
  xSemaphoreGive(uartMutex);
}

void sendButtonStateChange(uint8_t button, bool state)
{
  // Send through UART to general controller.
  static const uint8_t msgLength{5u};
  char buffer[msgLength + 1]{0};
  buffer[0] = msgLength;
  buffer[1] = 'B';
  buffer[2] = button;
  buffer[3] = 'S';
  buffer[4] = static_cast<uint8_t>(state);
  // To check if the received message length is wrong.
  buffer[5] = '\x00';

  size_t bytesWritten{0};

  sendUartBytes(buffer, msgLength + 1);
}

void sendEncoderStateChange(uint8_t encoder, uint8_t state)
{
  // Send through UART to general controller.
  static const uint8_t msgLength{5u};
  char buffer[msgLength + 1]{0};
  buffer[0] = msgLength;
  buffer[1] = 'E';
  buffer[2] = encoder;
  buffer[3] = 'S';
  buffer[4] = state;
  // To check if the received message length is wrong.
  buffer[5] = '\x00';

  sendUartBytes(buffer, msgLength + 1);
}

bool updateEncoderSlot()
{
  bool hasChanged{false};

  // Read the current angle of the encoder.
  // Serial.println(encoder.rawAngle());
  const uint16_t angle = encoder.readAngle();
  // We add the Encoder resolution to the angle to avoid negative numbers.
  const uint16_t offsetAngle = ((angle + ENCODER_RESOLUTION) - ENCODER_ANGLE_OFFSET) % ENCODER_RESOLUTION;
  currentEndcoderAngle = offsetAngle;
  // Calculate the current slot (hard slot).
  const float floatingSlot = offsetAngle / (static_cast<float>(ENCODER_RESOLUTION) / NUMBER_OF_ENCODER_SLOTS);
  const uint8_t hardSlot = static_cast<uint8_t>(floatingSlot);

  if (hardSlot != currentEndcoderSlot)
  {
    // Wenn aktuelle Position plus Hysterese immernoch im ursprünglichen Slot ist, dann ist der Slot nicht gewechselt.
    const float floatingSlotPlusHyst = floatingSlot + ENCODER_SLOT_HYSTERESIS;
    const float floatingSlotPlusHystClamped = floatingSlotPlusHyst > NUMBER_OF_ENCODER_SLOTS ? floatingSlotPlusHyst - NUMBER_OF_ENCODER_SLOTS : floatingSlotPlusHyst;
    const uint8_t hardSlotPlusHysteresis = static_cast<uint8_t>(floatingSlotPlusHystClamped);
    // Oder wenn aktuelle Position minus Hysterese immernoch im ursprünglichen Slot ist, dann ist der Slot nicht gewechselt.
    const float floatingSlotMinusHyst = floatingSlot - ENCODER_SLOT_HYSTERESIS;
    const float floatingSlotMinusHystClamped = floatingSlotMinusHyst >= 0.0f ? floatingSlotMinusHyst : floatingSlotMinusHyst + NUMBER_OF_ENCODER_SLOTS;
    const uint8_t hardSlotMinusHysteresis = static_cast<uint8_t>(floatingSlotMinusHystClamped);

    if ((hardSlotMinusHysteresis == currentEndcoderSlot) || (hardSlotPlusHysteresis == currentEndcoderSlot))
    {
      // Do not change the slot.
    }
    else
    {
      hasChanged = true;
      currentEndcoderSlot = hardSlot;
    }
  }

  return hasChanged;
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("startup");

  // Create a mutex for the UART communication.
  uartMutex = xSemaphoreCreateMutex();
  Uart.begin(115200);

  // Setup BUTTON_PIN pin for input.
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // Attach interrupt to button.
  attachInterrupt(BUTTON_PIN, buttonPress, CHANGE);

  Wire.begin();

  encoder.begin();

  Serial.print("Address of encoder: ");
  Serial.println(encoder.getAddress());

  int b = encoder.isConnected();
  Serial.print("Connected: ");
  Serial.println(b ? "true" : "false");
}

uint32_t iterationCounter = 0;
void loop()
{
  if (iterationCounter % 10 == 0)
  {
    bool isConnected = encoder.isConnected();
    if (!isConnected)
    {
      Serial.println("Encoder is not connected");
    }

    const bool hasEncoderSlotChanged = updateEncoderSlot();
    if (hasEncoderSlotChanged)
    {
      Serial.print("Encoder slot changed to: ");
      Serial.println(currentEndcoderSlot);
      sendEncoderStateChange(0, currentEndcoderSlot);
    }
  }
  iterationCounter++;

  delay(2);
}