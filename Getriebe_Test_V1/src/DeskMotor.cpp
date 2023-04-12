#include "DeskMotor.hpp"

DeskMotor::DeskMotor(const float maxSpeed, const float maxAcceleration) : maxSpeed(maxSpeed), maxAcceleration(maxAcceleration)
{
}

DeskMotor::~DeskMotor()
{
}

void DeskMotor::setup()
{
    // Powering on the motor controller by turning on the relay(s) in the right order
    // SPI communication with motor controller
    deskMotor.setMaxSpeed(maxSpeed);
    deskMotor.setAcceleration(maxAcceleration);
    deskMotor.setCurrentPosition(0); // TODO: should be lastCurrentPosition
    Serial.printf("Main motor initialized");
}

void DeskMotor::run(int newSpeed) // has to run on different core so that it is non-blocking ?!?
{
    // set target position
    if (debugMode)
    {
        Serial.printf("Current position: %d \n", deskMotor.currentPosition());
        Serial.printf("steps to old target position: %d \n", deskMotor.distanceToGo());
    }
    deskMotor.moveTo(targetPosition);
    deskMotor.setMaxSpeed(newSpeed);
    // newSpeed is the computed speed to resync the two gearboxes

    if (debugMode)
    {
        Serial.printf("Target position set to %d \n", targetPosition);
    }

    long lastTime = millis();
    while (true)
    {
        if (isRunning && (deskMotor.distanceToGo() != 0))
        {
            deskMotor.run();
        }

        const long currentTime = millis();
        // Take absolute value of difference to handle overflow.
        const long diffTime = abs(currentTime - lastTime);

        if (diffTime >= skippedStepsUpdateIntervalMS)
        {
            lastTime = currentTime;

            // Subtract skipped steps from current position
            const int skippedSteps = getMissingSteps();
            deskMotor.fixMissingSteps(skippedSteps);

            // Update to new target position
            deskMotor.moveTo(targetPosition);
        }
    }
}

void DeskMotor::setMaxAcceleration(const float newMaxAcceleration)
{
    maxAcceleration = newMaxAcceleration;
    deskMotor.setAcceleration(maxAcceleration);
}

void DeskMotor::setMaxSpeed(const float newMaxSpeed)
{
    maxSpeed = newMaxSpeed;
    deskMotor.setMaxSpeed(maxSpeed);
}

long DeskMotor::getCurrentPosition()
{
    return deskMotor.currentPosition();
}

void DeskMotor::setNewTargetPosition(const long newTargetPosition)
{
    targetPosition = constrain(newTargetPosition, minSteps, maxSteps);
}

void DeskMotor::addToTargetPosition(const long stepsToAdd)
{
    targetPosition = constrain(targetPosition + stepsToAdd, minSteps, maxSteps);
}

void DeskMotor::start()
{
    isRunning = true;
}

void DeskMotor::stop()
{
    isRunning = false;
}

void DeskMotor::updateMotorSpeed(const int speed)
{
    deskMotor.setMaxSpeed(speed);
}

void DeskMotor::addSkippedSteps(const int stepsToAdd)
{
    // Add the number steps atomically as the motor might reset it to 0.
    skippedSteps.fetch_add(stepsToAdd);
}

int DeskMotor::getMissingSteps()
{
    // Atomically replace the number of skipped steps as another thread might add to it.
    return skippedSteps.exchange(0);
}

void DeskMotor::moveUp()
{
    const bool isMovingDownwards = deskMotor.distanceToGo() < 0;
    if (isMovingDownwards)
    {
        // The motor is currently moving downwards, therefore, do nothing.
        return;
    }

    // Calculate target position based on current position and speed.
    const long currentPosition = deskMotor.currentPosition();
    const float currentSpeed = deskMotor.speed();
    const float expectedEndSpeed = currentSpeed + maxAcceleration * moveInputIntervalMS / 1000;
    const float cappedEndSpeed = min(expectedEndSpeed, maxSpeed);

    // Triangular rise till max speed.
    const float timeTillMaxSpeed = (expectedEndSpeed - cappedEndSpeed) / cappedEndSpeed;
    const float triangleRiseSteps = 0;
    // Rectangular plateau at max speed.
    const float rectangleSteps = 0;
    // Triangular decrease till halt.
    const float triangleFallSteps = 0;

    const long deltaSteps = triangleRiseSteps + rectangleSteps + triangleFallSteps;
    const long bufferSteps = deltaSteps * 0.002;
    const long targetPosition = currentPosition + deltaSteps + bufferSteps;

    // TODO Does this work if we are close to the end of the desk?

    // Set target position.
    setNewTargetPosition(targetPosition);
}

void DeskMotor::moveDown()
{
    // Calculate target position based on current position and speed.
    // Set target position.
}