#pragma once

#include <Arduino.h>
#include "GearboxCommunication.hpp"
#include "ButtonEvents.hpp"
#include <queue>

class InputEvent
{
    ButtonId buttonId;
    ButtonEvent buttonEvent;

public:
    InputEvent(ButtonId buttonId, ButtonEvent buttonEvent) : buttonId(buttonId), buttonEvent(buttonEvent) {}
    ~InputEvent() = default;
};

class InputController
{
private:
    GearboxCommunication *const gearbox{};
    std::queue<InputEvent *> eventQueue;

public:
    InputController(GearboxCommunication *const gearbox, std::queue<InputEvent *> eventQueue) : gearbox(gearbox), eventQueue(eventQueue) {}
    ~InputController() = default;
};