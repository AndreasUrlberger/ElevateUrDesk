
#include "Lightgate.hpp"

Lightgate::Lightgate(const uint8_t brakePin) : brakePin(brakePin)
{
    pinMode(brakePin, INPUT);
}

bool Lightgate::isLightgateBlocked() const
{
    return digitalRead(brakePin) == HIGH;
}
