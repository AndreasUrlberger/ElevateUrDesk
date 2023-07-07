#pragma once

#include <Arduino.h>
#include "AccelStepper.h"
#include "Lightgate.hpp"

enum class BrakeState
{
    OPEN = 0,
    INTERMEDIATE = 1,
    CLOSED = 2,
    ERROR = 3
};

class Brake
{
private:
    static constexpr float MAX_SPEED = 500;        // max speed of primary brake motor
    static constexpr float MAX_ACCELERATION = 100; // max acceleration of primary brake motor
    static constexpr long STEPS_TO_GO = 1000;      // steps to go from open to closed

    AccelStepper stepper;

    const Lightgate lightgateOpen;
    const Lightgate lightgateClosed;

public:
    Brake(const uint8_t lightgateOpenPin, const uint8_t lightgateClosedPin, const uint8_t brakePin1, const uint8_t brakePin2, const uint8_t brakePin3, const uint8_t brakePin4);
    ~Brake() = default;

    BrakeState getBrakeState() const;

    void openBrake();
    void closeBrake();

    void step();
};
