#include "DeskMotor.hpp"

DeskMotor *DeskMotor::instance;

DeskMotor::DeskMotor(const float maxSpeed, const float maxAcceleration) : maxSpeed(maxSpeed), maxAcceleration(maxAcceleration)
{
    instance = this;
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

    Serial.printf("Main motor initialized\n");
}

void DeskMotor::step()
{
    if (isRunning && (deskMotor.distanceToGo() != 0))
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

        digitalWrite(18, HIGH);

        // Update to new target position
        deskMotor.moveTo(targetPosition);

        digitalWrite(18, LOW);
    }
}

void IRAM_ATTR onDeskMotorTimer()
{
    DeskMotor::instance->step();
}

// Run on the core that starts it.
void DeskMotor::startTimer()
{
    // The prescaler is used to divide the base clock frequency of the ESP32’s timer. The ESP32’s timer uses the APB clock (APB_CLK) as its base clock, which is normally 80 MHz. By setting the prescaler to 80, we are dividing the base clock frequency by 80, resulting in a timer tick frequency of 1 MHz (80 MHz / 80 = 1 MHz).
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onDeskMotorTimer, true);
    timerAlarmWrite(timer, iterationIntervalUS, true);
    timerAlarmEnable(timer);
}

void DeskMotor::init(const int newSpeed) // has to run on different core so that it is non-blocking ?!?
{
    // set target position
    if (debugMode)
    {
        Serial.printf("Current position: %d \n", deskMotor.currentPosition());
        Serial.printf("steps to old target position: %d \n", deskMotor.distanceToGo());
    }
    deskMotor.moveTo(targetPosition);
    if (debugMode)
    {
        Serial.printf("steps to new target position: %d \n", deskMotor.distanceToGo());
    }
    deskMotor.setMaxSpeed(newSpeed);
    // newSpeed is the computed speed to resync the two gearboxes

    if (debugMode)
    {
        Serial.printf("Target position set to %d \n", targetPosition);
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

void DeskMotor::setCurrentPosition(const long newPosition)
{
    deskMotor.setCurrentPosition(newPosition);
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