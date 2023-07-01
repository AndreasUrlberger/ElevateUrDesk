#include "GearboxCommunication.hpp"

GearboxCommunication::GearboxCommunication(const uint8_t gearboxLeftAddress, const uint8_t gearboxRightAddress, TwoWire *i2c, const int i2cSdaPin, const int i2cSclPin, const uint32_t i2cFrequency) : addressLeft(gearboxLeftAddress), addressRight(gearboxRightAddress), i2c(i2c)
{
    // Initialize I2C bus.
    Serial.println("Initializing Gearbox I2C bus");
    const bool i2cSuccess = i2c->begin(i2cSdaPin, i2cSclPin, i2cFrequency);
    Serial.print("Gearbox I2C bus initialized: ");
    Serial.println(i2cSuccess ? "true" : "false");
}

void GearboxCommunication::sendCommand(uint8_t *data, const size_t dataLength, const size_t responseLength, uint8_t *response, const uint8_t address)
{
    i2c->beginTransmission(address);
    size_t bytesWritten{0u};
    while (bytesWritten < dataLength)
    {
        bytesWritten += i2c->write(&(data[bytesWritten]), dataLength - bytesWritten);
    }
    i2c->endTransmission();

    // Request a response from the gearbox.
    i2c->requestFrom(address, responseLength);

    // Read the response.
    size_t bytesRead{0u};
    while (bytesRead < responseLength)
    {
        if (i2c->available())
        {
            response[bytesRead] = i2c->read();
            bytesRead++;
        }
    }
}

void GearboxCommunication::driveUp()
{
    constexpr size_t DATA_LENGTH{5u};
    uint8_t data[DATA_LENGTH] = {0u};
    // Set first byte to command code
    data[0u] = CMD_MOVE_UP;
    uint32_t currentPositionLeft{};
    uint32_t currentPositionRight{};

    // Left
    // Set last 4 bytes to position of right gearbox
    *reinterpret_cast<uint32_t *>(&(data[1u])) = positionRight;
    sendCommand(data, DATA_LENGTH, 4u, reinterpret_cast<uint8_t *>(&currentPositionLeft), addressLeft);
    // Right
    // Set last 4 bytes to position of left gearbox
    *reinterpret_cast<uint32_t *>(&(data[1u])) = positionLeft;
    sendCommand(data, DATA_LENGTH, 4u, reinterpret_cast<uint8_t *>(&currentPositionRight), addressRight);

    positionLeft = currentPositionLeft;
    positionRight = currentPositionRight;
    Serial.print("Gearbox position left: ");
    Serial.print(positionLeft);
    Serial.print(" right: ");
    Serial.println(positionRight);
}

void GearboxCommunication::driveDown()
{
    constexpr size_t DATA_LENGTH{5u};
    uint8_t data[DATA_LENGTH] = {0u};
    // Set first byte to command code
    data[0u] = CMD_MOVE_DOWN;
    uint32_t currentPositionLeft{};
    uint32_t currentPositionRight{};

    // Left
    // Set last 4 bytes to position of right gearbox
    *reinterpret_cast<uint32_t *>(&(data[1u])) = positionRight;
    sendCommand(data, DATA_LENGTH, 4u, reinterpret_cast<uint8_t *>(&currentPositionLeft), addressLeft);
    // Right
    // Set last 4 bytes to position of left gearbox
    *reinterpret_cast<uint32_t *>(&(data[1u])) = positionLeft;
    sendCommand(data, DATA_LENGTH, 4u, reinterpret_cast<uint8_t *>(&currentPositionRight), addressRight);

    positionLeft = currentPositionLeft;
    positionRight = currentPositionRight;
    Serial.print("Gearbox position left: ");
    Serial.print(positionLeft);
    Serial.print(" right: ");
    Serial.println(positionRight);
}

void GearboxCommunication::driveTo(const uint32_t position)
{
    constexpr size_t DATA_LENGTH{9u};
    uint8_t data[DATA_LENGTH] = {0u};
    // Set first byte to command code
    data[0u] = CMD_MOVE_TO;
    uint32_t currentPositionLeft{};
    uint32_t currentPositionRight{};
    // Set bytes 1-4 to target position
    *reinterpret_cast<uint32_t *>(&(data[1u])) = position;

    // Left
    // Set last 4 bytes to position of right gearbox
    *reinterpret_cast<uint32_t *>(&(data[5u])) = positionRight;
    sendCommand(data, DATA_LENGTH, 4u, reinterpret_cast<uint8_t *>(&currentPositionLeft), addressLeft);
    // Right
    // Set last 4 bytes to position of left gearbox
    *reinterpret_cast<uint32_t *>(&(data[5u])) = positionLeft;
    sendCommand(data, DATA_LENGTH, 4u, reinterpret_cast<uint8_t *>(&currentPositionRight), addressRight);

    positionLeft = currentPositionLeft;
    positionRight = currentPositionRight;
    Serial.print("Gearbox position left: ");
    Serial.print(positionLeft);
    Serial.print(" right: ");
    Serial.println(positionRight);
}

void GearboxCommunication::emergencyStop()
{
    constexpr size_t DATA_LENGTH{1u};
    uint8_t data[DATA_LENGTH] = {CMD_EMERGENCY_STOP};
    uint32_t currentPositionLeft{};
    uint32_t currentPositionRight{};

    // Left
    sendCommand(data, 1u, 4u, reinterpret_cast<uint8_t *>(&currentPositionLeft), addressLeft);
    // Right
    sendCommand(data, 1u, 4u, reinterpret_cast<uint8_t *>(&currentPositionRight), addressRight);

    positionLeft = currentPositionLeft;
    positionRight = currentPositionRight;
    Serial.print("Gearbox position left: ");
    Serial.print(positionLeft);
    Serial.print(" right: ");
    Serial.println(positionRight);
}

void GearboxCommunication::getPosition()
{
    static const size_t DATA_LENGTH{5u};
    uint8_t data[DATA_LENGTH] = {0u};
    // Set first byte to command code
    data[0u] = CMD_GET_POSITION;
    uint32_t currentPositionLeft{};
    uint32_t currentPositionRight{};

    // Left
    // Set last 4 bytes to position of right gearbox
    *reinterpret_cast<uint32_t *>(&(data[1u])) = positionRight;
    sendCommand(data, 1u, 4u, reinterpret_cast<uint8_t *>(&currentPositionLeft), addressLeft);
    // Right
    // Set last 4 bytes to position of left gearbox
    *reinterpret_cast<uint32_t *>(&(data[1u])) = positionLeft;
    sendCommand(data, 1u, 4u, reinterpret_cast<uint8_t *>(&currentPositionRight), addressRight);

    positionLeft = currentPositionLeft;
    positionRight = currentPositionRight;
    Serial.print("Gearbox position left: ");
    Serial.print(positionLeft);
    Serial.print(" right: ");
    Serial.println(positionRight);
}