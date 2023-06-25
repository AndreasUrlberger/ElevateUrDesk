#pragma once

#include <Arduino.h>
#include "AccelStepper.h"
#include "Lightgate.hpp"
#include "Pinout.hpp"

class Brake
{
private:
    /* data */
    AccelStepper primaryStepper{AccelStepper::HALF4WIRE, LARGE_BRAKE_1, LARGE_BRAKE_3, LARGE_BRAKE_2, LARGE_BRAKE_4};
    AccelStepper secondaryStepper{AccelStepper::HALF4WIRE, SMALL_BRAKE_1, SMALL_BRAKE_3, SMALL_BRAKE_2, SMALL_BRAKE_4};
    Lightgate lightgate{};

public:
    Brake(/* args */) = default;
    ~Brake() = default;

    void setupPrimary();
    void setupSecondary();
    int8_t openPrimaryBrake();
    void openSecondaryBrake();
    void closePrimaryBrake();
    void closeSecondaryBrake();
};
