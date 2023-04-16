#pragma once

#include <Arduino.h>
#include "Pinout.hpp"
#include <AccelStepper.h>
#include <atomic>

class DeskMotor
{
    friend class DebugControls;
    friend class MainLogUtil;

private:
    AccelStepper deskMotor{AccelStepper::HALF4WIRE, PrimaryBrake1, PrimaryBrake3, PrimaryBrake2, PrimaryBrake4};
    float maxSpeed{1500.f}; // max speed of main motor
    float maxAcceleration{100.f};
    /*volatile*/ long targetPosition{0}; // current target position of the motor
    bool isRunning{false};
    std::atomic_int skippedSteps{0};

    int getMissingSteps();
    long moveInputIntervalMS{10};

    float upDownStepBufferFactor{0.002f};
    // The number of step iterations after which the skipped steps are updated.
    static constexpr const long skippedStepsUpdateIteration{10000};
    // Current iteration counter.
    long iterationCounter{0};
    static const constexpr long iterationIntervalUS{10};

public:
    DeskMotor(const float maxSpeed, const float maxAcceleration);
    ~DeskMotor();

    static DeskMotor *instance;
    static hw_timer_t *timerHandle;

    void setMaxSpeed(const float newSpeed);
    void setMaxAcceleration(const float newAcceleration);
    long getCurrentPosition();
    void setNewTargetPosition(const long newTargetPosition);
    void addToTargetPosition(const long stepsToAdd);
    void setCurrentPosition(const long newPosition);
    void startTimer();

    void step();

    void start();
    void stop();

    void addSkippedSteps(const int stepsToAdd);

    void moveUp();
    void moveDown();
};
