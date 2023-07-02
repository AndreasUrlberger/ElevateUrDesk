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

bool playSound{false};

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

static const uint8_t BUZZER_PIN{2u};
static bool buzzerOn{false};

static const size_t BUTTON_MAIN_INDEX{0u}; // The one in the center of the wheel.
static const size_t BUTTON_UP_INDEX{1u};
static const size_t BUTTON_DOWN_INDEX{2u};
static const size_t BUTTON_SHORTCUT_1{3u};
static const size_t BUTTON_SHORTCUT_2{4u};

static const size_t NUMBER_OF_BUTTONS{5u};
static Button buttons[NUMBER_OF_BUTTONS]{Button(29u), Button(28u), Button(27u), Button(3u), Button(4u)};
static Button::ButtonEvent buttonEvents[NUMBER_OF_BUTTONS]{Button::ButtonEvents::NO_EVENT, Button::ButtonEvents::NO_EVENT, Button::ButtonEvents::NO_EVENT, Button::ButtonEvents::NO_EVENT, Button::ButtonEvents::NO_EVENT};

void setupButtons()
{
  for (size_t i = 0u; i < NUMBER_OF_BUTTONS; i++)
  {
    buttons[i].setup();
  }

  // TODO DEBUG only.
  buttons[BUTTON_UP_INDEX].toggleDriveMode(true);
  buttons[BUTTON_DOWN_INDEX].toggleDriveMode(true);
}

void setupBuzzer()
{
  pinMode(BUZZER_PIN, OUTPUT);
}

void switchBuzzer(bool on)
{
  buzzerOn = on;
  digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
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

      Serial.print("Button ");
      Serial.print(buttonIndex);
      Serial.print(" event: ");
      Serial.println(event);
    }
  }

  if (numEvents == 0u) // Dont want to send an empty and unnecessary message.
  {
    return;
  }

  if (buttonEvents[BUTTON_SHORTCUT_1] == Button::ButtonEvents::SINGLE_CLICK)
  {
    playSound = !playSound;
    if (!playSound)
    {
      noTone(BUZZER_PIN);
    }
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
  setupBuzzer();
  noTone(BUZZER_PIN);

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

void setup1()
{
}

#pragma region notes
/*************************************************
 * Public Constants
 *************************************************/
#define REST 0

#define NOTE_B0 31
#define NOTE_C1 33
#define NOTE_CS1 35
#define NOTE_D1 37
#define NOTE_DS1 39
#define NOTE_E1 41
#define NOTE_F1 44
#define NOTE_FS1 46
#define NOTE_G1 49
#define NOTE_GS1 52
#define NOTE_A1 55
#define NOTE_AS1 58
#define NOTE_B1 62
#define NOTE_C2 65
#define NOTE_CS2 69
#define NOTE_D2 73
#define NOTE_DS2 78
#define NOTE_E2 82
#define NOTE_F2 87
#define NOTE_FS2 93
#define NOTE_G2 98
#define NOTE_GS2 104
#define NOTE_A2 110
#define NOTE_AS2 117
#define NOTE_B2 123
#define NOTE_C3 131
#define NOTE_CS3 139
#define NOTE_D3 147
#define NOTE_DS3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_FS3 185
#define NOTE_G3 196
#define NOTE_GS3 208
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_CS4 277
#define NOTE_D4 294
#define NOTE_DS4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_FS4 370
#define NOTE_G4 392
#define NOTE_GS4 415
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880
#define NOTE_AS5 932
#define NOTE_B5 988
#define NOTE_C6 1047
#define NOTE_CS6 1109
#define NOTE_D6 1175
#define NOTE_DS6 1245
#define NOTE_E6 1319
#define NOTE_F6 1397
#define NOTE_FS6 1480
#define NOTE_G6 1568
#define NOTE_GS6 1661
#define NOTE_A6 1760
#define NOTE_AS6 1865
#define NOTE_B6 1976
#define NOTE_C7 2093
#define NOTE_CS7 2217
#define NOTE_D7 2349
#define NOTE_DS7 2489
#define NOTE_E7 2637
#define NOTE_F7 2794
#define NOTE_FS7 2960
#define NOTE_G7 3136
#define NOTE_GS7 3322
#define NOTE_A7 3520
#define NOTE_AS7 3729
#define NOTE_B7 3951
#define NOTE_C8 4186
#define NOTE_CS8 4435
#define NOTE_D8 4699
#define NOTE_DS8 4978
#pragma endregion notes

void playMelody();

void loop1()
{

  playMelody();
}

#pragma region DOOM
// int melody[] = {
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_FS3, NOTE_D3, NOTE_B2, NOTE_A3, NOTE_FS3, NOTE_B2, NOTE_D3, NOTE_FS3, NOTE_A3, NOTE_FS3, NOTE_D3, NOTE_B2,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_B3, NOTE_G3, NOTE_E3, NOTE_G3, NOTE_B3, NOTE_E4, NOTE_G3, NOTE_B3, NOTE_E4, NOTE_B3, NOTE_G4, NOTE_B4,

//     NOTE_A2, NOTE_A2, NOTE_A3, NOTE_A2, NOTE_A2, NOTE_G3, NOTE_A2, NOTE_A2,
//     NOTE_F3, NOTE_A2, NOTE_A2, NOTE_DS3, NOTE_A2, NOTE_A2, NOTE_E3, NOTE_F3,
//     NOTE_A2, NOTE_A2, NOTE_A3, NOTE_A2, NOTE_A2, NOTE_G3, NOTE_A2, NOTE_A2,
//     NOTE_F3, NOTE_A2, NOTE_A2, NOTE_DS3,

//     NOTE_A2, NOTE_A2, NOTE_A3, NOTE_A2, NOTE_A2, NOTE_G3, NOTE_A2, NOTE_A2,
//     NOTE_F3, NOTE_A2, NOTE_A2, NOTE_DS3, NOTE_A2, NOTE_A2, NOTE_E3, NOTE_F3,
//     NOTE_A2, NOTE_A2, NOTE_A3, NOTE_A2, NOTE_A2, NOTE_G3, NOTE_A2, NOTE_A2,
//     NOTE_A3, NOTE_F3, NOTE_D3, NOTE_A3, NOTE_F3, NOTE_D3, NOTE_C4, NOTE_A3, NOTE_F3, NOTE_A3, NOTE_F3, NOTE_D3,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2,

//     NOTE_CS3, NOTE_CS3, NOTE_CS4, NOTE_CS3, NOTE_CS3, NOTE_B3, NOTE_CS3, NOTE_CS3,
//     NOTE_A3, NOTE_CS3, NOTE_CS3, NOTE_G3, NOTE_CS3, NOTE_CS3, NOTE_GS3, NOTE_A3,
//     NOTE_B2, NOTE_B2, NOTE_B3, NOTE_B2, NOTE_B2, NOTE_A3, NOTE_B2, NOTE_B2,
//     NOTE_G3, NOTE_B2, NOTE_B2, NOTE_F3,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_B3, NOTE_G3, NOTE_E3, NOTE_G3, NOTE_B3, NOTE_E4, NOTE_G3, NOTE_B3, NOTE_E4, NOTE_B3, NOTE_G4, NOTE_B4,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_FS3, NOTE_DS3, NOTE_B2, NOTE_FS3, NOTE_DS3, NOTE_B2, NOTE_G3, NOTE_D3, NOTE_B2, NOTE_DS4, NOTE_DS3, NOTE_B2,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_E4, NOTE_B3, NOTE_G3, NOTE_G4, NOTE_E4, NOTE_G3, NOTE_B3, NOTE_D4, NOTE_E4, NOTE_G4, NOTE_E4, NOTE_G3,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2,

//     NOTE_A2, NOTE_A2, NOTE_A3, NOTE_A2, NOTE_A2, NOTE_G3, NOTE_A2, NOTE_A2,
//     NOTE_F3, NOTE_A2, NOTE_A2, NOTE_DS3, NOTE_A2, NOTE_A2, NOTE_E3, NOTE_F3,
//     NOTE_A2, NOTE_A2, NOTE_A3, NOTE_A2, NOTE_A2, NOTE_G3, NOTE_A2, NOTE_A2,
//     NOTE_A3, NOTE_F3, NOTE_D3, NOTE_A3, NOTE_F3, NOTE_D3, NOTE_C4, NOTE_A3, NOTE_F3, NOTE_A3, NOTE_F3, NOTE_D3,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2,

//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_C3, NOTE_E2, NOTE_E2, NOTE_AS2, NOTE_E2, NOTE_E2, NOTE_B2, NOTE_C3,
//     NOTE_E2, NOTE_E2, NOTE_E3, NOTE_E2, NOTE_E2, NOTE_D3, NOTE_E2, NOTE_E2,
//     NOTE_B3, NOTE_G3, NOTE_E3, NOTE_B2, NOTE_E3, NOTE_G3, NOTE_C4, NOTE_B3, NOTE_G3, NOTE_B3, NOTE_G3, NOTE_E3};

// int durations[] = {
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 2,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 2,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 2,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 2,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 2,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 2,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 2,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 2,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 2,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 2,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 2,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 2,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 2,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 2,

//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     8, 8, 8, 8, 8, 8, 8, 8,
//     16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
#pragma endregion DOOM

#pragma region TETRIS
// int melody[] = {
//     NOTE_E5, NOTE_B4, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_B4,
//     NOTE_A4, NOTE_A4, NOTE_C5, NOTE_E5, NOTE_D5, NOTE_C5,
//     NOTE_B4, NOTE_C5, NOTE_D5, NOTE_E5,
//     NOTE_C5, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_B4, NOTE_C5,

//     NOTE_D5, NOTE_F5, NOTE_A5, NOTE_G5, NOTE_F5,
//     NOTE_E5, NOTE_C5, NOTE_E5, NOTE_D5, NOTE_C5,
//     NOTE_B4, NOTE_B4, NOTE_C5, NOTE_D5, NOTE_E5,
//     NOTE_C5, NOTE_A4, NOTE_A4, REST,

//     NOTE_E5, NOTE_B4, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_B4,
//     NOTE_A4, NOTE_A4, NOTE_C5, NOTE_E5, NOTE_D5, NOTE_C5,
//     NOTE_B4, NOTE_C5, NOTE_D5, NOTE_E5,
//     NOTE_C5, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_B4, NOTE_C5,

//     NOTE_D5, NOTE_F5, NOTE_A5, NOTE_G5, NOTE_F5,
//     NOTE_E5, NOTE_C5, NOTE_E5, NOTE_D5, NOTE_C5,
//     NOTE_B4, NOTE_B4, NOTE_C5, NOTE_D5, NOTE_E5,
//     NOTE_C5, NOTE_A4, NOTE_A4, REST,

//     NOTE_E5, NOTE_C5,
//     NOTE_D5, NOTE_B4,
//     NOTE_C5, NOTE_A4,
//     NOTE_GS4, NOTE_B4, REST,
//     NOTE_E5, NOTE_C5,
//     NOTE_D5, NOTE_B4,
//     NOTE_C5, NOTE_E5, NOTE_A5,
//     NOTE_GS5};

// int durations[] = {
//     4, 8, 8, 4, 8, 8,
//     4, 8, 8, 4, 8, 8,
//     4, 8, 4, 4,
//     4, 4, 8, 4, 8, 8,

//     4, 8, 4, 8, 8,
//     4, 8, 4, 8, 8,
//     4, 8, 8, 4, 4,
//     4, 4, 4, 4,

//     4, 8, 8, 4, 8, 8,
//     4, 8, 8, 4, 8, 8,
//     4, 8, 4, 4,
//     4, 4, 8, 4, 8, 8,

//     4, 8, 4, 8, 8,
//     4, 8, 4, 8, 8,
//     4, 8, 8, 4, 4,
//     4, 4, 4, 4,

//     2, 2,
//     2, 2,
//     2, 2,
//     2, 4, 8,
//     2, 2,
//     2, 2,
//     4, 4, 2,
//     2};
#pragma endregion TETRIS

#pragma region PIRATES
int melody[] = {
    NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, REST,
    NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, REST,
    NOTE_C5, NOTE_D5, NOTE_B4, NOTE_B4, REST,
    NOTE_A4, NOTE_G4, NOTE_A4, REST,

    NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, REST,
    NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, REST,
    NOTE_C5, NOTE_D5, NOTE_B4, NOTE_B4, REST,
    NOTE_A4, NOTE_G4, NOTE_A4, REST,

    NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, REST,
    NOTE_A4, NOTE_C5, NOTE_D5, NOTE_D5, REST,
    NOTE_D5, NOTE_E5, NOTE_F5, NOTE_F5, REST,
    NOTE_E5, NOTE_D5, NOTE_E5, NOTE_A4, REST,

    NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, REST,
    NOTE_D5, NOTE_E5, NOTE_A4, REST,
    NOTE_A4, NOTE_C5, NOTE_B4, NOTE_B4, REST,
    NOTE_C5, NOTE_A4, NOTE_B4, REST,

    NOTE_A4, NOTE_A4,
    // Repeat of first part
    NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, REST,
    NOTE_C5, NOTE_D5, NOTE_B4, NOTE_B4, REST,
    NOTE_A4, NOTE_G4, NOTE_A4, REST,

    NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, REST,
    NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, REST,
    NOTE_C5, NOTE_D5, NOTE_B4, NOTE_B4, REST,
    NOTE_A4, NOTE_G4, NOTE_A4, REST,

    NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, REST,
    NOTE_A4, NOTE_C5, NOTE_D5, NOTE_D5, REST,
    NOTE_D5, NOTE_E5, NOTE_F5, NOTE_F5, REST,
    NOTE_E5, NOTE_D5, NOTE_E5, NOTE_A4, REST,

    NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, REST,
    NOTE_D5, NOTE_E5, NOTE_A4, REST,
    NOTE_A4, NOTE_C5, NOTE_B4, NOTE_B4, REST,
    NOTE_C5, NOTE_A4, NOTE_B4, REST,
    // End of Repeat

    NOTE_E5, REST, REST, NOTE_F5, REST, REST,
    NOTE_E5, NOTE_E5, REST, NOTE_G5, REST, NOTE_E5, NOTE_D5, REST, REST,
    NOTE_D5, REST, REST, NOTE_C5, REST, REST,
    NOTE_B4, NOTE_C5, REST, NOTE_B4, REST, NOTE_A4,

    NOTE_E5, REST, REST, NOTE_F5, REST, REST,
    NOTE_E5, NOTE_E5, REST, NOTE_G5, REST, NOTE_E5, NOTE_D5, REST, REST,
    NOTE_D5, REST, REST, NOTE_C5, REST, REST,
    NOTE_B4, NOTE_C5, REST, NOTE_B4, REST, NOTE_A4};

int durations[] = {
    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8,

    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8,

    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 8, 4, 8,

    8, 8, 4, 8, 8,
    4, 8, 4, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 4,

    4, 8,
    // Repeat of First Part
    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8,

    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8,

    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 8, 8,
    8, 8, 8, 4, 8,

    8, 8, 4, 8, 8,
    4, 8, 4, 8,
    8, 8, 4, 8, 8,
    8, 8, 4, 4,
    // End of Repeat

    4, 8, 4, 4, 8, 4,
    8, 8, 8, 8, 8, 8, 8, 8, 4,
    4, 8, 4, 4, 8, 4,
    8, 8, 8, 8, 8, 2,

    4, 8, 4, 4, 8, 4,
    8, 8, 8, 8, 8, 8, 8, 8, 4,
    4, 8, 4, 4, 8, 4,
    8, 8, 8, 8, 8, 2};
#pragma endregion PIRATES

size_t melodyLength{sizeof(melody) / sizeof(int)};
size_t noteIndex{0u};

void playMelody()
{
  if (playSound)
  {
    // to calculate the note duration, take one second divided by the note type.
    // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int duration = (1000 / durations[noteIndex]);
    // int duration = 1000 / durations[noteIndex];
    tone(BUZZER_PIN, melody[noteIndex]);
    delay(duration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = duration * 1.30;
    noTone(BUZZER_PIN);
    delay(pauseBetweenNotes);
  }

  noteIndex++;
  if (noteIndex >= melodyLength)
  {
    delay(5000);
    noteIndex = 0;
  }
}