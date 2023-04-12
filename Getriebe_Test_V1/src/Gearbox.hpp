#pragma once

#include <Arduino.h>
#include <string>
#include "Lightgate.hpp"
#include "Brake.hpp"
#include "Pinout.hpp"
#include "DeskMotor.hpp"
#include "DebugControls.hpp"

class Gearbox
{
    friend DebugControls;

    enum class GearboxState
    {
        READY,
        STANDBY,
        STOPPED,
        ERROR
    };

private:
    static constexpr float maxDeskMotorSpeed{500.f};        // max speed of main motor
    static constexpr float maxDeskMotorAcceleration{100.f}; // max acceleration of main motor

    /* data */
    std::string name{};
    Lightgate lightgate{};
    DeskMotor deskMotor{maxDeskMotorSpeed, maxDeskMotorAcceleration};
    Brake brake{};
    GearboxState state{};
    float sensorHeight{};
    float mathematicalHeight{};

    std::string nameOfState(GearboxState state);

public:
    Gearbox(std::string gearboxName, float sensorHeight, float mathematicalHeight);
    ~Gearbox();

    void setup();
    void run(int currentPosition, int requestedPosition, int stepDeviation);
    void powerLoss();
    void status();
    void standby();
    void stop();
    int computeNewSpeed(int stepDeviation, int currentSpeed);
    int computeTargetPosition(int stepDeviation, int currentSpeed);

    void updateMotorSpeed(int speed);

    void startMotor();
    void stopMotor();

    void moveUp();
    void moveDown();
    void moveToPosition(long targetPosition);
};
