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
    static Communication *instance;

    // Command codes for communication with general controller.
    static constexpr char CMD_MOVE_UP = 'u';
    static constexpr char CMD_MOVE_DOWN = 'd';
    static constexpr char CMD_MOVE_TO = 'm';
    static constexpr char CMD_EMERGENCY_STOP = 'e';
    static constexpr char CMD_GET_POSITION = 'p';

    // I2C settings for communication with general controller.
    static constexpr int I2C_SDA_PIN = 21;
    static constexpr int I2C_SCL_PIN = 22;
    static constexpr uint32_t I2C_FREQ = 100000u;
#ifdef GEARBOX_LEFT
    static constexpr int8_t I2C_ADDRESS = 0x33;
    static constexpr const char *const GEARBOX_NAME = "left";
#else
    static constexpr int8_t I2C_ADDRESS = 0x88;
    static constexpr const char *const GEARBOX_NAME = "right";
#endif

    static constexpr uint32_t MAX_GEARBOX_DEVIATION = 1000u;
    static constexpr uint32_t MAX_SOFT_GEARBOX_DEVIATION = 400u;

    Gearbox gearbox;
    std::string controlPanelMsgBuffer = "";
    uint8_t expectedMsgLength = 0;

    // Variables for I2C communication with general controller.
    static constexpr size_t MAX_EXPECTED_I2C_DATA_LENGTH = 16u;
    bool readAllI2cData{false};
    size_t i2cDataLength{0u};
    uint8_t i2cData[MAX_EXPECTED_I2C_DATA_LENGTH]{0u};

    uint32_t otherGearboxPosition{0u};

    // Checks if the two gearboxes deviate too far from each other and performs an emergency stop if they do. Returns true if the gearboxes are close enough to each other.bool checkForGearboxDeviation(uint32_t currentPosition)
    bool checkForGearboxDeviation(uint32_t currentPosition);
    uint32_t calculateCorrection(uint32_t deviation);
    void genCtrlMoveUp();
    void genCtrlMoveDown();
    void genCtrlMoveTo();
    void genCtrlEmergencyStop();
    void genCtrlGetPosition();

public:
    void performMoveTo(const long targetPosition);
    void performMoveUp(uint32_t penalty);
    void performMoveDown(uint32_t penalty);
    void performEmergencyStop();

    Communication(float gearboxSensorHeight, float gearboxMathematicalHeight);
    ~Communication() = default;

    // OnReceive function for I2C communication with general controller.
    void genCtrlOnReceiveI2C(int numBytes);

    // OnRequest function for I2C communication with general controller.
    void genCtrlOnRequestI2C();

    Gearbox *const getGearbox() { return &gearbox; }
};
