#pragma once

#include <Arduino.h>

class Example
{
private:
    /* data */
    int32_t exampleVariable = 0;

public:
    Example(/* args */) = default;
    ~Example() = default;

    void exampleFunc();
};
