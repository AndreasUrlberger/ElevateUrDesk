#pragma once

#include <Arduino.h>
#include <AccelStepper.h>
#include <Wire.h>
#include "Pinout.hpp"
#include <string>
#include "Gearbox.hpp"

class Communication
{
    friend class DebugControls;
    friend class MainLogUtil;

private:
    static constexpr char CMD_MOVE_UP = 'u';
    static constexpr char CMD_MOVE_DOWN = 'd';
    static constexpr char CMD_MOVE_TO = 'm';
    static constexpr char CMD_EMERGENCY_STOP = 'e';

    Gearbox gearbox;
    std::string controlPanelMsgBuffer = "";
    uint8_t expectedMsgLength = 0;

public:
    void moveTo(const long targetPosition);

    void moveUp(uint32_t penalty);
    void moveDown(uint32_t penalty);

    void emergencyStop();
    void initDeskMotor();

    Communication(std::string gearboxName, float gearboxSensorHeight, float gearboxMathematicalHeight);
    ~Communication() = default;

    bool readControllerMessage();

    uint32_t getCurrentMotorPosition();
};
