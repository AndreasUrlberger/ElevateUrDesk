#pragma once

#include "Arduino.h"
#include "DeskMotor.hpp"
#include "Communication.hpp"

class DebugControls
{
private:
    /* data */
    // Do not allow construction of this class
    DebugControls() = delete;
    ~DebugControls() = delete;

    static Communication *mCommunication;
    static DeskMotor *mDeskMotor;

public:
    static void handleDebugMode(int debugFeature);
    static void mainMenu();
    static void I2Cdebug();
    static void motorDebug();
    static void customMotorTest();

    static void debugButton1Press();

    static void initDebugControls(Communication *const communication);
};
