#pragma once

#include <Arduino.h>
#include "Pinout.hpp"
#include <TMCStepper.h>
#include <AccelStepper.h>
#include <atomic>

class DeskMotor
{
    friend class DebugControls;
    friend class MainLogUtil;

private:
    TMC2130Stepper driver = TMC2130Stepper(DESK_MOTOR_CS_PIN, DESK_MOTOR_R_SENSE); // Hardware SPI
    AccelStepper deskMotor = AccelStepper(deskMotor.DRIVER, DESK_MOTOR_STEP_PIN, DESK_MOTOR_DIR_PIN);

    float maxSpeed{}; // max speed of main motor
    float maxAcceleration{};
    /*volatile*/ long targetPosition{0}; // current target position of the motor
    bool isRunning{false};
    std::atomic_int skippedSteps{0};

    int getMissingSteps();
    // Calculates the number of steps for the given speed and the given time frame.
    long calculateDeltaSteps(float currentSpeed);
    long moveInputIntervalMS{20};

    float upDownStepBufferFactor{0.1f};
    // The number of step iterations after which the skipped steps are updated.
    static constexpr const long skippedStepsUpdateIteration{1000};
    // Current iteration counter.
    long iterationCounter{0};

public:
    DeskMotor(const float maxSpeed, const float maxAcceleration);
    ~DeskMotor() = default;

    void setMaxSpeed(const float newSpeed);
    void setMaxAcceleration(const float newAcceleration);
    uint32_t getCurrentPosition();
    int32_t getCurrentSpeed();
    bool isMotorMovingUpwards();
    bool isMotorMovingDownwards();
    void setNewTargetPosition(const long newTargetPosition);
    void addToTargetPosition(const long stepsToAdd);
    void setCurrentPosition(const long newPosition);

    void step();

    void start();
    void stop();

    void addSkippedSteps(const int stepsToAdd);

    void moveUp(uint32_t penalty);
    void moveDown(uint32_t penalty);

    uint32_t hwReadSkippedSteps();
};
