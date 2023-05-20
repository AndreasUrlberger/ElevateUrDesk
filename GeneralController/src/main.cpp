#include <Arduino.h>
#include <string>

#define Uart Serial2

void setup()
{
  // Initialize Serial communication
  Serial.begin(115200);
  Uart.begin(115200, SERIAL_8N1, 16, 17);
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
      Serial.print("Button: ");
      Serial.print(static_cast<uint32_t>(controlPanelMsg[1u]));
      Serial.print(" State: ");
      Serial.println(static_cast<uint32_t>(controlPanelMsg[3u]));
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