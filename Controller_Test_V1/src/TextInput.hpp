#pragma once

#include "Arduino.h"

class TextInput
{
private:
    // Do not allow construction of this class
    TextInput() = delete;
    ~TextInput() = delete;

    static constexpr byte numChars = 32;
    static char receivedChars[numChars]; // an array to store the received data

public:
    static int recvWithEndMarker();

    static int getIntInput();
};
