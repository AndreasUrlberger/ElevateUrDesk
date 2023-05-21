#include <Arduino.h>
#include <string>

#define Uart Serial2

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
};
typedef uint8_t ButtonEvent;

void setup()
{
  // Initialize Serial communication
  Serial.begin(115200);
  Uart.begin(115200, SERIAL_8N1, 16, 17);
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
    default:
      Serial.println(" unknown event");
      break;
    }
  }
}

std::string controlPanelMsgBuffer = "";
uint8_t expectedMsgLength = 0;
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
      // Print message as hex
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
  // Read the next message from the control panel.
  readControlPanelMessage();
}