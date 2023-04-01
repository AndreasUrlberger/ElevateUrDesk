#pragma once

#include <Arduino.h>
#include "AccelStepper.h"
#include "Lightgate.hpp"
#include "Pinout.hpp"

class Brake
{
private:
    /* data */
    AccelStepper primaryStepper{AccelStepper::HALF4WIRE, PrimaryBrake1, PrimaryBrake3, PrimaryBrake2, PrimaryBrake4};
    AccelStepper secondaryStepper{AccelStepper::HALF4WIRE, SecondaryBrake1, SecondaryBrake3, SecondaryBrake2, SecondaryBrake4};
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
