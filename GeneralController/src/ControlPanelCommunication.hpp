#pragma once

#include <Arduino.h>
#include <queue>
#include "InputController.hpp"

class ControlPanelCommunication
{
private:
    std::queue<InputEvent *> eventQueue;

    std::string controlPanelMsgBuffer{""};
    uint8_t expectedMsgLength{0u};

    bool receiveMessage();
    void processMessage(const char *message, size_t messageLength);

public:
    ControlPanelCommunication(std::queue<InputEvent *> eventQueue, const int8_t tx_pin, const int8_t rx_pin, const uint32_t config, const uint32_t baudrate);
    ~ControlPanelCommunication() = default;

    bool update();
};