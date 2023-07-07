#pragma once

#include <Arduino.h>

class Lightgate
{
private:
    const uint8_t brakePin{};

public:
    Lightgate(const uint8_t brakePin);
    ~Lightgate() = default;

    bool isLightgateBlocked() const;
};