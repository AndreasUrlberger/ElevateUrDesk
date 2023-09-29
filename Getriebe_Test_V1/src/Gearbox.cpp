#include "Gearbox.hpp"
#include "DeskMotor.hpp"

Gearbox::Gearbox(std::string gearboxName, float sensorHeight, float mathematicalHeight)
{
    pinMode(RELAY_3V, OUTPUT);
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
    if(largeBrake.getBrakeState() == Brake::BRAKE_STATE_UNLOCKED){
        return Brake::BRAKE_STATE_UNLOCKED;
    }else if (largeBrake.getBrakeState() == Brake::BRAKE_STATE_LOCKED){
        return Brake::BRAKE_STATE_LOCKED;
    }else{
        return Brake::BRAKE_STATE_ERROR;
    }
}

DeskMotor *const Gearbox::getDeskMotor()
{
    return &deskMotor;
}

Brake *const Gearbox::getLargeBrake()
{
    return &largeBrake;
}

void Gearbox::loosenBrakes()
{
    largeBrake.openBrake();
}

void Gearbox::fastenBrakes()
{
    largeBrake.closeBrake();
}

void Gearbox::toggleMotorControlPower(const bool enable)
{
    if (enable)
    {
        digitalWrite(RELAY_3V, HIGH);
    }
    else
    {
        digitalWrite(RELAY_3V, LOW);
    }
}

void Gearbox::toggleMotorControl(const bool enable)
{
    if (enable)
    {
        digitalWrite(DESK_MOTOR_EN_PIN, LOW);
    }
    else
    {
        digitalWrite(DESK_MOTOR_EN_PIN, HIGH);
    }
}