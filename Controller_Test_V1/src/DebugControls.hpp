#pragma once

#include "Arduino.h"

class DebugControls
{
private:
    /* data */
public:
    // Do not allow construction of this class
    DebugControls(/* args */);
    ~DebugControls();

    static void handleDebugMode(int debugFeature);
    static void I2Cdebug();
};
