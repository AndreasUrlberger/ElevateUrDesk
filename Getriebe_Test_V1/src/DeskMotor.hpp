#pragma once

#include <Arduino.h>
#include "Pinout.hpp"
#include <AccelStepper.h>
#include <atomic>

class DeskMotor
{
private:
    AccelStepper deskMotor{AccelStepper::HALF4WIRE, PrimaryBrake1, PrimaryBrake3, PrimaryBrake2, PrimaryBrake4}; // TODO: disable when debugging is done
    float maxSpeed{500.f};                                                                                       // max speed of main motor
    float maxAcceleration{100.f};
    long targetPosition = 0; // current target position of the motor
    bool isRunning{false};
    std::atomic_int skippedSteps{0};

    int getMissingSteps();
    long skippedStepsUpdateIntervalMS{10};
    long moveInputIntervalMS{10};

public:
    DeskMotor(const float maxSpeed, const float maxAcceleration);
    ~DeskMotor();

    void setup();
    void run(const int newSpeed);
    void setMaxSpeed(const float newSpeed);
    void setMaxAcceleration(const float newAcceleration);
    long getCurrentPosition();
    void setNewTargetPosition(const long newTargetPosition);
    void addToTargetPosition(const long stepsToAdd);

    void start();
    void stop();

    void updateMotorSpeed(const int speed);
    void addSkippedSteps(const int stepsToAdd);

    void DeskMotor::moveUp();
    void DeskMotor::moveDown();
};
