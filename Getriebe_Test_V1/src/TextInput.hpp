#pragma once

#include "Arduino.h"

class TextInput
{
private:
    /* data */
    static const byte numChars = 32;
    static char receivedChars[numChars]; // an array to store the received data
public:
    // Do not allow construction of this class
    TextInput(/* args */);
    ~TextInput();

    static int recvWithEndMarker();

    static int getIntInput();
};
