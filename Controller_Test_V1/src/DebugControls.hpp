#pragma once

#include "Arduino.h"

class DebugControls
{
private:
    // Do not allow construction of this class
    DebugControls() = delete;
    ~DebugControls() = delete;

public:
    static void handleDebugMode(int debugFeature);
    static void I2Cdebug();
};
