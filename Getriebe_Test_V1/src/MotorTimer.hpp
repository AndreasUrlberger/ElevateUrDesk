#pragma once

#include <Arduino.h>
#include <atomic>

#include "DeskMotor.hpp"
#include "Brake.hpp"

class MotorTimer
{
    static hw_timer_t *timerHandle;
    static constexpr long iterationIntervalUS{10};

    static DeskMotor *deskMotor;
    static Brake *brake1;
    static Brake *brake2;

public:
    static MotorTimer *instance;
    std::atomic_int dueTaskIterations;

    MotorTimer(DeskMotor *const deskMotor, Brake *const brake1, Brake *const brake2);
    ~MotorTimer() = default;

    void startTimer();
    void runTask();
};