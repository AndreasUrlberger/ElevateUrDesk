// Open and close brakes by controlling a stepper motor. Primary brake is the brake on the last gear, secondary break is the brake nearest to the motor

#include "Brake.hpp"
#include "Lightgate.hpp"

float primaryMaxSpeed = 500;        // max speed of primary brake motor
float primaryMaxAcceleration = 100; // max acceleration of primary brake motor
long primaryStepsNeeded = 1000;     // steps to go from open to closed

float secondaryMaxSpeed = 500;
float secondaryMaxAcceleration = 100;
long secondaryStepsNeeded = 1000;

void Brake::setupPrimary()
{
    primaryStepper.setMaxSpeed(primaryMaxSpeed);
    primaryStepper.setAcceleration(primaryMaxAcceleration);
    Serial.printf("Primary Brake Stepper initialized");
}

void Brake::setupSecondary()
{
    secondaryStepper.setMaxSpeed(secondaryMaxSpeed);
    secondaryStepper.setAcceleration(secondaryMaxAcceleration);
    Serial.printf("Secondary Brake Stepper initialized");
}

int8_t Brake::openPrimaryBrake() // return 1 = succesfully opened; 0 = still closed; -1 = status undefined
{
    primaryStepper.move(primaryStepsNeeded);
    if (primaryStepper.distanceToGo() != 0)
    {
        primaryStepper.run();
    }
    int8_t LightgateStatus = lightgate.CheckPrimaryStatus();
    return LightgateStatus;
}

void Brake::openSecondaryBrake()
{
}

void Brake::closePrimaryBrake()
{
    primaryStepper.move(-primaryStepsNeeded);
}

void Brake::closeSecondaryBrake()
{
}