#include "DeskMotor.hpp"

#include <chrono>
#include <thread>
#include <esp_task_wdt.h>

DeskMotor *DeskMotor::instance;
hw_timer_t *DeskMotor::timerHandle;

void IRAM_ATTR onDeskMotorTimer()
{
    DeskMotor::instance->dueTaskIterations.fetch_add(1);
}

DeskMotor::DeskMotor(const float maxSpeed, const float maxAcceleration) : maxSpeed(maxSpeed), maxAcceleration(maxAcceleration)
{
    Serial.println("DeskMotor constructor called.");
    instance = this;
    deskMotor.setCurrentPosition(0);
    deskMotor.setMaxSpeed(maxSpeed);
    deskMotor.setAcceleration(maxAcceleration);

    startTimer();

    Serial.printf("Main motor initialized\n");
}

DeskMotor::~DeskMotor()
{
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
        deskMotor.moveTo(targetPosition);
    }
}

void DeskMotor::runTask()
{
    Serial.println("RunTask started");

    esp_task_wdt_init(UINT32_MAX, false);
    esp_task_wdt_delete(NULL);

    Serial.println("WDT diabled on core 1");

    while (true)
    {
        if (dueTaskIterations.load() > 0)
        {
            dueTaskIterations.fetch_sub(1);
            step();
        }
    }
}

void DeskMotor::startTimer()
{
    // Create new task to handle the timer interrupt and start the desk motor task.
    Serial.println("Start timer/motor task.");
    xTaskCreatePinnedToCore(
        [](void *param)
        {
            // Runs on core 0.
            Serial.println("DeskMotorTimerTask is running on core " + String(xPortGetCoreID()) + ".");

            // The prescaler is used to divide the base clock frequency of the ESP32’s timer. The ESP32’s timer uses the APB clock (APB_CLK) as its base clock, which is normally 80 MHz. By setting the prescaler to 80, we are dividing the base clock frequency by 80, resulting in a timer tick frequency of 1 MHz (80 MHz / 80 = 1 MHz).
            DeskMotor::timerHandle = timerBegin(0, 80, true);
            timerAttachInterrupt(DeskMotor::timerHandle, &onDeskMotorTimer, true);
            timerAlarmWrite(DeskMotor::timerHandle, iterationIntervalUS, true);
            timerAlarmEnable(DeskMotor::timerHandle);
            Serial.println("Just started timer.");

            DeskMotor::instance->runTask();
        },
        "DeskMotorTimerTask", // Task name
        10000,                // Stack size (bytes)
        NULL,                 // Parameter
        configMAX_PRIORITIES, // Task priority
        NULL,                 // Task handle
        0);                   // Core where the task should run
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
    return deskMotor.currentPosition();
}

void DeskMotor::setNewTargetPosition(const long newTargetPosition)
{
    targetPosition = constrain(newTargetPosition, minSteps, maxSteps);
    // TODO Debug only, remove.
    // deskMotor.moveTo(targetPosition);
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
    // Add the number steps atomically as the motor might reset it to 0.
    skippedSteps.fetch_add(stepsToAdd);
}

int DeskMotor::getMissingSteps()
{
    // Atomically replace the number of skipped steps as another thread might add to it.
    return skippedSteps.exchange(0);
}

void DeskMotor::setCurrentPosition(const long newPosition)
{
    deskMotor.setCurrentPosition(newPosition);
}

void DeskMotor::moveUp()
{
    const bool isMovingDownwards = deskMotor.distanceToGo() < 0;
    if (isMovingDownwards)
    {
        // The motor is currently moving downwards, therefore, do nothing.
        return;
    }

    // TODO DEBUGGING ONLY
    digitalWrite(18, HIGH);

    const float currentSpeed = deskMotor.speed();
    const long currentPosition = deskMotor.currentPosition();
    // Add delta steps calculated for the given speed.
    const long targetPosition = currentPosition + calculateDeltaSteps(currentSpeed);

    // Set target position.
    setNewTargetPosition(targetPosition);

    // TODO DEBUGGING ONLY
    digitalWrite(18, LOW);
}

void DeskMotor::moveDown()
{
    const bool isMovingUpwards = deskMotor.distanceToGo() > 0;
    if (isMovingUpwards)
    {
        // The motor is currently moving upwards, therefore, do nothing.
        return;
    }

    // TODO DEBUGGING ONLY
    digitalWrite(18, HIGH);

    // Take the negative speed because then we can use the same calculation as for moving upwards.
    const float currentSpeed = -deskMotor.speed();
    const long currentPosition = deskMotor.currentPosition();
    // Subtract delta steps calculated for the given speed to account for the inverted speed.
    long targetPosition = currentPosition - calculateDeltaSteps(currentSpeed);

    // Set target position.
    setNewTargetPosition(targetPosition);

    // TODO DEBUGGING ONLY
    digitalWrite(18, LOW);
}

long DeskMotor::calculateDeltaSteps(float currentSpeed)
{
    const long currentPosition = deskMotor.currentPosition();

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