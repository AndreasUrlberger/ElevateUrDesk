#include "RotarySensor.hpp"
#include "AS5600.h"
#include "Wire.h"

AS5600 as5600; //  use default Wire

float maxTravelDistance = 700.0; // maximaler Verfahrweg (mm), muss noch bestimmt werden!!!
bool setupFinished = false;

bool RotarySensor::setup() // sets up the rotary sensor
{
    Wire.begin(RotarySDA, RotarySCL); // look up which pins the as5600 library uses !!!
    // maybe self test?
    setupFinished = true;

    return true;
}

float RotarySensor::read() // returns corrected value (degrees?)
{
    float uncorrected = readRaw();
    // float corrected = Hysteresekurve ausgleichen + Startpunkt setzen (damit 0 Grad ganz unten, 360 Grad ganz oben)
    float corrected = uncorrected;  // for debugging purposes only
    Serial.printf("corrected angle: %f\n", corrected);
    return corrected;
}

float RotarySensor::readLinear() // returns corrected height in mm
{
    corrected = read();                             // Drehwinkel in Grad
    linear = (corrected / 360) * maxTravelDistance; // aktuelle Höhe in mm
    Serial.printf("Sensorhöhe: %f\n", linear);
    return linear;
}

float RotarySensor::readRaw() // returns uncorrected value
{
    if (setupFinished == false)
    {
        setup();
    }
    rawValue = as5600.readAngle();
    Serial.printf("uncorrected angle: %f\n", rawValue);
    return rawValue;
}
