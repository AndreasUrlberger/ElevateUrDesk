// Open and close brakes by controlling a stepper motor. Primary brake is the brake on the last gear, secondary break is the brake nearest to the motor

#include "Brake.hpp"
#include "Lightgate.hpp"

Brake::Brake(const int8_t dir, const uint8_t lightgateOpenPin, const uint8_t lightgateClosedPin, const uint8_t brakePin1, const uint8_t brakePin2, const uint8_t brakePin3, const uint8_t brakePin4) : lightgateOpen(lightgateOpenPin), lightgateClosed(lightgateClosedPin), stepper(AccelStepper::HALF4WIRE, brakePin1, brakePin3, brakePin2, brakePin4), targetPositionOpen(STEPS_TO_GO * dir), targetPositionClosed(-STEPS_TO_GO * dir)
{
    stepper.setMaxSpeed(MAX_SPEED);
    stepper.setAcceleration(MAX_ACCELERATION);
}

BrakeState Brake::getBrakeState() const
{
    const bool open = lightgateOpen.isLightgateBlocked();
    const bool closed = lightgateClosed.isLightgateBlocked();

    if (!open && !closed)
    {
        return Brake::BRAKE_STATE_INTERMEDIARY;
    }
    else if (open && !closed)
    {
        return Brake::BRAKE_STATE_UNLOCKED;
    }
    else if (!open && closed)
    {
        return Brake::BRAKE_STATE_LOCKED;
    }
    else // (open && closed) Can only happen if the connection to the lightgates is broken
    {
        return Brake::BRAKE_STATE_ERROR;
    }
}

void Brake::openBrake()
{
    stepper.moveTo(targetPositionOpen);
    Serial.print("Open Brake, Target position: ");
    Serial.println(stepper.targetPosition());
}

void Brake::closeBrake()
{
    stepper.moveTo(targetPositionClosed);
    Serial.print("Close Brake, Target position: ");
    Serial.println(stepper.targetPosition());
}

void Brake::step()
{
    stepper.run();
}