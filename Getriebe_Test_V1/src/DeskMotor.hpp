#pragma once

#include <Arduino.h>
#include <AccelStepper.h>
#include "Pinout.hpp"

class DeskMotor
{
private:
    /* data */
    AccelStepper deskMotor{AccelStepper::DRIVER, MotorStep, MotorDir};
    float maxSpeed = 500;        // max speed of main motor
    float maxAcceleration = 100; // max acceleration of main motor

public:
    DeskMotor(const float maxSpeed, const float maxAcceleration);
    ~DeskMotor();

    void setup();
    void run(const int targetPosition, const int newSpeed);
    void getSkippedSteps();

    void setMaxSpeed(const float newSpeed);
    void setMaxAcceleration(const float newAcceleration);
};

DeskMotor::DeskMotor(const float maxSpeed, const float maxAcceleration) : maxSpeed(maxSpeed), maxAcceleration(maxAcceleration)
{
}

DeskMotor::~DeskMotor()
{
}
