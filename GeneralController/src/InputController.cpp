#include "InputController.hpp"
#include "Pinout.hpp"

InputController::InputController(GearboxCommunication *const gearbox, std::queue<InputEvent *> *const eventQueue) : gearbox(gearbox), eventQueue(eventQueue)
{
    pinMode(RELAY_24V_PIN, OUTPUT);
    pinMode(GEARBOX_POWER_RELAY_PIN, OUTPUT);
}

void InputController::update()
{
    updateUiStateMachine();
    updateGearboxStateMachine();

    static uint32_t lastPosLeft = -1;
    static uint32_t lastPosRight = -1;

    if (lastPosLeft != gearbox->getPositionLeft() || lastPosRight != gearbox->getPositionRight())
    {
        Serial.print("Gearbox position left: ");
        Serial.print(gearbox->getPositionLeft());
        Serial.print(" right: ");
        Serial.println(gearbox->getPositionRight());

        lastPosLeft = gearbox->getPositionLeft();
        lastPosRight = gearbox->getPositionRight();
    }

    static UiState lastUiState = UiState::MoveDown;
    if (lastUiState != uiState)
    {
        Serial.print("UI State: ");
        switch (uiState)
        {
        case UiState::Idle:
            Serial.println("Idle");
            break;
        case UiState::DriveControl:
            Serial.println("DriveControl");
            break;
        case UiState::MoveUp:
            Serial.println("MoveUp");
            break;
        case UiState::MoveDown:
            Serial.println("MoveDown");
            break;
        case UiState::MoveTo:
            Serial.println("MoveTo");
            break;
        default:
            Serial.println("Unknown");
            break;
        }

        lastUiState = uiState;
    }

    static GearboxState lastGearboxState = GearboxState::LockingBrakes;
    if (lastGearboxState != gearboxState)
    {
        Serial.print("Gearbox State: ");
        switch (gearboxState)
        {
        case GearboxState::OnBrake:
            Serial.println("OnBrake");
            break;
        case GearboxState::LockingBrakes:
            Serial.println("LockingBrakes");
            break;
        case GearboxState::UnlockingBrakes:
            Serial.println("UnlockingBrakes");
            break;
        case GearboxState::Stop:
            Serial.println("Stop");
            break;
        case GearboxState::DriveMode:
            Serial.println("DriveMode");
            break;
        case GearboxState::EmergencyStop:
            Serial.println("EmergencyStop");
            break;
        case GearboxState::EmergencyStopRecovery:
            Serial.println("EmergencyStopRecovery");
            break;
        default:
            Serial.println("Unknown");
            break;
        }

        lastGearboxState = gearboxState;
    }

    static BrakeState lastBrakeStateLeft = 255;
    if (lastBrakeStateLeft != gearbox->getBrakeStateLeft())
    {
        Serial.print("Brake State Left: ");
        Serial.print(static_cast<uint32_t>(gearbox->getBrakeStateLeft()));
        Serial.print(" ");
        // Print name of brake state
        switch (gearbox->getBrakeStateLeft())
        {
        case GearboxCommunication::BRAKE_STATE_UNLOCKED:
            Serial.println("BRAKE_STATE_UNLOCKED");
            break;
        case GearboxCommunication::BRAKE_STATE_LOCKED:
            Serial.println("BRAKE_STATE_LOCKED");
            break;
        case GearboxCommunication::BRAKE_STATE_INTERMEDIARY:
            Serial.println("BRAKE_STATE_INTERMEDIARY");
            break;
        case GearboxCommunication::BRAKE_STATE_ERROR:
            Serial.println("BRAKE_STATE_ERROR");
            break;
        default:
            Serial.println("Unknown");
            break;
        }

        lastBrakeStateLeft = gearbox->getBrakeStateLeft();
    }

    static BrakeState lastBrakeStateRight = 255;
    if (lastBrakeStateRight != gearbox->getBrakeStateRight())
    {
        Serial.print("Brake State Right: ");
        // Print name of brake state
        switch (gearbox->getBrakeStateRight())
        {
        case GearboxCommunication::BRAKE_STATE_UNLOCKED:
            Serial.println("BRAKE_STATE_UNLOCKED");
            break;
        case GearboxCommunication::BRAKE_STATE_LOCKED:
            Serial.println("BRAKE_STATE_LOCKED");
            break;
        case GearboxCommunication::BRAKE_STATE_INTERMEDIARY:
            Serial.println("BRAKE_STATE_INTERMEDIARY");
            break;
        case GearboxCommunication::BRAKE_STATE_ERROR:
            Serial.println("BRAKE_STATE_ERROR");
            break;
        default:
            Serial.println("Unknown");
            break;
        }

        lastBrakeStateRight = gearbox->getBrakeStateRight();
    }

    // Keep track of speed.
    lastPositionLeft = gearbox->getPositionLeft();
    lastPositionRight = gearbox->getPositionRight();
}

void InputController::updateUiStateMachine()
{
    // Process all input events in UI State Machine.
    while (!eventQueue->empty())
    {
        InputEvent *const event = eventQueue->front();

        // Print Event.
        Serial.print("Button: ");
        // Print name of button
        switch (event->buttonId)
        {
        case ButtonEvents::ID_MAIN:
            Serial.print("Main");
            break;
        case ButtonEvents::ID_MOVE_UP:
            Serial.print("Up");
            break;
        case ButtonEvents::ID_MOVE_DOWN:
            Serial.print("Down");
            break;
        case ButtonEvents::ID_SHORTCUT_1:
            Serial.print("Shortcut 1");
            break;
        case ButtonEvents::ID_SHORTCUT_2:
            Serial.print("Shortcut 2");
            break;
        default:
            Serial.print("Unknown");
            break;
        }

        Serial.print(", Event: ");
        // Print name of event
        switch (event->buttonEvent)
        {
        case ButtonEvents::BUTTON_PRESSED:
            Serial.println("BUTTON_PRESSED");
            break;
        case ButtonEvents::BUTTON_RELEASED:
            Serial.println("BUTTON_RELEASED");
            break;
        case ButtonEvents::LONG_CLICK:
            Serial.println("LONG_CLICK");
            break;
        case ButtonEvents::DOUBLE_CLICK:
            Serial.println("DOUBLE_CLICK");
            break;
        case ButtonEvents::SINGLE_CLICK:
            Serial.println("SINGLE_CLICK");
            break;
        case ButtonEvents::START_DOUBLE_HOLD_CLICK:
            Serial.println("START_DOUBLE_HOLD_CLICK");
            break;
        case ButtonEvents::END_DOUBLE_HOLD_CLICK:
            Serial.println("END_DOUBLE_HOLD_CLICK");
            break;
        default:
            Serial.println("Unknown");
            break;
        }

        switch (uiState)
        {
        case UiState::Idle:
            uiIdle(event);
            break;
        case UiState::DriveControl:
            uiDriveControl(event);
            break;
        case UiState::MoveUp:
            moveUp(event);
            break;
        case UiState::MoveDown:
            moveDown(event);
            break;
        case UiState::MoveTo:
            moveTo(event);
            break;
        }

        eventQueue->pop();
        delete event;
    }
}

void InputController::updateGearboxStateMachine()
{
    // We do not want to enter the emergency stop again if we are currently in the emergency stop recovery state (It is to be expected that the gearbox deviation is too large in this state).
    if (gearboxState != GearboxState::EmergencyStopRecovery)
    {
        // Check for conditions of emergency stop.
        // Calculate diff between position of gearboxes.
        const uint32_t gearboxLeftPosition = gearbox->getPositionLeft();
        const uint32_t gearboxRightPosition = gearbox->getPositionRight();
        const int32_t diff = static_cast<int32_t>(gearboxLeftPosition) - static_cast<int32_t>(gearboxRightPosition);
        if (abs(diff) > MAX_GEARBOX_DEVIATION)
        {
            gearboxState = GearboxState::EmergencyStop;
        }
    }

    // Check what state transition to make.
    switch (gearboxState)
    {
    case GearboxState::OnBrake:
        checkTransitionOnBrake();
        break;
    case GearboxState::LockingBrakes:
        checkTransitionLockingBrakes();
        break;
    case GearboxState::UnlockingBrakes:
        checkTransitionUnlockingBrakes();
        break;
    case GearboxState::DriveMode:
        checkTransitionDriveMode();
        break;
    case GearboxState::Stop:
        checkTransitionStop();
        break;
    case GearboxState::EmergencyStop:
        checkTransitionEmergencyStop();
        break;
    case GearboxState::EmergencyStopRecovery:
        checkTransitionEmergencyStopRecovery();
        break;
    default:
        throw std::runtime_error("Unknown gearbox state to transition from");
        break;
    }

    // Execute current state.
    switch (gearboxState)
    {
    case GearboxState::OnBrake:
        performOnBrake();
        break;
    case GearboxState::LockingBrakes:
        performLockingBrakes();
        break;
    case GearboxState::UnlockingBrakes:
        performUnlockingBrakes();
        break;
    case GearboxState::DriveMode:
        performDriveMode();
        break;
    case GearboxState::Stop:
        performStop();
        break;
    case GearboxState::EmergencyStop:
        performEmergencyStop();
        break;
    case GearboxState::EmergencyStopRecovery:
        performEmergencyStopRecovery();
        break;
    default:
        throw std::runtime_error("Unknown gearbox state to execute");
        break;
    }
}

bool InputController::isInMovingUiState()
{
    return (uiState == UiState::MoveUp || uiState == UiState::MoveDown || uiState == UiState::MoveTo || uiState == UiState::DriveControl);
}

#pragma region UI Methods
void InputController::uiIdle(InputEvent *const event)
{
    if (event->buttonId == ButtonEvents::ID_MAIN && event->buttonEvent == ButtonEvents::SINGLE_CLICK)
    {
        // Main button clicked -> Drive control mode.
        uiState = UiState::DriveControl;
    }
}

void InputController::uiDriveControl(InputEvent *const event)
{
    if (event->buttonId == ButtonEvents::ID_MAIN && event->buttonEvent == ButtonEvents::SINGLE_CLICK)
    {
        // Main button clicked -> Back to idle mode.
        uiState = UiState::Idle;
        return;
    }
    const bool moveUp = event->buttonId == ButtonEvents::ID_MOVE_UP && event->buttonEvent == ButtonEvents::BUTTON_PRESSED;
    if (moveUp)
    {
        // Move up.
        uiState = UiState::MoveUp;
        return;
    }
    const bool moveDown = event->buttonId == ButtonEvents::ID_MOVE_DOWN && event->buttonEvent == ButtonEvents::BUTTON_PRESSED;
    if (moveDown)
    {
        // Move down.
        uiState = UiState::MoveDown;
        return;
    }

    if (event->buttonId == ButtonEvents::ID_SHORTCUT_2 && event->buttonEvent == ButtonEvents::SINGLE_CLICK)
    {
        // Shortcut 2 clicked -> Move to.
        uiState = UiState::MoveTo;
        return;
    }
}

void InputController::moveUp(InputEvent *const event)
{
    if (event->buttonId == ButtonEvents::ID_MAIN && event->buttonEvent == ButtonEvents::SINGLE_CLICK)
    {
        // Main button clicked -> Back to idle mode.
        uiState = UiState::Idle;
        return;
    }

    if (event->buttonId == ButtonEvents::ID_MOVE_UP && event->buttonEvent == ButtonEvents::BUTTON_RELEASED)
    {
        // Move up button released -> Don't move.
        uiState = UiState::DriveControl;
        return;
    }

    if (event->buttonId == ButtonEvents::ID_MOVE_DOWN && event->buttonEvent == ButtonEvents::BUTTON_PRESSED)
    {
        // Move down button pressed -> Don't move.
        uiState = UiState::DriveControl;
        return;
    }
}

void InputController::moveDown(InputEvent *const event)
{
    if (event->buttonId == ButtonEvents::ID_MAIN && event->buttonEvent == ButtonEvents::SINGLE_CLICK)
    {
        // Main button clicked -> Back to idle mode.
        uiState = UiState::Idle;
        return;
    }

    if (event->buttonId == ButtonEvents::ID_MOVE_DOWN && event->buttonEvent == ButtonEvents::BUTTON_RELEASED)
    {
        // Move down button released -> Don't move.
        uiState = UiState::DriveControl;
        return;
    }

    if (event->buttonId == ButtonEvents::ID_MOVE_UP && event->buttonEvent == ButtonEvents::BUTTON_PRESSED)
    {
        // Move up button pressed -> Don't move.
        uiState = UiState::DriveControl;
        return;
    }
}

void InputController::moveTo(InputEvent *const event)
{
    if (event->buttonId == ButtonEvents::ID_MAIN && event->buttonEvent == ButtonEvents::SINGLE_CLICK)
    {
        // Main button clicked -> Back to idle mode.
        uiState = UiState::Idle;
        return;
    }

    // Cancel move to if any button is pressed.
    uiState = UiState::DriveControl;
}
#pragma endregion UI Methods

#pragma region Gearbox Methods
void InputController::checkTransitionOnBrake()
{
    // If in moving ui state, unlock brakes.
    if (isInMovingUiState())
    {
        gearboxState = GearboxState::UnlockingBrakes;
        // Important to reset for first run of sub state machine, otherwise, the wait time of the first state will be skipped.
        lastUnlockTransition = millis();
    }
}

void InputController::checkTransitionUnlockingBrakes()
{
    // This transition is same for all.
    // If not in moving ui state, stop.
    if (!isInMovingUiState())
    {
        gearboxState = GearboxState::Stop;
        // Reset unlockingBrakeState.
        unlockingBrakeState = UnlockingBrakeState::SwitchOnGearboxPower;
        return;
    }

    // DEBUG ONLY
    static UnlockingBrakeState lastUnlockingBrakeState = UnlockingBrakeState::UnlockDriveUp;

    if (lastUnlockingBrakeState != unlockingBrakeState)
    {
        Serial.print("UnlockingBrakeState: ");
        switch (unlockingBrakeState)
        {
        case UnlockingBrakeState::SwitchOnGearboxPower:
            Serial.println("SwitchOnGearboxPower");
            break;
        case UnlockingBrakeState::SwitchOnMotorPowerSupply:
            Serial.println("SwitchOnMotorPowerSupply");
            break;
        case UnlockingBrakeState::SwitchOnMotorControlPower:
            Serial.println("SwitchOnMotorControlPower");
            break;
        case UnlockingBrakeState::SwitchOnMotorControl:
            Serial.println("SwitchOnMotorControl");
            break;
        case UnlockingBrakeState::UnlockBrakes:
            Serial.println("UnlockBrakes");
            break;
        case UnlockingBrakeState::UnlockDriveUp:
            Serial.println("UnlockDriveUp");
            break;
        default:
            Serial.println("Unknown");
            break;
        }
        lastUnlockingBrakeState = unlockingBrakeState;
    }
    // Check state transition for sub state machine.
    switch (unlockingBrakeState)
    {
    case UnlockingBrakeState::SwitchOnGearboxPower:
        checkTransitionSwitchOnGearboxPower();
        break;
    case UnlockingBrakeState::SwitchOnMotorPowerSupply:
        checkTransitionSwitchOnMotorPowerSupply();
        break;
    case UnlockingBrakeState::SwitchOnMotorControlPower:
        checkTransitionSwitchOnMotorControlPower();
        break;
    case UnlockingBrakeState::SwitchOnMotorControl:
        checkTransitionSwitchOnMotorControl();
        break;
    case UnlockingBrakeState::UnlockBrakes:
        checkTransitionUnlockBrakes();
        break;
    case UnlockingBrakeState::UnlockDriveUp:
        checkTransitionUnlockDriveUp();
        break;
    default:
        throw std::runtime_error("Unknown unlockingBrakeState to transition from");
        break;
    }
}

#pragma region UnlockingBrakes Transitions
void InputController::checkTransitionSwitchOnGearboxPower()
{
    const uint32_t currentTime = millis();
    // If given time has passed, switch to next state
    if (currentTime - lastUnlockTransition >= SWITCH_ON_GEARBOX_POWER_TIME)
    {
        unlockingBrakeState = UnlockingBrakeState::SwitchOnMotorPowerSupply;
        lastUnlockTransition = currentTime;
    }
}

void InputController::checkTransitionSwitchOnMotorPowerSupply()
{
    const uint32_t currentTime = millis();
    // If given time has passed, switch to next state
    if (currentTime - lastUnlockTransition >= SWITCH_ON_MOTOR_POWER_SUPPLY_TIME)
    {
        unlockingBrakeState = UnlockingBrakeState::SwitchOnMotorControlPower;
        lastUnlockTransition = currentTime;
    }
}

void InputController::checkTransitionSwitchOnMotorControlPower()
{
    const uint32_t currentTime = millis();

    if (!wasLastActionSuccessful)
    {
        lastUnlockTransition = currentTime;
        return;
    }

    // If given time has passed, switch to next state
    if (currentTime - lastUnlockTransition >= SWITCH_ON_MOTOR_CONTROL_POWER_TIME)
    {
        unlockingBrakeState = UnlockingBrakeState::SwitchOnMotorControl;
        lastUnlockTransition = currentTime;
    }
}

void InputController::checkTransitionSwitchOnMotorControl()
{
    const uint32_t currentTime = millis();

    if (!wasLastActionSuccessful)
    {
        lastUnlockTransition = currentTime;
        return;
    }

    // If given time has passed, switch to next state
    if (currentTime - lastUnlockTransition >= SWITCH_ON_MOTOR_CONTROL_TIME)
    {
        unlockingBrakeState = UnlockingBrakeState::UnlockBrakes;
        lastUnlockTransition = currentTime;
    }
}

void InputController::checkTransitionUnlockBrakes()
{
    // // Check if both brakes are unlocked.
    // if (gearbox->getBrakeStateLeft() == GearboxCommunication::BRAKE_STATE_UNLOCKED && gearbox->getBrakeStateRight() == GearboxCommunication::BRAKE_STATE_UNLOCKED)
    // {
    //     // Brakes are unlocked, switch to drive mode state
    //     gearboxState = GearboxState::DriveMode;
    //     // Reset unlockingBrakeState for next time.
    //     unlockingBrakeState = UnlockingBrakeState::SwitchOnGearboxPower;
    //     return;
    // }

    // // Check if max time for brake unlocking is reached, then switch to drive up state.
    // // TODO Does it make sense to also switch to drive up once one brake is unlocked but the other not?
    // const uint32_t currentTime = millis();
    // if (currentTime - lastUnlockTransition >= MAX_BRAKE_UNLOCKING_TIME)
    // {
    //     unlockingBrakeState = UnlockingBrakeState::UnlockDriveUp;
    //     isFirstRunDriveUp = true;
    //     lastUnlockTransition = currentTime;
    //     return;
    // }

    // TODO Replace with above once the brake is used again.
    // Assume brakes are unlocked.
    // Brakes are unlocked, switch to drive mode state
    gearboxState = GearboxState::DriveMode;
    // Reset unlockingBrakeState for next time.
    unlockingBrakeState = UnlockingBrakeState::SwitchOnGearboxPower;
}

void InputController::checkTransitionUnlockDriveUp()
{
    // // Check if both brakes are unlocked.
    // if (gearbox->getBrakeStateLeft() == GearboxCommunication::BRAKE_STATE_UNLOCKED && gearbox->getBrakeStateRight() == GearboxCommunication::BRAKE_STATE_UNLOCKED)
    // {
    //     // Brakes are unlocked, switch to drive mode state
    //     gearboxState = GearboxState::DriveMode;
    //     // Reset unlockingBrakeState for next time.
    //     unlockingBrakeState = UnlockingBrakeState::SwitchOnGearboxPower;
    //     return;
    // }

    // TODO Replace with above once brake is used again.
    // Assume brakes are unlocked.
    // Brakes are unlocked, switch to drive mode state
    gearboxState = GearboxState::DriveMode;
    // Reset unlockingBrakeState for next time.
    unlockingBrakeState = UnlockingBrakeState::SwitchOnGearboxPower;
}
#pragma endregion UnlockingBrakes Transitions

void InputController::checkTransitionLockingBrakes()
{
    // This transition is same for all.
    // If in moving ui state, unlock brakes.
    if (isInMovingUiState())
    {
        gearboxState = GearboxState::UnlockingBrakes;
        // Important to reset for first run of sub state machine, otherwise, the wait time of the first state will be skipped.
        lastUnlockTransition = millis();
        // Reset lockingBrakeState.
        lockingBrakeState = LockingBrakeState::LockBrakes;
        return;
    }

    static LockingBrakeState lastLockingBrakeState = LockingBrakeState::SwitchOffGearboxPower;

    if (lastLockingBrakeState != lockingBrakeState)
    {
        Serial.print("LockingBrakeState: ");
        switch (lockingBrakeState)
        {
        case LockingBrakeState::LockBrakes:
            Serial.println("LockBrakes");
            break;
        case LockingBrakeState::SwitchOffMotorControl:
            Serial.println("SwitchOffMotorControl");
            break;
        case LockingBrakeState::SwitchOffMotorControlPower:
            Serial.println("SwitchOffMotorControlPower");
            break;
        case LockingBrakeState::SwitchOffMotorPowerSupply:
            Serial.println("SwitchOffMotorPowerSupply");
            break;
        case LockingBrakeState::SwitchOffGearboxPower:
            Serial.println("SwitchOffGearboxPower");
            break;
        default:
            Serial.println("Unknown");
            break;
        }
        lastLockingBrakeState = lockingBrakeState;
    }

    // Check state transition for sub state machine.
    switch (lockingBrakeState)
    {
    case LockingBrakeState::LockBrakes:
        checkTransitionLockBrakes();
        break;
    case LockingBrakeState::SwitchOffMotorControl:
        checkTransitionSwitchOffMotorControl();
        break;
    case LockingBrakeState::SwitchOffMotorControlPower:
        checkTransitionSwitchOffMotorControlPower();
        break;
    case LockingBrakeState::SwitchOffMotorPowerSupply:
        checkTransitionSwitchOffMotorPowerSupply();
        break;
    case LockingBrakeState::SwitchOffGearboxPower:
        checkTransitionSwitchOffGearboxPower();
        break;
    default:
        throw std::runtime_error("Unknown lockingBrakeState to transition from");
        break;
    }
}

#pragma region LockingBrakes Transitions
void InputController::checkTransitionLockBrakes()
{
    // // Switch to next state if both brakes are locked, or the max time for locking is reached.
    // const uint32_t currentTime = millis();
    // if ((gearbox->getBrakeStateLeft() == GearboxCommunication::BRAKE_STATE_LOCKED && gearbox->getBrakeStateRight() == GearboxCommunication::BRAKE_STATE_LOCKED) || (currentTime - lastLockTransition >= MAX_BRAKE_LOCKING_TIME))
    // {
    //     lockingBrakeState = LockingBrakeState::SwitchOffMotorControl;
    //     lastLockTransition = currentTime;
    // }

    // TODO Replace with above once brake is used again.
    // Assume brakes are locked
    const uint32_t currentTime = millis();
    lockingBrakeState = LockingBrakeState::SwitchOffMotorControl;
    lastLockTransition = currentTime;
}

void InputController::checkTransitionSwitchOffMotorControl()
{
    const uint32_t currentTime = millis();

    if (!wasLastActionSuccessful)
    {
        lastLockTransition = currentTime;
        return;
    }

    // If given time has passed, switch to next state
    if (currentTime - lastLockTransition >= SWITCH_OFF_MOTOR_CONTROL_TIME)
    {
        lockingBrakeState = LockingBrakeState::SwitchOffMotorControlPower;
        lastLockTransition = currentTime;
    }
}

void InputController::checkTransitionSwitchOffMotorControlPower()
{
    const uint32_t currentTime = millis();

    if (!wasLastActionSuccessful)
    {
        lastLockTransition = currentTime;
        return;
    }

    // If given time has passed, switch to next state
    if (currentTime - lastLockTransition >= SWITCH_OFF_MOTOR_CONTROL_POWER_TIME)
    {
        lockingBrakeState = LockingBrakeState::SwitchOffMotorPowerSupply;
        lastLockTransition = currentTime;
    }
}

void InputController::checkTransitionSwitchOffMotorPowerSupply()
{
    const uint32_t currentTime = millis();
    // If given time has passed, switch to next state
    if (currentTime - lastLockTransition >= SWITCH_OFF_MOTOR_POWER_SUPPLY_TIME)
    {
        lockingBrakeState = LockingBrakeState::SwitchOffGearboxPower;
        lastLockTransition = currentTime;
    }
}

void InputController::checkTransitionSwitchOffGearboxPower()
{
    const uint32_t currentTime = millis();
    // If given time has passed, switch to next state
    if (currentTime - lastLockTransition >= SWITCH_OFF_GEARBOX_POWER_TIME)
    {
        lockingBrakeState = LockingBrakeState::LockBrakes;
        gearboxState = GearboxState::OnBrake;
        lastLockTransition = currentTime;
    }
}
#pragma endregion LockingBrakes Transitions

void InputController::checkTransitionStop()
{
    // Lock brakes when there is no movement.
    if (lastPositionLeft == gearbox->getPositionLeft() && lastPositionRight == gearbox->getPositionRight())
    {
        gearboxState = GearboxState::LockingBrakes;
        // Important to reset for first run of sub state machine, otherwise, the wait time of the first state will be skipped.
        lastLockTransition = millis();
    }
}

void InputController::checkTransitionDriveMode()
{
    // If not in any of the moving state, stop.
    if (!isInMovingUiState())
    {
        gearboxState = GearboxState::Stop;
    }
}

void InputController::checkTransitionEmergencyStop()
{
    // Transition to recovery state if there is no movement anymore.
    if (lastPositionLeft == gearbox->getPositionLeft() && lastPositionRight == gearbox->getPositionRight())
    {
        gearboxState = GearboxState::EmergencyStopRecovery;
        // Need to reset recovery target position for a new value to get set.
        emergencyStopRecoverPosition = UINT32_MAX;
    }
}

void InputController::checkTransitionEmergencyStopRecovery()
{
    // Transition to drive mode state if the gearboxes are close enough to each other again.
    if (abs(static_cast<int32_t>(gearbox->getPositionLeft()) - static_cast<int32_t>(gearbox->getPositionRight())) <= MAX_DEVIATION_STOP_RECOVERY)
    {
        gearboxState = GearboxState::DriveMode;
    }
}

void InputController::performOnBrake()
{
    // Nothing to do, this state just waits for any events.
    gearbox->getPosition();
}

void InputController::performLockingBrakes()
{
    // Call according function to current lock state machine.
    switch (lockingBrakeState)
    {
    case LockingBrakeState::LockBrakes:
        performLockBrakes();
        break;
    case LockingBrakeState::SwitchOffMotorControl:
        performSwitchOffMotorControl();
        break;
    case LockingBrakeState::SwitchOffMotorControlPower:
        performSwitchOffMotorControlPower();
        break;
    case LockingBrakeState::SwitchOffMotorPowerSupply:
        performSwitchOffMotorPowerSupply();
        break;
    case LockingBrakeState::SwitchOffGearboxPower:
        performSwitchOffGearboxPower();
        break;
    default:
        throw std::runtime_error("Unknown lockingBrakeState to execute");
        break;
    }
}

void InputController::performUnlockingBrakes()
{
    // Call according function to current unlock state machine.
    switch (unlockingBrakeState)
    {
    case UnlockingBrakeState::SwitchOnGearboxPower:
        performSwitchOnGearboxPower();
        break;
    case UnlockingBrakeState::SwitchOnMotorPowerSupply:
        performSwitchOnMotorPowerSupply();
        break;
    case UnlockingBrakeState::SwitchOnMotorControlPower:
        performSwitchOnMotorControlPower();
        break;
    case UnlockingBrakeState::SwitchOnMotorControl:
        performSwitchOnMotorControl();
        break;
    case UnlockingBrakeState::UnlockBrakes:
        performUnlockBrakes();
        break;
    case UnlockingBrakeState::UnlockDriveUp:
        performUnlockDriveUp();
        break;
    default:
        throw std::runtime_error("Unknown unlockingBrakeState to execute");
        break;
    }
}

void InputController::performStop()
{
    // We do nothing in this state, we wait for the gearboxes to stop, they will not get any new commands.
    gearbox->getPosition();
}

void InputController::performDriveMode()
{
    // Depending on the UI State we either drive up, down, to or not at all.
    switch (uiState)
    {
    case UiState::MoveUp:
        gearbox->driveUp();
        break;
    case UiState::MoveDown:
        gearbox->driveDown();
        break;
    case UiState::MoveTo:
        gearbox->driveTo(40000u);
        break;
    default:
        gearbox->getPosition();
        break;
    }
}

void InputController::performEmergencyStop()
{
    gearbox->emergencyStop();
}

void InputController::performEmergencyStopRecovery()
{
    if (emergencyStopRecoverPosition == UINT32_MAX)
    {
        // First run.
        // Use the middle of the two gearboxes as target position.
        emergencyStopRecoverPosition = gearbox->getPositionLeft() / 2 + gearbox->getPositionRight() / 2;
    }

    gearbox->driveTo(emergencyStopRecoverPosition);
}

void InputController::performSwitchOnGearboxPower()
{
    digitalWrite(GEARBOX_POWER_RELAY_PIN, HIGH);
    gearbox->getPosition();
}

void InputController::performSwitchOnMotorPowerSupply()
{
    digitalWrite(RELAY_24V_PIN, HIGH);
    gearbox->getPosition();
}

void InputController::performSwitchOnMotorControlPower()
{
    wasLastActionSuccessful = gearbox->toggleMotorControlPower(true);
}

void InputController::performSwitchOnMotorControl()
{
    wasLastActionSuccessful = gearbox->toggleMotorControl(true);
}

void InputController::performUnlockBrakes()
{
    gearbox->loosenBrake();
}

void InputController::performUnlockDriveUp()
{
    if (isFirstRunDriveUp)
    {
        isFirstRunDriveUp = false;
        startPositionDriveUp = max(gearbox->getPositionLeft(), gearbox->getPositionRight());
        targetPositionDriveUp = startPositionDriveUp + UNLOCKING_DRIVE_UP_DISTANCE;
    }

    gearbox->driveTo(targetPositionDriveUp);

    // Check if target position is reached.
    if (gearbox->getPositionLeft() >= targetPositionDriveUp && gearbox->getPositionRight() >= targetPositionDriveUp)
    {
        // Target position is reached and brakes are still locked, drive up again.
        targetPositionDriveUp = targetPositionDriveUp + UNLOCKING_DRIVE_UP_DISTANCE;
    }
}

void InputController::performLockBrakes()
{
    gearbox->fastenBrake();
}

void InputController::performSwitchOffMotorControl()
{
    wasLastActionSuccessful = gearbox->toggleMotorControl(false);
}

void InputController::performSwitchOffMotorControlPower()
{
    wasLastActionSuccessful = gearbox->toggleMotorControlPower(false);
}

void InputController::performSwitchOffMotorPowerSupply()
{
    digitalWrite(RELAY_24V_PIN, LOW);
    gearbox->getPosition();
}

void InputController::performSwitchOffGearboxPower()
{
    digitalWrite(GEARBOX_POWER_RELAY_PIN, LOW);
    gearbox->getPosition();
}
#pragma endregion Gearbox Machine