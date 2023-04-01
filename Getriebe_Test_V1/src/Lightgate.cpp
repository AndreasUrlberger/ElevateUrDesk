// Reads out if specific pins are HIGH or LOW in order to determine the state of connected lightgates. Returns true or false, depending on if the brake is open or not

#include "lightgate.hpp"

void Lightgate::setup()
{
    // Set specified pins to input
}

void Lightgate::CheckClosed()
{
}

bool Lightgate::CheckPrimaryClosed()
{
    // relevante Pins auslesen
    bool status = false;

    if (PrimarySensorOpen == HIGH && PrimarySensorClosed == LOW)
    {
        status = true;
    }
    else
    {
        status = false;
    }
    return status;
}

int8_t Lightgate::CheckPrimaryStatus()
{
    // relevante Pins auslesen
    int8_t status = -1;
    if (PrimarySensorOpen == HIGH && PrimarySensorClosed == LOW)
    {
        status = 0; // Brake is closed
    }
    else
    {
        if (PrimarySensorOpen == LOW && PrimarySensorClosed == HIGH)
        {
            status = 1; // Brake is open
        }
        else
        {
            status = -1; // Brake status is undefined (Brake is stuck between open and closed)
        }
    }
    return status;
}

void Lightgate::CheckSecondaryClosed()
{
}