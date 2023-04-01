#pragma once

#include <Arduino.h>
#include "Pinout.hpp"

class Lightgate
{
private:
    /* data */

public:
    Lightgate(/* args */) = default;
    ~Lightgate() = default;

    void setup();
    void CheckClosed();
    bool CheckPrimaryClosed();
    void CheckSecondaryClosed();
    int8_t CheckPrimaryStatus();
};