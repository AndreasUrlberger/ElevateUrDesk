#pragma once

#include <Arduino.h>
#include <Wire.h>

class GearboxCommunication
{
private:
    static constexpr char CMD_MOVE_UP = 'u';
    static constexpr char CMD_MOVE_DOWN = 'd';
    static constexpr char CMD_MOVE_TO = 'm';
    static constexpr char CMD_EMERGENCY_STOP = 'e';
    static constexpr char CMD_GET_POSITION = 'p';

    const uint8_t addressLeft{};
    const uint8_t addressRight{};
    TwoWire *const i2c{};
    uint32_t positionLeft{0u};
    uint32_t positionRight{0u};

    void sendCommand(uint8_t *data, const size_t dataLength, const size_t responseLength, uint8_t *response, const uint8_t address);

public:
    GearboxCommunication(const uint8_t gearboxLeftAddress, const uint8_t gearboxRightAddress, TwoWire *i2c, const int i2cSdaPin, const int i2cSclPin, const uint32_t i2cFrequency);
    ~GearboxCommunication() = default;

    void driveUp();
    void driveDown();
    void driveTo(const uint32_t position);
    void emergencyStop();
    void getPosition();

    uint32_t getPositionLeft() const { return positionLeft; };
    uint32_t getPositionRight() const { return positionRight; };
};