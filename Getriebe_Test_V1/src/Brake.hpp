#pragma once

#include <Arduino.h>
#include "AccelStepper.h"
#include "Lightgate.hpp"

typedef uint8_t BrakeState;

class Brake
{
private:
    static constexpr float MAX_SPEED = 500;        // max speed of primary brake motor
    static constexpr float MAX_ACCELERATION = 200; // max acceleration of primary brake motor
    static constexpr long STEPS_TO_GO = 500;      // steps to go from open to closed

    AccelStepper stepper;

    const Lightgate lightgateOpen;
    const Lightgate lightgateClosed;

public:
    static constexpr BrakeState BRAKE_STATE_LOCKED = 0;
    static constexpr BrakeState BRAKE_STATE_INTERMEDIARY = 1;
    static constexpr BrakeState BRAKE_STATE_UNLOCKED = 3;
    static constexpr BrakeState BRAKE_STATE_ERROR = 2;

    Brake(const uint8_t lightgateOpenPin, const uint8_t lightgateClosedPin, const uint8_t brakePin1, const uint8_t brakePin2, const uint8_t brakePin3, const uint8_t brakePin4);
    ~Brake() = default;

    BrakeState getBrakeState() const;

    void openBrake();
    void closeBrake();

    void step();
};
