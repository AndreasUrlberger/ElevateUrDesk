#include "GearboxCommunication.hpp"

GearboxCommunication::GearboxCommunication(const uint8_t gearboxLeftAddress, const uint8_t gearboxRightAddress, TwoWire *i2c, const int i2cSdaPin, const int i2cSclPin, const uint32_t i2cFrequency) : addressLeft(gearboxLeftAddress), addressRight(gearboxRightAddress), i2c(i2c)
{
    // Initialize I2C bus.
    Serial.println("Initializing Gearbox I2C bus");
    const bool i2cSuccess = i2c->begin(i2cSdaPin, i2cSclPin, i2cFrequency);
    Serial.print("Gearbox I2C bus initialized: ");
    Serial.println(i2cSuccess ? "true" : "false");
}

void GearboxCommunication::sendCommand(uint8_t *data, const size_t dataLength, const bool isLeftGearbox)
{
    constexpr size_t RESPONSE_LENGTH{5u};
    const uint8_t address = isLeftGearbox ? addressLeft : addressRight;
    uint8_t response[RESPONSE_LENGTH] = {0u};

    i2c->beginTransmission(address);
    size_t bytesWritten{0u};
    while (bytesWritten < dataLength)
    {
        bytesWritten += i2c->write(&(data[bytesWritten]), dataLength - bytesWritten);
    }
    i2c->endTransmission();

    // Request a response from the gearbox.
    i2c->requestFrom(address, RESPONSE_LENGTH);

    // Read the response.
    size_t bytesRead{0u};
    while (bytesRead < RESPONSE_LENGTH)
    {
        if (i2c->available())
        {
            response[bytesRead] = i2c->read();
            bytesRead++;
        }
    }

    processResponse(response, isLeftGearbox);
}

void GearboxCommunication::processResponse(const uint8_t *const response, const bool isLeftGearbox)
{
    // Process the response.
    if (isLeftGearbox)
    {
        positionLeft = *reinterpret_cast<const uint32_t *>(response);
        brakeStateLeft = response[4u];
    }
    else
    {
        positionRight = *reinterpret_cast<const uint32_t *>(response);
        brakeStateRight = response[4u];
    }
}

void GearboxCommunication::driveUp()
{
    constexpr size_t DATA_LENGTH{5u};
    // Save last position such that both gearboxes get the position from roughly the same time.
    const uint32_t lastPositionRight{positionRight};
    const uint32_t lastPositionLeft{positionLeft};

    uint8_t data[DATA_LENGTH] = {0u};
    // Set first byte to command code
    data[0u] = CMD_MOVE_UP;

    // Left
    // Set last 4 bytes to position of right gearbox
    *reinterpret_cast<uint32_t *>(&(data[1u])) = lastPositionRight;
    sendCommand(data, DATA_LENGTH, true);

    // Right
    // Set last 4 bytes to position of left gearbox
    *reinterpret_cast<uint32_t *>(&(data[1u])) = lastPositionLeft;
    sendCommand(data, DATA_LENGTH, false);
}

void GearboxCommunication::driveDown()
{
    constexpr size_t DATA_LENGTH{5u};
    // Save last position such that both gearboxes get the position from roughly the same time.
    const uint32_t lastPositionRight{positionRight};
    const uint32_t lastPositionLeft{positionLeft};

    uint8_t data[DATA_LENGTH] = {0u};
    // Set first byte to command code
    data[0u] = CMD_MOVE_DOWN;

    // Left
    // Set last 4 bytes to position of right gearbox
    *reinterpret_cast<uint32_t *>(&(data[1u])) = lastPositionRight;
    sendCommand(data, DATA_LENGTH, true);
    // Right
    // Set last 4 bytes to position of left gearbox
    *reinterpret_cast<uint32_t *>(&(data[1u])) = lastPositionLeft;
    sendCommand(data, DATA_LENGTH, false);
}

void GearboxCommunication::driveTo(const uint32_t position)
{
    constexpr size_t DATA_LENGTH{9u};
    // Save last position such that both gearboxes get the position from roughly the same time.
    const uint32_t lastPositionRight{positionRight};
    const uint32_t lastPositionLeft{positionLeft};

    uint8_t data[DATA_LENGTH] = {0u};
    // Set first byte to command code
    data[0u] = CMD_MOVE_TO;
    // Set bytes 1-4 to target position
    *reinterpret_cast<uint32_t *>(&(data[1u])) = position;

    // Left
    // Set last 4 bytes to position of right gearbox
    *reinterpret_cast<uint32_t *>(&(data[5u])) = lastPositionRight;
    sendCommand(data, DATA_LENGTH, true);
    // Right
    // Set last 4 bytes to position of left gearbox
    *reinterpret_cast<uint32_t *>(&(data[5u])) = lastPositionLeft;
    sendCommand(data, DATA_LENGTH, false);
}

void GearboxCommunication::emergencyStop()
{
    constexpr size_t DATA_LENGTH{5u};
    // Save last position such that both gearboxes get the position from roughly the same time.
    const uint32_t lastPositionRight{positionRight};
    const uint32_t lastPositionLeft{positionLeft};

    uint8_t data[DATA_LENGTH] = {0U};
    // Set first byte to command code
    data[0u] = CMD_EMERGENCY_STOP;

    // Left
    // Set last 4 bytes to position of right gearbox
    *reinterpret_cast<uint32_t *>(&(data[1u])) = lastPositionRight;
    sendCommand(data, DATA_LENGTH, true);

    // Right
    // Set last 4 bytes to position of left gearbox
    *reinterpret_cast<uint32_t *>(&(data[1u])) = lastPositionLeft;
    sendCommand(data, DATA_LENGTH, false);
}

void GearboxCommunication::getPosition()
{
    constexpr size_t DATA_LENGTH{5u};
    // Save last position such that both gearboxes get the position from roughly the same time.
    const uint32_t lastPositionRight{positionRight};
    const uint32_t lastPositionLeft{positionLeft};

    uint8_t data[DATA_LENGTH] = {0u};
    // Set first byte to command code
    data[0u] = CMD_GET_POSITION;

    // Left
    // Set last 4 bytes to position of right gearbox
    *reinterpret_cast<uint32_t *>(&(data[1u])) = lastPositionRight;
    sendCommand(data, DATA_LENGTH, true);
    // Right
    // Set last 4 bytes to position of left gearbox
    *reinterpret_cast<uint32_t *>(&(data[1u])) = lastPositionLeft;
    sendCommand(data, DATA_LENGTH, false);
}