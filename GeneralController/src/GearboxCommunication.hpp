#pragma once

#include <Arduino.h>
#include <Wire.h>

typedef uint8_t BrakeState;

class GearboxCommunication
{
private:
    static constexpr char CMD_MOVE_UP = 'u';
    static constexpr char CMD_MOVE_DOWN = 'd';
    static constexpr char CMD_MOVE_TO = 'm';
    static constexpr char CMD_EMERGENCY_STOP = 'e';
    static constexpr char CMD_GET_POSITION = 'p';
    static constexpr char CMD_LOOSEN_BRAKE = 'l';
    static constexpr char CMD_FASTEN_BRAKE = 'f';

    const uint8_t addressLeft{};
    const uint8_t addressRight{};
    TwoWire *const i2c{};
    uint32_t positionLeft{0u};
    uint32_t positionRight{0u};
    uint8_t brakeStateLeft{BRAKE_STATE_LOCKED};
    uint8_t brakeStateRight{BRAKE_STATE_LOCKED};

    void sendCommand(uint8_t *data, const size_t dataLength, const bool isLeftGearbox);
    void processResponse(const uint8_t *const response, const bool isLeftGearbox);

public:
    static constexpr BrakeState BRAKE_STATE_LOCKED = 0;
    static constexpr BrakeState BRAKE_STATE_INTERMEDIARY = 1;
    static constexpr BrakeState BRAKE_STATE_UNLOCKED = 3;
    static constexpr BrakeState BRAKE_STATE_ERROR = 2;

    GearboxCommunication(const uint8_t gearboxLeftAddress, const uint8_t gearboxRightAddress, TwoWire *i2c, const int i2cSdaPin, const int i2cSclPin, const uint32_t i2cFrequency);
    ~GearboxCommunication() = default;

    void driveUp();
    void driveDown();
    void driveTo(const uint32_t position);
    void emergencyStop();
    void getPosition();
    void loosenBrake();
    void fastenBrake();

    uint32_t getPositionLeft() const { return positionLeft; };
    uint32_t getPositionRight() const { return positionRight; };
    uint8_t getBrakeStateLeft() const { return brakeStateLeft; };
    uint8_t getBrakeStateRight() const
    {
        // TODO DEBUGGING ONLY, FIX THIS
        // return brakeStateRight;
        return brakeStateLeft;
    };
};