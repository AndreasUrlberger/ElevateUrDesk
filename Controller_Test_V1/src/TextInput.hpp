#pragma once

#include "Arduino.h"

class TextInput
{
private:
    /* data */
public:
    // Do not allow construction of this class
    TextInput(/* args */);
    ~TextInput();

    static int recvWithEndMarker();

    static int getIntInput();
};
