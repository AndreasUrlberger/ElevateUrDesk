#include <Arduino.h>
#include <string>
#include <ButtonEvents.hpp>
#include <chrono>
#include <thread>
#include <Wire.h>
#include <GearboxCommunication.hpp>

#define Uart Serial2

static constexpr uint8_t GEARBOX_LEFT_ADDRESS = 0x33;
static constexpr uint8_t GEARBOX_RIGHT_ADDRESS = 0x88;
static constexpr uint32_t MAX_GEARBOX_DEVIATION = 1000u;

static constexpr int I2C_SDA_PIN = 21;
static constexpr int I2C_SCL_PIN = 22;
static constexpr uint32_t I2C_FREQ = 100000u;

static constexpr uint8_t BUTTON_ID_MAIN{0u}; // The one in the center of the wheel.
static constexpr uint8_t BUTTON_ID_MOVE_UP{1u};
static constexpr uint8_t BUTTON_ID_MOVE_DOWN{2u};
static constexpr uint8_t BUTTON_ID_SHORTCUT_1{3u};
static constexpr uint8_t BUTTON_ID_SHORTCUT_2{4u};

std::string controlPanelMsgBuffer = "";
uint8_t expectedMsgLength = 0u;

// Chrono time
std::chrono::steady_clock::time_point start;
// Target time
std::chrono::steady_clock::time_point target;
// Iteration duration
std::chrono::steady_clock::duration iterationDuration = std::chrono::milliseconds(10);

// Control panel state
bool moveUp{false};
bool moveDown{false};
bool moveTo{false};

GearboxCommunication gearbox(GEARBOX_LEFT_ADDRESS, GEARBOX_RIGHT_ADDRESS, &Wire, I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);

void setup()
{
  // Initialize Serial communication
  Serial.begin(115200);
  Uart.begin(115200, SERIAL_8N1, 16, 17);

  // Initialize start time
  start = std::chrono::steady_clock::now();
  target = start;

  // DEBUGGING
  // init output pin
  pinMode(23, OUTPUT);
}

void processButtonMessage(const char *message, size_t messageLength)
{
  const size_t numEvents = messageLength / 4;

  // Iterate over all events in the message
  for (size_t i = 0u; i < numEvents; i++)
  {
    const uint8_t buttonId = static_cast<uint8_t>(message[1u + (i * 4u)]);
    const uint8_t buttonEvent = static_cast<uint8_t>(message[3u + (i * 4u)]);

    Serial.print("Button ");
    Serial.print(buttonId);

    switch (buttonEvent)
    {
    case ButtonEvents::SINGLE_CLICK:
      Serial.println(" single click");
      if (buttonId == BUTTON_ID_MAIN)
      {
        moveTo = true;
      }
      break;
    case ButtonEvents::DOUBLE_CLICK:
      Serial.println(" double click");
      break;
    case ButtonEvents::LONG_CLICK:
      Serial.println(" long click");
      break;
    case ButtonEvents::START_DOUBLE_HOLD_CLICK:
      Serial.println(" start double hold click");
      break;
    case ButtonEvents::END_DOUBLE_HOLD_CLICK:
      Serial.println(" end double hold click");
      break;
    case ButtonEvents::NO_EVENT: // Should never happen
      Serial.println(" no event");
      break;
    case ButtonEvents::BUTTON_PRESSED:
      if (buttonId == BUTTON_ID_MOVE_UP)
      {
        moveUp = true;
      }
      else if (buttonId == BUTTON_ID_MOVE_DOWN)
      {
        moveDown = true;
      }
      Serial.println(" pressed");
      break;
    case ButtonEvents::BUTTON_RELEASED:
      if (buttonId == BUTTON_ID_MOVE_UP)
      {
        moveUp = false;
      }
      else if (buttonId == BUTTON_ID_MOVE_DOWN)
      {
        moveDown = false;
      }
      Serial.println(" released");
      break;
    default: // Should never happen either.
      Serial.println(" unknown event");
      break;
    }
  }
}

bool readControlPanelMessage()
{
  // Read as many bytes as are available in the buffer but at most till the next newline.
  bool messageComplete{false};
  bool isMsgValid{false};
  while ((Uart.available() > 0) && !messageComplete)
  {
    if (expectedMsgLength == 0)
    {
      expectedMsgLength = Uart.read();
    }
    else
    {
      // Read the next character from the buffer
      char receivedChar = Uart.read();

      if (controlPanelMsgBuffer.length() == (expectedMsgLength - 1))
      {
        messageComplete = true;
        isMsgValid = (static_cast<uint8_t>(receivedChar) == 0);
      }
      else
      {
        // Add the new character to the end of the string.
        controlPanelMsgBuffer += receivedChar;
      }
    }
  }

  // If we received a full message and it is valid, then print it
  if (messageComplete && isMsgValid)
  {
    const char *controlPanelMsg = controlPanelMsgBuffer.c_str();

    switch (controlPanelMsg[0u])
    {
    case 'B':
      processButtonMessage(controlPanelMsg, controlPanelMsgBuffer.length());
      break;
    case 'E':
      Serial.print("Encoder: ");
      Serial.print(static_cast<uint32_t>(controlPanelMsg[1u]));
      Serial.print(" State: ");
      Serial.println(*reinterpret_cast<const uint16_t *>(&(controlPanelMsg[3u])));
      break;
    default:
      Serial.println("Unknown");
      for (size_t i = 0; i < controlPanelMsgBuffer.length(); i++)
      {
        Serial.print("'");
        Serial.print(static_cast<uint32_t>(controlPanelMsgBuffer[i]));
        Serial.print("'");
        Serial.print(" ");
      }
      Serial.println();
      break;
    }

    // Clear everything for the next message.
    controlPanelMsgBuffer = "";
    expectedMsgLength = 0;
  }

  return messageComplete;
}

void loop()
{
  // Wait till the next iteration should start.
  std::this_thread::sleep_until(target);

  // Execute code here.
  digitalWrite(23, HIGH);

  // Read all messages from the control panel.
  while (readControlPanelMessage())
  {
  }

  // TODO Priorities for commands, e.g. emergency stop has highest priority, overriding all other commands.

  // Send move command to gearboxes.
  if (moveUp && moveDown)
  {
    // Do nothing.
    gearbox.getPosition();
  }
  else if (moveUp)
  {
    // Tell gearboxes to move up.
    gearbox.driveUp();
  }
  else if (moveDown)
  {
    // Tell gearboxes to move down.
    gearbox.driveDown();
  }
  else
  {
    // Do nothing.
    gearbox.getPosition();
  }

  if (moveTo)
  {
    const uint32_t moveToPosition = 40000;
    gearbox.driveTo(moveToPosition);
    moveTo = false;
  }

  // Calculate diff between position of gearboxes.
  const uint32_t gearboxLeftPosition = gearbox.getPositionLeft();
  const uint32_t gearboxRightPosition = gearbox.getPositionRight();
  const int32_t diff = static_cast<int32_t>(gearboxLeftPosition) - static_cast<int32_t>(gearboxRightPosition);
  if (abs(diff) > MAX_GEARBOX_DEVIATION)
  {
    // Emergency stop.
    gearbox.emergencyStop();
  }

  digitalWrite(23, LOW);

  // Update the target time for the next iteration.
  target += iterationDuration;
}