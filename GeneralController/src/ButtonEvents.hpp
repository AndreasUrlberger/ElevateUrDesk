#pragma once

#include <cstdint>

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