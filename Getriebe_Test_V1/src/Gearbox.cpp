#include "Gearbox.hpp"
#include "DeskMotor.hpp"

void Gearbox::setup()
{
    lightgate.CheckClosed();
    // Motorsteuerung aktivieren
    // DeskMotor bestromen
    brake.openPrimaryBrake();
    brake.openSecondaryBrake();
    // wenn Bremse noch geschlossen, DeskMotor solange nach oben bewegen bis Lightgate meldet dass Bremse geöffnet ist
    // read mathematical steps from "somewhere"
    Serial.printf("Gearbox %s ready\n", name);
}

void Gearbox::run(int currentPosition, int requestedPosition, int stepDeviation)
{
    // compute new speed
    // TODO: entkommentieren
    // int newSpeed = computeNewSpeed(stepDeviation, currentSpeed);

    // handle request
    // compute steps:
    //  targetPosition = computeTargetPosition(stepDeviation, newSpeed);

    // after targetPosition is reached, skipped steps are read from motor controller, and added to targetPosition (repeat until targetPosition is reached)
    // then, motor is stopped by ("stepper::stop()") if no new request was made
    // if new request was made, restart run() with new targetPosition
    int targetPosition = currentPosition;
    // TODO: entkommentieren
    // DeskMotor::run(targetPosition, newSpeed);

    deskMotor.setMaxAcceleration(150);
    deskMotor.run(1000);
}

void Gearbox::standby()
{
    // DeskMotor und Motorsteuerung aktiv lassen
    // Bremsen geöffnet lassen
    // nach 2s (?) ohne weitere Befehle, Gearbox.Stop() ausführen
    Serial.printf("Gearbox %s in Standby, will shutdown in 2s\n", name);
}

void Gearbox::stop()
{
    brake.closePrimaryBrake();
    brake.closeSecondaryBrake();
    // wenn Bremse noch offen, solange nach unten fahren bis secondaryBrake ODER primaryBrake geschlossen ist
    // DeskMotor stromlos schalten
    // Motorsteuerung deaktivieren
    // Sensorhöhe und mathematische Höhe speichern
    Serial.printf("Gearbox %s finished\n", name);
}

void Gearbox::status()
{
    // ausgelesene und rechnerische Höhe ausgeben
    // aktuelle Schrittzahl ausgebenar
    Serial.printf("Gearbox %s Status: %s \n", name, nameOfState(state));                          // Status: Ready, Standby, Stopped, Error
    Serial.printf("Sensor height: %s, DeskMotor height: %s\n", sensorHeight, mathematicalHeight); // gibt Sensorhöhe (von Rotationssensor umgerechnet) und mathematische Höhe (Summe der Motorschritte) aus
}

void Gearbox::powerLoss()
{
    brake.closePrimaryBrake();
    brake.closeSecondaryBrake();
    // DeskMotor und Motorsteuerung deaktivieren
    // Sensorhöhe und mathematische Höhe speichern
    // auslesen ob Bremsen geschlossen sind
    Serial.printf("PowerLoss-Actions succesfully performed \n");
}

std::string Gearbox::nameOfState(GearboxState state)
{
    switch (state)
    {
    case GearboxState::READY:
        return "ready";
    case GearboxState::STANDBY:
        return "standby";
    case GearboxState::STOPPED:
        return "stopped";
    case GearboxState::ERROR:
        return "error";
    }

    return "ERROR";
}

int Gearbox::computeNewSpeed(int stepDeviation, int currentSpeed)
{
    int newSpeed;
    /* compute new speed

    stepDeviation is the difference between motor 1 (this one) and 2, negative if motor 1 is behind motor 2
      action should only be taken if difference is positive
      if difference is negative, motor 2 should be slowed down, as speeding up motor 1 could cause it to skip steps

      TODO: check if it also works in opposite direction (motor should also slow down while going down)
    */
    if (stepDeviation > 0)
    {
        // formula for calculating the new speed
        newSpeed = currentSpeed - (stepDeviation * 0.001);
        // new speed must not be lower than 0
        if (newSpeed < 0)
        {
            newSpeed = 0;
        }
        if (debugMode)
        {
            Serial.printf("New speed: %d\n", newSpeed);
        }
    }
    else
    {
        // no action needed
        newSpeed = currentSpeed;
    }
    return newSpeed;
}

int Gearbox::computeTargetPosition(int stepDeviation, int currentSpeed)
{
    int targetPosition;
    // compute target position
    // targetPosition = current step count + steps that can be made in xx ms
    // check for direction of movement
    // if targetPosition > requestedPosition, targetPosition = requestedPosition
    // if targetPosition > maxSteps, targetPosition = maxSteps
    // if targetPosition < minSteps, targetPosition = minSteps

    return targetPosition;
}

Gearbox::Gearbox(std::string gearboxName, float sensorHeight, float mathematicalHeight)
{
    name = gearboxName;
    this->sensorHeight = sensorHeight;
    this->mathematicalHeight = mathematicalHeight;
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

void Gearbox::updateMotorSpeed(int speed)
{
    deskMotor.setMaxSpeed(speed);
}

void Gearbox::moveUp()
{
    // Calculate target position based on current position and speed.
    // Set target position.
    deskMotor.moveUp();
}

void Gearbox::moveDown()
{
    // Calculate target position based on current position and speed.
    // Set target position.
    deskMotor.moveDown();
}

void Gearbox::moveToPosition(long targetPosition)
{
    deskMotor.setNewTargetPosition(targetPosition);
}