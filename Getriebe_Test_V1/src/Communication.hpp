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
    Gearbox gearbox;

public:
    void moveTo(const long targetPosition);

    void moveUp();
    void moveDown();

    void emergencyStop();
    void initDeskMotor();

    Communication(std::string gearboxName, float gearboxSensorHeight, float gearboxMathematicalHeight);
    ~Communication() = default;

#pragma region REMOVED
    // bool receiveMessage(uint8_t (&message)[32]);
    // void handleRequest(uint8_t (&message)[32], uint8_t (&reply)[32]);
    // void handleSetupCommand(uint8_t (&message)[32], uint8_t (&reply)[32]);
    // void handleRunCommand(uint8_t (&message)[32], uint8_t (&reply)[32]);
    // void handleStopCommand(uint8_t (&message)[32], uint8_t (&reply)[32]);
    // void handleDebugCommand(uint8_t (&message)[32], uint8_t (&reply)[32]);
    // void toggleDebugMode(uint8_t (&message)[32], uint8_t (&reply)[32]);
    // void toggleUpdateMode(uint8_t (&message)[32], uint8_t (&reply)[32]);
    // void returnSoftwareVersion(uint8_t (&message)[32], uint8_t (&reply)[32]);
#pragma endregion REMOVED
};
