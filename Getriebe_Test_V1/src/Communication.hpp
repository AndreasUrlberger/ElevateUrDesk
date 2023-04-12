#pragma once

#include <Arduino.h>
#include <AccelStepper.h>
#include <Wire.h>
#include "Pinout.hpp"
#include <string>
#include "Gearbox.hpp"
#include "DebugControls.hpp"

class Communication
{
    friend DebugControls;

private:
    Gearbox gearbox;

public:
    void run(int speed);

    bool receiveMessage(uint8_t (&message)[32]);
    void handleRequest(uint8_t (&message)[32], uint8_t (&reply)[32]);

    void handleSetupCommand(uint8_t (&message)[32], uint8_t (&reply)[32]);
    void handleRunCommand(uint8_t (&message)[32], uint8_t (&reply)[32]);
    void handleStopCommand(uint8_t (&message)[32], uint8_t (&reply)[32]);
    void handleDebugCommand(uint8_t (&message)[32], uint8_t (&reply)[32]);

    void toggleDebugMode(uint8_t (&message)[32], uint8_t (&reply)[32]);
    void toggleUpdateMode(uint8_t (&message)[32], uint8_t (&reply)[32]);
    void returnSoftwareVersion(uint8_t (&message)[32], uint8_t (&reply)[32]);

    void emergencyStop();

    Communication(std::string gearboxName, float gearboxSensorHeight, float gearboxMathematicalHeight);
    ~Communication() = default;
};
