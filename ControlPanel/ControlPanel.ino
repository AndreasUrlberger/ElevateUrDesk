#include <Arduino.h>
#include <Wire.h>
// Requires AS5600 by Rob Tillaart
#include <AS5600.h>
// Requires FreeRTOS by Richard Barry.
#include "FreeRTOS.h"
#include "semphr.h"
#include <functional>

#define Uart Serial1
// Using micropython addresses.
static const uint8_t BUTTON_PIN{29u};
static const uint8_t NUMBER_OF_ENCODER_SLOTS{11u};
static const uint16_t ENCODER_RESOLUTION{4096u};
static const uint16_t ENCODER_ANGLE_OFFSET{3946u}; // - 150
// Hysteresis for the encoder slots in percent.
static const float ENCODER_SLOT_HYSTERESIS{0.05f};

AS5600 encoder;
// We use 255 as a default value to not influence the calculation of the first slot (Hysteresis).
uint8_t currentEndcoderSlot{255u};
// Already offset by ENCODER_ANGLE_OFFSET.
uint16_t currentEndcoderAngle{0u};
// Mutex for UART communication.
SemaphoreHandle_t uartMutex;

#pragma region Button
class Button
{
  // Decision tree for button events.
  //
  //                                   NotPressed
  //                                       |
  //                                    Pressed
  //                                       |
  //             +-------------------------+-------------------------+
  //             |                         |                         |
  //           <50ms                     <750ms                    >750ms
  //             |                         |                         |
  //         Released                  Released                      |
  //             |                         |                         |
  //         NO_CLICK                      |                     LONG_CLICK
  //                                       |
  //             +-------------------------+-------------------------+
  //             |                                                   |
  //          <300ms                                              >300ms
  //             |                                                   |
  //         Pressed                                            SINGLE_CLICK
  //             |
  //             +-------------------------+-------------------------+
  //             |                         |                         |
  //           <50ms                     >50ms                    >750ms
  //             |                         |                         |
  //         Released                  Released                DOUBLE_HOLD_CLICK
  //             |                         |
  //        SINGLE_CLICK              DOUBLE_CLICK

  static const uint32_t MIN_CLICK_DURATION{50u};
  static const uint32_t MIN_LONG_CLICK_DURATION{750u};
  static const uint32_t MAX_TIME_BETWEEN_CLICKS{300u};

  static const uint32_t NUM_SMOOTHING_ITERATIONS{5u};

public:
  class ButtonEvents
  {
  private:
    ButtonEvents() = delete;
    ~ButtonEvents() = delete;

  public:
    static const uint8_t SINGLE_CLICK = 0;
    static const uint8_t DOUBLE_CLICK = 1;
    static const uint8_t LONG_CLICK = 2;
    static const uint8_t START_DOUBLE_HOLD_CLICK = 3;
    static const uint8_t END_DOUBLE_HOLD_CLICK = 4;
    static const uint8_t NO_EVENT = 5;

    // Only in drive mode.
    static const uint8_t BUTTON_PRESSED = 6;
    static const uint8_t BUTTON_RELEASED = 7;
  };
  typedef uint8_t ButtonEvent;

private:
  class ButtonStates
  {
  public:
    static const uint8_t NOT_PRESSED = 0;
    static const uint8_t PRESSED = 1;
    static const uint8_t DOUBLE_CLICK_DECISION = 2;
    static const uint8_t PRESSED_SECOND = 3;
    static const uint8_t DOUBLE_HOLD = 4;
    static const uint8_t LONG_CLICK = 5;

    ButtonStates() = delete;
    ~ButtonStates() = delete;
  };
  typedef uint8_t ButtonState;

  uint32_t lastActionTime = 0u;
  bool buttonPressedLastIteration = false;
  ButtonState state = ButtonStates::NOT_PRESSED;
  const uint8_t pin;
  bool isDriveMode{false};
  std::array<bool, NUM_SMOOTHING_ITERATIONS> buttonHistory{false};
  size_t buttonHistoryIndex{0u};

  using StateTransitionFunction = std::function<ButtonEvent(uint32_t, bool, ButtonState &)>;
  // Directly initialize the table.
  static inline const StateTransitionFunction stateTransitionTable[6]{
      // NOT_PRESSED state (low).
      [](uint32_t time, bool isHigh, ButtonState &newState) -> ButtonEvent
      {
        if (isHigh)
        {
          newState = ButtonStates::PRESSED;
          return ButtonEvents::NO_EVENT;
        }
        else
        {
          newState = ButtonStates::NOT_PRESSED;
          return ButtonEvents::NO_EVENT;
        }
      },

      // PRESSED state (high).
      [](uint32_t time, bool isHigh, ButtonState &newState) -> ButtonEvent
      {
        if (time > MIN_LONG_CLICK_DURATION)
        {
          if (isHigh)
          {
            // EVENT: LONG_CLICK
            newState = ButtonStates::LONG_CLICK;
            return ButtonEvents::LONG_CLICK;
          }
          else
          {
            // EVENT: LONG_CLICK
            newState = ButtonStates::NOT_PRESSED;
            return ButtonEvents::LONG_CLICK;
          }
        }
        else if (time > MIN_CLICK_DURATION)
        {
          if (isHigh)
          {
            // No change to current state.
            newState = ButtonStates::PRESSED;
            return ButtonEvents::NO_EVENT;
          }
          else
          {
            newState = ButtonStates::DOUBLE_CLICK_DECISION;
            return ButtonEvents::NO_EVENT;
          }
        }
        else
        {
          if (isHigh)
          {
            // No change to current state.
            newState = ButtonStates::PRESSED;
            return ButtonEvents::NO_EVENT;
          }
          else
          {
            // Too short click.
            newState = ButtonStates::NOT_PRESSED;
            return ButtonEvents::NO_EVENT;
          }
        }
      },

      // DOUBLE_CLICK_DECISION state (low).
      [](uint32_t time, bool isHigh, ButtonState &newState) -> ButtonEvent
      {
        if (time > MAX_TIME_BETWEEN_CLICKS)
        {
          // EVENT: SINGLE_CLICK
          if (isHigh)
          {
            // One could argue that this should be either a SINGLE_CLICK or the start of a DOUBLE_CLICK.
            newState = ButtonStates::PRESSED;
          }
          else
          {
            newState = ButtonStates::NOT_PRESSED;
          }
          return ButtonEvents::SINGLE_CLICK;
        } // Could add requirement for the button to be released for a minimum time here.
        else
        {
          if (isHigh)
          {
            newState = ButtonStates::PRESSED_SECOND;
            return ButtonEvents::NO_EVENT;
          }
          else
          {
            // No change to current state.
            newState = ButtonStates::DOUBLE_CLICK_DECISION;
            return ButtonEvents::NO_EVENT;
          }
        }
      },

      // PRESSED_SECOND state (high).
      [](uint32_t time, bool isHigh, ButtonState &newState) -> ButtonEvent
      {
        if (time > MIN_LONG_CLICK_DURATION)
        {
          if (isHigh)
          {
            // EVENT: START_DOUBLE_HOLD_CLICK
            newState = ButtonStates::DOUBLE_HOLD;
            return ButtonEvents::START_DOUBLE_HOLD_CLICK;
          }
          else
          {
            // One could argue that this should be either a START_DOUBLE_HOLD_CLICK event directly followed by a END_DOUBLE_HOLD_CLICK event or a DOUBLE_CLICK event. I is impossible to tell if the button was released before or after the threshold, therefore, we chose the DOUBLE_CLICK, as two instantaneous events are undesireable.
            // EVENT: DOUBLE_CLICK
            newState = ButtonStates::NOT_PRESSED;
            return ButtonEvents::DOUBLE_CLICK;
          }
        }
        else if (time > MIN_CLICK_DURATION)
        {
          if (isHigh)
          {
            // No change to current state.
            newState = ButtonStates::PRESSED_SECOND;
            return ButtonEvents::NO_EVENT;
          }
          else
          {
            // EVENT: DOUBLE_CLICK
            newState = ButtonStates::NOT_PRESSED;
            return ButtonEvents::DOUBLE_CLICK;
          }
        }
        else
        {
          if (isHigh)
          {
            // No change to current state.
            newState = ButtonStates::PRESSED_SECOND;
            return ButtonEvents::NO_EVENT;
          }
          else
          {
            // Too short click.
            newState = ButtonStates::NOT_PRESSED;
            return ButtonEvents::NO_EVENT;
          }
        }
      },

      // DOUBLE_HOLD state (low).
      [](uint32_t time, bool isHigh, ButtonState &newState) -> ButtonEvent
      {
        if (isHigh)
        {
          // No change to current state.
          newState = ButtonStates::DOUBLE_HOLD;
          return ButtonEvents::NO_EVENT;
        }
        else
        {
          // EVENT: END_DOUBLE_HOLD_CLICK
          newState = ButtonStates::NOT_PRESSED;
          return ButtonEvents::END_DOUBLE_HOLD_CLICK;
        }
      },

      // LONG_CLICK state (high).
      [](uint32_t time, bool isHigh, ButtonState &newState) -> ButtonEvent
      {
        if (isHigh)
        {
          // No change to current state.
          newState = ButtonStates::LONG_CLICK;
          return ButtonEvents::NO_EVENT;
        }
        else
        {
          newState = ButtonStates::NOT_PRESSED;
          return ButtonEvents::NO_EVENT;
        }
      },
  };

public:
  Button(uint8_t pin) : pin(pin)
  {
  }

  void setup()
  {
    pinMode(pin, INPUT_PULLUP);
  }

  ButtonEvent transition()
  {
    const bool isPressed = digitalRead(pin) == LOW;
    buttonHistory[buttonHistoryIndex] = isPressed;
    buttonHistoryIndex = (buttonHistoryIndex + 1) % buttonHistory.size();
    const uint32_t timestamp = millis();
    ButtonEvent buttonEvent{};

    if (isDriveMode)
    {
      // Calculate average of button history.
      uint8_t sum = 0;
      for (size_t i = 0; i < buttonHistory.size(); i++)
      {
        sum += buttonHistory[i];
      }

      const bool isSmoothPressed = sum > (buttonHistory.size() / 2);

      if (isSmoothPressed && (state == ButtonStates::NOT_PRESSED))
      {
        state = ButtonStates::PRESSED;
        buttonEvent = ButtonEvents::BUTTON_PRESSED;
      }
      else if (!isSmoothPressed && (state == ButtonStates::PRESSED))
      {
        state = ButtonStates::NOT_PRESSED;
        buttonEvent = ButtonEvents::BUTTON_RELEASED;
      }
      else
      {
        buttonEvent = ButtonEvents::NO_EVENT;
      }
    }
    else
    {
      const ButtonState oldState = state;
      buttonEvent = stateTransitionTable[state](timestamp - lastActionTime, isPressed, state);
    }

    if (isPressed != buttonPressedLastIteration)
    {
      lastActionTime = timestamp;
      buttonPressedLastIteration = isPressed;
    }

    return buttonEvent;
  }

  void toggleDriveMode(bool isDriveMode)
  {
    this->isDriveMode = isDriveMode;

    const bool currentButtonState = digitalRead(pin) == LOW;

    // Exit any current state.
    state = currentButtonState ? ButtonStates::PRESSED : ButtonStates::NOT_PRESSED;
  }
};
#pragma endregion Button

static const size_t NUMBER_OF_BUTTONS{2u};
static Button buttons[NUMBER_OF_BUTTONS]{Button(29u), Button(28u) /*, Button(2u), Button(3u), Button(4u)*/};
static Button::ButtonEvent buttonEvents[NUMBER_OF_BUTTONS]{Button::ButtonEvents::NO_EVENT, Button::ButtonEvents::NO_EVENT /*, Button::ButtonEvents::NO_EVENT, Button::ButtonEvents::NO_EVENT, Button::ButtonEvents::NO_EVENT*/};

void setupButtons()
{
  for (size_t i = 0u; i < NUMBER_OF_BUTTONS; i++)
  {
    buttons[i].setup();
  }

  // TODO DEBUG only.
  buttons[0].toggleDriveMode(true);
}

void loopButtons()
{
  // Read the current state of the buttons and check if they have changed.
  for (size_t i = 0u; i < NUMBER_OF_BUTTONS; i++)
  {
    buttonEvents[i] = buttons[i].transition();
  }
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

void sendButtonStateChange()
{
  // Send through UART to general controller.
  const size_t maxMsgLength{4u * NUMBER_OF_BUTTONS + 2u};
  char buffer[maxMsgLength]{0u};
  uint8_t numEvents{0u};

  for (size_t buttonIndex = 0u; buttonIndex < NUMBER_OF_BUTTONS; buttonIndex++)
  {
    Button::ButtonEvent event = buttonEvents[buttonIndex];
    if (event != Button::ButtonEvents::NO_EVENT)
    {
      // NumEvents also works as an index for the buffer.
      buffer[4u * numEvents + 1u] = 'B';
      buffer[4u * numEvents + 2u] = buttonIndex;
      buffer[4u * numEvents + 3u] = 'S';
      buffer[4u * numEvents + 4u] = event;

      numEvents++;
    }
  }

  if (numEvents == 0u) // Dont want to send an empty and unnecessary message.
  {
    return;
  }

  const uint8_t msgLength = UINT8_C(4) * numEvents + UINT8_C(1);
  buffer[0u] = msgLength;
  // To check if the received message length is wrong.
  buffer[msgLength] = '\x00';

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

  setupButtons();

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
  if (iterationCounter % 5 == 0)
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

  loopButtons();
  sendButtonStateChange();

  delay(1);
}