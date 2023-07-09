#include "ControlPanelCommunication.hpp"

#define Uart Serial2

ControlPanelCommunication::ControlPanelCommunication(std::queue<InputEvent *> *const eventQueue, const int8_t tx_pin, const int8_t rx_pin, const uint32_t config, const uint32_t baudrate) : eventQueue(eventQueue)
{
    Uart.begin(baudrate, config, rx_pin, tx_pin);
}

bool ControlPanelCommunication::update()
{
    return receiveMessage();
}

void ControlPanelCommunication::processMessage(const char *message, size_t messageLength)
{
    const size_t numEvents = messageLength / 4u;

    // Iterate over all events in the message
    for (size_t i = 0u; i < numEvents; i++)
    {
        const uint8_t buttonId = static_cast<uint8_t>(message[1u + (i * 4u)]);
        const uint8_t buttonEvent = static_cast<uint8_t>(message[3u + (i * 4u)]);

        eventQueue->push(new InputEvent(buttonId, buttonEvent));
    }
}

bool ControlPanelCommunication::receiveMessage()
{
    // Read as many bytes as are available in the buffer but at most till the next newline.
    bool messageComplete{false};
    bool isMsgValid{false};
    while ((Uart.available() > 0u) && !messageComplete)
    {
        if (expectedMsgLength == 0u)
        {
            expectedMsgLength = Uart.read();
        }
        else
        {
            // Read the next character from the buffer
            char receivedChar = Uart.read();

            if (controlPanelMsgBuffer.length() == (expectedMsgLength - 1u))
            {
                messageComplete = true;
                isMsgValid = (static_cast<uint8_t>(receivedChar) == 0u);
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
            processMessage(controlPanelMsg, controlPanelMsgBuffer.length());
            break;
        case 'E':
            Serial.print("Encoder: ");
            Serial.print(static_cast<uint32_t>(controlPanelMsg[1u]));
            Serial.print(" State: ");
            Serial.println(*reinterpret_cast<const uint16_t *>(&(controlPanelMsg[3u])));
            break;
        default:
            Serial.println("Unknown");
            for (size_t i = 0u; i < controlPanelMsgBuffer.length(); i++)
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
        expectedMsgLength = 0u;
    }

    return messageComplete;
}