#include "DeskMotor.hpp"

void DeskMotor::setup()
{
    // Powering on the motor controller by turning on the relay(s) in the right order
    // SPI communication with motor controller
    deskMotor.setMaxSpeed(maxSpeed);
    deskMotor.setAcceleration(maxAcceleration);
    Serial.printf("Main motor initialized");
}

void DeskMotor::run(int targetPosition, int newSpeed) // has to run on different core so that it is non-blocking ?!?
{
    // set target position

    // newSpeed is the computed speed to resync the two gearboxes

    // debug mode
    if (DebugMode == true)
    {
        Serial.printf("Target position %d set\n", targetPosition);
    }

    // run motor until target is reached
    if (deskMotor.distanceToGo() != 0)
    {
        deskMotor.run();
    }

    // debug mode
    if (DebugMode == true)
    {
        Serial.println("DeskMotor stopped");
    }

    // if GearboxRun == false, run stop command of gearbox
}

void DeskMotor::getSkippedSteps()
{
    // get skipped steps from motor controller
}

void DeskMotor::setMaxAcceleration(const float newMaxAcceleration)
{
    maxAcceleration = newMaxAcceleration;
    deskMotor.setAcceleration(maxAcceleration);
}

void DeskMotor::setMaxSpeed(const float newMaxSpeed)
{
    maxSpeed = newMaxSpeed;
    deskMotor.setMaxSpeed(maxSpeed);
}