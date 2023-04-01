#pragma once

#include <Arduino.h>
#include <AccelStepper.h>
#include <Wire.h>
#include "Pinout.hpp"

class Communication
{
private:
    /* data */
public:
    Communication(/* args */);
    ~Communication();

    bool receiveMessage(uint8_t (&message)[32]);
    static void handleRequest(uint8_t (&message)[32], uint8_t (&reply)[32]);
};

Communication::Communication(/* args */)
{
}

Communication::~Communication()
{
}
