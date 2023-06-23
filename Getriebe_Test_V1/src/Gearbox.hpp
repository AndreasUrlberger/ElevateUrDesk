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
    static constexpr float maxDeskMotorSpeed{1500.f};        // max speed of main motor
    static constexpr float maxDeskMotorAcceleration{250.f}; // max acceleration of main motor

    DeskMotor deskMotor{maxDeskMotorSpeed, maxDeskMotorAcceleration};

#pragma region REMOVED
    // std::string nameOfState(GearboxState state);

    // std::string name{};
    // Lightgate lightgate{};
    // Brake brake{};
    // GearboxState state{};
    // float sensorHeight{};
    // float mathematicalHeight{};
#pragma endregion REMOVED

public:
    Gearbox(std::string gearboxName, float sensorHeight, float mathematicalHeight);
    ~Gearbox();

    void initMotor();
    void startMotor();
    void stopMotor();

    void moveUp(uint32_t penalty);
    void moveDown(uint32_t penalty);
    void moveToPosition(long targetPosition);

    uint32_t getCurrentPosition();

#pragma region REMOVED
    // void setup();
    // void run(int currentPosition, int requestedPosition, int stepDeviation);
    // void powerLoss();
    // void status();
    // void standby();
    // void stop();
    // int computeNewSpeed(int stepDeviation, int currentSpeed);
    // int computeTargetPosition(int stepDeviation, int currentSpeed);
    // void updateMotorSpeed(int speed);
#pragma endregion REMOVED
};
