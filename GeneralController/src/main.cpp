#include <Arduino.h>
#include <string>
#include <ButtonEvents.hpp>
#include <chrono>
#include <thread>
#include <Wire.h>

#define Uart Serial2

std::string controlPanelMsgBuffer = "";
uint8_t expectedMsgLength = 0;

// Chrono time
std::chrono::steady_clock::time_point start;
// Target time
std::chrono::steady_clock::time_point target;
// Iteration duration
std::chrono::steady_clock::duration iterationDuration = std::chrono::milliseconds(10);

uint8_t upButtonId = 1;
uint8_t downButtonId = 2;

// Control panel state
bool moveUp{false};
bool moveDown{false};

int gearboxAddress = 0x08;

void setup()
{
  // Initialize Serial communication
  Serial.begin(115200);
  Uart.begin(115200, SERIAL_8N1, 16, 17);

  // Initialize start time
  start = std::chrono::steady_clock::now();
  target = start;

  int sdaPin = 21;
  int sclPin = 22;
  int freq = 100000;
  Serial.println("Initializing I2C bus");
  bool i2cSuccess = Wire.begin(sdaPin, sclPin, freq);
  Serial.print("I2C bus initialized: ");
  Serial.println(i2cSuccess ? "true" : "false");

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
      if (buttonId == upButtonId)
      {
        moveUp = true;
      }
      else if (buttonId == downButtonId)
      {
        moveDown = true;
      }
      Serial.println(" pressed");
      break;
    case ButtonEvents::BUTTON_RELEASED:
      if (buttonId == upButtonId)
      {
        moveUp = false;
      }
      else if (buttonId == downButtonId)
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

void sendGearboxCommand(uint8_t *data, size_t dataLength, size_t responseLength, uint8_t *response)
{
  Wire.beginTransmission(gearboxAddress);
  size_t bytesWritten{0u};
  while (bytesWritten < dataLength)
  {
    bytesWritten += Wire.write(&(data[bytesWritten]), dataLength - bytesWritten);
  }
  Wire.endTransmission();

  // Request a response from the gearbox.
  Wire.requestFrom(gearboxAddress, responseLength);

  // Read the response.
  size_t bytesRead{0u};
  while (bytesRead < responseLength)
  {
    if (Wire.available())
    {
      response[bytesRead] = Wire.read();
      bytesRead++;
    }
  }
}

static constexpr char CMD_MOVE_UP = 'u';
static constexpr char CMD_MOVE_DOWN = 'd';
static constexpr char CMD_MOVE_TO = 'm';
static constexpr char CMD_EMERGENCY_STOP = 'e';

void sendGearboxDriveUp()
{
  uint8_t data[1] = {CMD_MOVE_UP};
  uint32_t currentGreaboxPosition{};
  sendGearboxCommand(data, 1, 4, reinterpret_cast<uint8_t *>(&currentGreaboxPosition));
  Serial.print("Current position: ");
  Serial.println(currentGreaboxPosition);
}

void sendGearboxDriveDown()
{
  uint8_t data[1] = {CMD_MOVE_DOWN};
  uint32_t currentGreaboxPosition{};
  sendGearboxCommand(data, 1, 4, reinterpret_cast<uint8_t *>(&currentGreaboxPosition));
  Serial.print("Current position: ");
  Serial.println(currentGreaboxPosition);
}

void sendGearboxMoveTo(uint32_t position)
{
  uint8_t data[5] = {CMD_MOVE_TO};
  memcpy(&(data[1]), &position, 4);
  sendGearboxCommand(data, 5, 0, nullptr);
}

void sendGearboxEmergencyStop()
{
  uint8_t data[1] = {CMD_EMERGENCY_STOP};
  sendGearboxCommand(data, 1, 0, nullptr);
}

void loop()
{
  // Read the next message from the control panel.

  // Wait till the next iteration should start.
  std::this_thread::sleep_until(target);

  // Execute code here.
  digitalWrite(23, HIGH);

  // Read all messages from the control panel.
  while (readControlPanelMessage())
  {
  }

  // Send move command to gearboxes.
  if (moveUp && moveDown)
  {
    // Do nothing.
  }
  else if (moveUp)
  {
    // Tell gearboxes to move up.
    sendGearboxDriveUp();
  }
  else if (moveDown)
  {
    // Tell gearboxes to move down.
    sendGearboxDriveDown();
  }
  else
  {
    // Do nothing.
  }

  digitalWrite(23, LOW);

  // Update the target time for the next iteration.
  target += iterationDuration;
}