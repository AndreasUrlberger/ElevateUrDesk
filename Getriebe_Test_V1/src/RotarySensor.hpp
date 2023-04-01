#pragma once

#include <Arduino.h>
#include "Pinout.hpp"

class RotarySensor
{
private:
    // maximaler Verfahrweg (mm), muss noch bestimmt werden!!!
    static constexpr float maxTravelDistance{700.0f};

    float corrected{0.0f};
    float linear{0.0f};
    float rawValue{0.0f};

public:
    RotarySensor(/* args */);
    ~RotarySensor();

    bool setup();
    float read();
    float readLinear();
    float readRaw();
};

RotarySensor::RotarySensor(/* args */)
{
}

RotarySensor::~RotarySensor()
{
}
