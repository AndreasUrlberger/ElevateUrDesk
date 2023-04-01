#pragma once

#include <Arduino.h>
#include <string>
#include "Lightgate.hpp"
#include "Brake.hpp"
#include "Pinout.hpp"

class Gearbox
{
    enum class GearboxState
    {
        READY,
        STANDBY,
        STOPPED,
        ERROR
    };

private:
    /* data */
    std::string name{};
    Lightgate lightgate{};
    Brake brake{};
    GearboxState state{};
    float sensorHeight{};
    float mathematicalHeight{};

    std::string nameOfState(GearboxState state);

public:
    Gearbox(std::string gearboxName, float sensorHeight, float mathematicalHeight);
    ~Gearbox();

    void Start();
    void Run(int currentPosition, int requestedPosition, int stepDeviation);
    void PowerLoss();
    void Status();
    void Standby();
    void Stop();
    int ComputeNewSpeed(int stepDeviation, int currentSpeed);
};
