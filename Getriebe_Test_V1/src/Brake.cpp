// Open and close brakes by controlling a stepper motor. Primary brake is the brake on the last gear, secondary break is the brake nearest to the motor

#include "Brake.hpp"
#include "Lightgate.hpp"

Brake::Brake(const uint8_t lightgateOpenPin, const uint8_t lightgateClosedPin, const uint8_t brakePin1, const uint8_t brakePin2, const uint8_t brakePin3, const uint8_t brakePin4) : lightgateOpen(lightgateOpenPin), lightgateClosed(lightgateClosedPin), stepper(AccelStepper::HALF4WIRE, brakePin1, brakePin3, brakePin2, brakePin4)
{
    stepper.setMaxSpeed(MAX_SPEED);
    stepper.setAcceleration(MAX_ACCELERATION);
}

// TODO Implement stepping functionality like in DeskMotor.cpp

BrakeState Brake::getBrakeState() const
{
    const bool open = lightgateOpen.isLightgateBlocked();
    const bool closed = lightgateClosed.isLightgateBlocked();

    if (!open && !closed)
    {
        return BrakeState::INTERMEDIATE;
    }
    else if (open && !closed)
    {
        return BrakeState::OPEN;
    }
    else if (!open && closed)
    {
        return BrakeState::CLOSED;
    }
    else
    {
        return BrakeState::ERROR;
    }
}

void Brake::openBrake()
{
    // TODO Implement
    stepper.moveTo(STEPS_TO_GO);
}

void Brake::closeBrake()
{
    // TODO Implement
    stepper.moveTo(-STEPS_TO_GO);
}

void Brake::step()
{
    stepper.run();
}