#include "Communication.hpp"
#include "Gearbox.hpp"
#include "Pinout.hpp"

Communication::Communication(std::string gearboxName, float gearboxSensorHeight, float gearboxMathematicalHeight) : gearbox(gearboxName, gearboxSensorHeight, gearboxMathematicalHeight)
{
}

void Communication::moveTo(const long targetPosition)
{
  gearbox.moveToPosition(targetPosition);
  gearbox.startMotor();
}

void Communication::moveUp()
{
  gearbox.startMotor();
  gearbox.moveUp();
}

void Communication::moveDown()
{
  gearbox.startMotor();
  gearbox.moveDown();
}

void Communication::emergencyStop()
{
  gearbox.stopMotor();
}

void Communication::initDeskMotor()
{
  gearbox.initMotor();
}

bool Communication::readControllerMessage()
{
  // Read as many bytes as are available in the buffer but at most till the next newline.
  bool messageComplete{false};
  bool isMsgValid{false};
  while ((Wire.available() > 0) && !messageComplete)
  {
    if (expectedMsgLength == 0)
    {
      expectedMsgLength = Wire.read();
    }
    else
    {
      // Read the next character from the buffer
      char receivedChar = Wire.read();

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
    case CMD_MOVE_UP:
      Serial.println("Received move up command.");
      moveUp();
      break;
    case CMD_MOVE_DOWN:
      Serial.println("Received move down command.");
      moveDown();
      break;
    case CMD_MOVE_TO:
    {
      const uint32_t targetPosition = static_cast<uint32_t>(controlPanelMsg[1u]);
      Serial.println("Received move to command with target position: " + String(targetPosition));
      moveTo(targetPosition);
      break;
    }
    case CMD_EMERGENCY_STOP:
      emergencyStop();
      break;
    default:
      // Should never happen.
      // Print message bytes as integers for debugging.
      Serial.print("Received invalid message: ");
      for (size_t i = 0; i < expectedMsgLength; i++)
      {
        Serial.print(static_cast<uint32_t>(controlPanelMsg[i]));
        Serial.print(" ");
      }
      break;
    }

    // Clear everything for the next message.
    controlPanelMsgBuffer = "";
    expectedMsgLength = 0;
  }

  return messageComplete;
}

uint32_t Communication::getCurrentMotorPosition()
{
  return gearbox.getCurrentPosition();
}