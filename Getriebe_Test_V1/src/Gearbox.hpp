#pragma once

#include <Arduino.h>
#include <string>
#include "Lightgate.hpp"
#include "Brake.hpp"
#include "Pinout.hpp"
#include "DeskMotor.hpp"

class Gearbox
{
    friend class DebugControls;
    friend class MainLogUtil;

    enum class GearboxState
    {
        READY,
        STANDBY,
        STOPPED,
        ERROR
    };

private:
    static constexpr float maxDeskMotorSpeed{1000.f};       // max speed of main motor
    static constexpr float maxDeskMotorAcceleration{100.f}; // max acceleration of main motor

    DeskMotor deskMotor{maxDeskMotorSpeed, maxDeskMotorAcceleration};

    Brake smallBrake{LIGHTGATE_SMALL_BRAKE_OPEN, LIGHTGATE_SMALL_BRAKE_CLOSED, SMALL_BRAKE_1, SMALL_BRAKE_2, SMALL_BRAKE_3, SMALL_BRAKE_4};
    Brake largeBrake{LIGHTGATE_LARGE_BRAKE_OPEN, LIGHTGATE_LARGE_BRAKE_CLOSED, LARGE_BRAKE_1, LARGE_BRAKE_2, LARGE_BRAKE_3, LARGE_BRAKE_4};

public:
    Gearbox(std::string gearboxName, float sensorHeight, float mathematicalHeight);
    ~Gearbox();

    void startMotor();
    void stopMotor();

    void moveUp(uint32_t penalty);
    void moveDown(uint32_t penalty);
    void moveToPosition(long targetPosition);
    void loosenBrakes();
    void fastenBrakes();

    uint32_t getCurrentPosition();
    BrakeState getCurrentBrakeState() const;

    DeskMotor *const getDeskMotor();
    Brake *const getSmallBrake();
    Brake *const getLargeBrake();

    void enableMotorPower();
    void disableMotorPower();
};
