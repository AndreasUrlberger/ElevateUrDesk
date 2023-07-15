#include "Gearbox.hpp"
#include "DeskMotor.hpp"

Gearbox::Gearbox(std::string gearboxName, float sensorHeight, float mathematicalHeight)
{
}

Gearbox::~Gearbox()
{
}

void Gearbox::startMotor()
{
    deskMotor.start();
}

void Gearbox::stopMotor()
{
    deskMotor.stop();
}

void Gearbox::moveUp(uint32_t penalty)
{
    // Calculate target position based on current position and speed.
    // Set target position.
    deskMotor.moveUp(penalty);
}

void Gearbox::moveDown(uint32_t penalty)
{
    // Calculate target position based on current position and speed.
    // Set target position.
    deskMotor.moveDown(penalty);
}

void Gearbox::moveToPosition(long targetPosition)
{
    deskMotor.setNewTargetPosition(targetPosition);
}

uint32_t Gearbox::getCurrentPosition()
{
    return deskMotor.getCurrentPosition();
}

BrakeState Gearbox::getCurrentBrakeState() const
{
    // Print state of both brakes as text.
    Serial.print("Small brake state: ");
    switch (smallBrake.getBrakeState())
    {
    case Brake::BRAKE_STATE_LOCKED:
        Serial.print("Locked");
        break;
    case Brake::BRAKE_STATE_INTERMEDIARY:
        Serial.print("Intermediary");
        break;
    case Brake::BRAKE_STATE_UNLOCKED:
        Serial.print("Unlocked");
        break;
    case Brake::BRAKE_STATE_ERROR:
        Serial.print("Error");
        break;
    }
    Serial.print(" Large brake state: ");
    switch (largeBrake.getBrakeState())
    {
    case Brake::BRAKE_STATE_LOCKED:
        Serial.println("Locked");
        break;
    case Brake::BRAKE_STATE_INTERMEDIARY:
        Serial.println("Intermediary");
        break;
    case Brake::BRAKE_STATE_UNLOCKED:
        Serial.println("Unlocked");
        break;
    case Brake::BRAKE_STATE_ERROR:
        Serial.println("Error");
        break;
    }

    if (smallBrake.getBrakeState() == Brake::BRAKE_STATE_UNLOCKED && largeBrake.getBrakeState() == Brake::BRAKE_STATE_UNLOCKED)
    {
        return Brake::BRAKE_STATE_UNLOCKED;
    }
    else if (smallBrake.getBrakeState() == Brake::BRAKE_STATE_LOCKED && largeBrake.getBrakeState() == Brake::BRAKE_STATE_LOCKED)
    {
        return Brake::BRAKE_STATE_LOCKED;
    }
    else if (smallBrake.getBrakeState() == Brake::BRAKE_STATE_ERROR || largeBrake.getBrakeState() == Brake::BRAKE_STATE_ERROR)
    {
        return Brake::BRAKE_STATE_ERROR;
    }
    else
    {
        return Brake::BRAKE_STATE_INTERMEDIARY;
    }
}

DeskMotor *const Gearbox::getDeskMotor()
{
    return &deskMotor;
}

Brake *const Gearbox::getSmallBrake()
{
    return &smallBrake;
}

Brake *const Gearbox::getLargeBrake()
{
    return &largeBrake;
}

void Gearbox::loosenBrakes()
{
    smallBrake.openBrake();
    largeBrake.openBrake();
}

void Gearbox::fastenBrakes()
{
    smallBrake.closeBrake();
    largeBrake.closeBrake();
}