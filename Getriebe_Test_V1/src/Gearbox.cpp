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

uint32_t Gearbox::getCurrentPosition(){
    return deskMotor.getCurrentPosition();
}



