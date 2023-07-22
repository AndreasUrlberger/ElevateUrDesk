#include "DeskMotor.hpp"

#include <chrono>
#include <thread>
#include <esp_task_wdt.h>

DeskMotor::DeskMotor(const float maxSpeed, const float maxAcceleration) : maxSpeed(maxSpeed), maxAcceleration(maxAcceleration)
{
    SPI.begin(DESK_MOTOR_SPI_SCK, DESK_MOTOR_SPI_MISO, DESK_MOTOR_SPI_MOSI, DESK_MOTOR_SPI_SS);

    pinMode(DESK_MOTOR_CS_PIN, OUTPUT);
    digitalWrite(DESK_MOTOR_CS_PIN, LOW);

    driver.begin();           // Initiate pins and registeries
    driver.rms_current(600);  // Set stepper current to 600mA. The command is the same as command TMC2130.setCurrent(600, 0.11, 0.5);
    driver.en_pwm_mode(true); // Enable extremely quiet stepping
    driver.pwm_autoscale(true);
    driver.microsteps(0); // We need to set zero microsteps for the step multiplier(each step translates to 256 microsteps) to work.
    driver.intpol(true);  // Enable interpolation for step multiplier

    deskMotor.setCurrentPosition(0);
    deskMotor.setMaxSpeed(maxSpeed);
    deskMotor.setAcceleration(maxAcceleration);
    deskMotor.setEnablePin(DESK_MOTOR_EN_PIN);
    deskMotor.setPinsInverted(false, false, true);
    deskMotor.enableOutputs();

    Serial.printf("Main motor initialized\n");
}

void DeskMotor::step()
{
    if (isRunning)
    {
        deskMotor.run();
    }

    iterationCounter++;
    if (iterationCounter >= skippedStepsUpdateIteration)
    {
        iterationCounter = 0;

        // Subtract skipped steps from current position
        const int skippedSteps = getMissingSteps();
        deskMotor.fixMissingSteps(skippedSteps);

// Update to new target position
#ifdef GEARBOX_LEFT
        deskMotor.moveTo(targetPosition);
#else
        deskMotor.moveTo(-targetPosition);
#endif
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

uint32_t DeskMotor::getCurrentPosition()
{
#ifdef GEARBOX_LEFT
    return deskMotor.currentPosition();
#else
    return -deskMotor.currentPosition();
#endif
}

int32_t DeskMotor::getCurrentSpeed()
{
#ifdef GEARBOX_LEFT
    return deskMotor.speed();
#else
    return -deskMotor.speed();
#endif
}

bool DeskMotor::isMotorMovingUpwards()
{
    // Explicitly not mark the motor as moving downwards if it is not moving at all.
#ifdef GEARBOX_LEFT
    return deskMotor.distanceToGo() > 0;
#else
    return deskMotor.distanceToGo() < 0;
#endif
}

bool DeskMotor::isMotorMovingDownwards()
{
    // Explicitly not mark the motor as moving downwards if it is not moving at all.
#ifdef GEARBOX_LEFT
    return deskMotor.distanceToGo() < 0;
#else
    return deskMotor.distanceToGo() > 0;
#endif
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

void DeskMotor::addSkippedSteps(const int stepsToAdd)
{
    // Add the number of steps atomically as the motor might reset it to 0.
    skippedSteps.fetch_add(stepsToAdd);
}

int DeskMotor::getMissingSteps()
{
    // Atomically replace the number of skipped steps as another thread might add to it.
    return skippedSteps.exchange(0);
}

void DeskMotor::setCurrentPosition(const long newPosition)
{
#ifdef GEARBOX_LEFT
    deskMotor.setCurrentPosition(newPosition);
#else
    deskMotor.setCurrentPosition(-newPosition);
#endif
}

void DeskMotor::moveUp(uint32_t penalty)
{
    if (isMotorMovingDownwards())
    {
        // The motor is currently moving downwards, therefore, do nothing.
        return;
    }

    const float currentSpeed = getCurrentSpeed();
    const long currentPosition = getCurrentPosition();
    const long deltaSteps = calculateDeltaSteps(currentSpeed);
    const long adjustedDeltaSteps = max(deltaSteps - static_cast<long>(penalty), 0L);
    // Add delta steps calculated for the given speed.
    const long targetPosition = currentPosition + adjustedDeltaSteps;

    // Set target position.
    setNewTargetPosition(targetPosition);
}

void DeskMotor::moveDown(uint32_t penalty)
{
    if (isMotorMovingUpwards())
    {
        // The motor is currently moving upwards, therefore, do nothing.
        return;
    }

    // Take the negative speed because then we can use the same calculation as for moving upwards.
    const float currentSpeed = -getCurrentSpeed();
    const long currentPosition = getCurrentPosition();
    const long deltaSteps = calculateDeltaSteps(currentSpeed);
    const long adjustedDeltaSteps = max(deltaSteps - static_cast<long>(penalty), 0L);
    // Subtract delta steps calculated for the given speed to account for the inverted speed.
    const long targetPosition = currentPosition - adjustedDeltaSteps;

    // Set target position.
    setNewTargetPosition(targetPosition);
}

uint32_t DeskMotor::hwReadSkippedSteps()
{
    return driver.LOST_STEPS();
}

long DeskMotor::calculateDeltaSteps(float currentSpeed)
{
    const long currentPosition = getCurrentPosition();

    const float maxPotAcceleration = maxAcceleration * moveInputIntervalMS / 1000;
    const float theoreticalEndSpeed = currentSpeed + maxPotAcceleration;

    float actualEndSpeed;
    float totalSteps = 0;

    if (theoreticalEndSpeed > maxSpeed)
    {
        // Speed is capped.
        actualEndSpeed = maxSpeed;
        // What percentage of time the acceleration was active for?
        const float activeAccelerationPercentage = (theoreticalEndSpeed - actualEndSpeed) / maxPotAcceleration;

        // Triangular rise till max speed (platoe).
        const float actualAcceleration = maxPotAcceleration * activeAccelerationPercentage;
        const float accelerationTime = moveInputIntervalMS * activeAccelerationPercentage / 1000;
        const float triangleRiseSteps = 0.5 * actualAcceleration * accelerationTime;
        // Rectangular plateau at max speed (platoe).
        const float plateauTime = (moveInputIntervalMS / 1000.0f - accelerationTime);
        const float rectangleSteps = actualAcceleration * plateauTime;

        totalSteps = triangleRiseSteps + rectangleSteps;
    }
    else
    {
        // Speed is not capped.
        actualEndSpeed = theoreticalEndSpeed;
        const float activeAccelerationPercentage = 1;

        // Triangular rise till.
        const float triangleRiseSteps = 0.5 * maxPotAcceleration * moveInputIntervalMS / 1000;

        totalSteps = triangleRiseSteps;
    }

    // Base rectangle.
    const float baseRectangleSteps = currentSpeed * moveInputIntervalMS / 1000;

    // Triangular decrease till halt.
    const float decelerationTime = actualEndSpeed / maxAcceleration; // seconds
    const float triangleFallSteps = 0.5 * actualEndSpeed * decelerationTime;

    totalSteps += baseRectangleSteps + triangleFallSteps;
    const float bufferSteps = totalSteps * upDownStepBufferFactor;
    const float deltaSteps = max(10.0f, ceil(totalSteps + bufferSteps));

    return deltaSteps;
}