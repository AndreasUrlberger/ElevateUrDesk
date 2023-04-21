#include "DeskMotor.hpp"

#include <chrono>
#include <thread>
#include <esp_task_wdt.h>

DeskMotor *DeskMotor::instance;

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
        // if(deskMotor.distanceToGo() != 0) Doesnt matter, does it?
        digitalWrite(18, HIGH);
        deskMotor.run();
        digitalWrite(18, LOW);
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

    // Create chrono time now.
    auto start = std::chrono::high_resolution_clock::now();
    // Create chrono interval time iterationIntervalUS.
    auto interval = std::chrono::microseconds(iterationIntervalUS);
    // Create chrono time for next iteration.
    auto next = start + interval;

    while (true)
    {
        // Execute code.
        DeskMotor::instance->step();

        // Wait for next iteration.
        std::this_thread::sleep_until(next);

        // Update next iteration time.
        next += interval;
    }
}

void DeskMotor::startTimer()
{
    // Create new task to handle the timer interrupt.
    Serial.println("Start timer task.");
    xTaskCreatePinnedToCore(
        [](void *param)
        {
            DeskMotor::instance->runTask();
        },
        "DeskMotorTimerTask", // Task name
        10000,                // Stack size (bytes)
        NULL,                 // Parameter
        configMAX_PRIORITIES, // Task priority
        NULL,                 // Task handle
        0);                   // Core where the task should run

    Serial.println("DeskMotorTimerTask started.");
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

    // Calculate target position based on current position and speed.
    const float currentSpeed = deskMotor.speed();
    const long currentPosition = deskMotor.currentPosition();
    Serial.print("currentPosition: ");
    Serial.println(currentPosition);
    Serial.print("currentSpeed: ");
    Serial.println(currentSpeed);

    const float maxPotAcceleration = maxAcceleration * moveInputIntervalMS / 1000;
    const float theoreticalEndSpeed = currentSpeed + maxPotAcceleration;

    // Serial.print("maxPotAcceleration: ");
    // Serial.println(maxPotAcceleration);

    // Serial.print("theorEndSpeed: ");
    // Serial.println(theoreticalEndSpeed);

    float actualEndSpeed;
    float totalSteps = 0;

    if (theoreticalEndSpeed > maxSpeed)
    {
        Serial.println("Max Speed.");
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

    // Serial.print("acceleration part: ");
    // Serial.println(totalSteps);

    // Base rectangle.
    const float baseRectangleSteps = currentSpeed * moveInputIntervalMS / 1000;

    // Serial.print("baseRectangle: ");
    // Serial.println(baseRectangleSteps);

    // Triangular decrease till halt.
    const float decelerationTime = actualEndSpeed / maxAcceleration; // seconds
    const float triangleFallSteps = 0.5 * actualEndSpeed * decelerationTime;

    // Serial.print("DecelerationTime ");
    // Serial.println(decelerationTime);

    // Serial.print("triangleFallSteps: ");
    // Serial.println(triangleFallSteps);

    totalSteps += baseRectangleSteps + triangleFallSteps;
    const long bufferSteps = totalSteps * upDownStepBufferFactor;
    const long targetPosition = currentPosition + totalSteps + bufferSteps;

    // Set target position.
    setNewTargetPosition(targetPosition);
    const double deltaSteps = totalSteps + bufferSteps;
    // Serial.print("DeltaPosition: ");
    // Serial.println(deltaSteps);
    Serial.print("TargetPosition: ");
    Serial.println(targetPosition);
}

void DeskMotor::moveDown()
{
    // Calculate target position based on current position and speed.
    // Set target position.
}