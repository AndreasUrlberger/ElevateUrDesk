#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "Pinout.hpp"

class GearboxController
{
private:
    /* data */

public:
    GearboxController(/* args */) = default;
    ~GearboxController() = default;

    bool communicate(int address, uint8_t (&message)[32], uint8_t (&outReply)[32]);
};