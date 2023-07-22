#include "InputController.hpp"

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
        case GearboxState::UnlockingDriveUp:
            Serial.println("UnlockingDriveUp");
            break;
        case GearboxState::Stop:
            Serial.println("Stop");
            break;
        case GearboxState::DriveMode:
            Serial.println("DriveMode");
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
    // Process all input event in UI State Machine.
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
    // Update gearbox state machine.
    // TODO Set enable pin in each state to only enable the motor when the desk is moving or is about to move.
    switch (gearboxState)
    {
    case GearboxState::OnBrake:
        gearboxOnBrake();
        break;
    case GearboxState::LockingBrakes:
        gearboxLockingBrakes();
        break;
    case GearboxState::UnlockingBrakes:
        gearboxUnlockingBrakes();
        break;
    case GearboxState::UnlockingDriveUp:
        gearboxUnlockingDriveUp();
        break;
    case GearboxState::DriveMode:
        gearboxDriveMode();
        break;
    case GearboxState::Stop:
        gearboxStop();
        break;
    }

    // Check state of gearbox for things like exmergen
    // Calculate diff between position of gearboxes.
    const uint32_t gearboxLeftPosition = gearbox->getPositionLeft();
    const uint32_t gearboxRightPosition = gearbox->getPositionRight();
    const int32_t diff = static_cast<int32_t>(gearboxLeftPosition) - static_cast<int32_t>(gearboxRightPosition);
    if (abs(diff) > MAX_GEARBOX_DEVIATION)
    {
        // Emergency stop.
        gearbox->emergencyStop();
        // TODO Not perfect.
        // TODO Maybe check if the gearbox is in an emergency state whenever it shall drive. Maybe also add an emergency stop UI State.
        gearboxState = GearboxState::LockingBrakes;
        return;
    }

    // Perform gearbox state.
    switch (gearboxState)
    {
    case GearboxState::UnlockingBrakes:
        gearbox->loosenBrake();
        break;
    case GearboxState::UnlockingDriveUp:
        performUnlockingDriveUp();
        break;
    case GearboxState::LockingBrakes:
        gearbox->fastenBrake();
        break;
    case GearboxState::DriveMode:
        performDriveMode();
        break;
    case GearboxState::OnBrake:
        gearbox->getPosition();
        break;
    default:
        break;
    }
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
void InputController::gearboxOnBrake()
{
    const bool isInMovingState = uiState == UiState::MoveUp || uiState == UiState::MoveDown || uiState == UiState::MoveTo || uiState == UiState::DriveControl;
    if (isInMovingState)
    {
        // Unlock brakes.
        gearboxState = GearboxState::UnlockingBrakes;
    }
}

void InputController::gearboxLockingBrakes()
{
    const bool isInMovingState = uiState == UiState::MoveUp || uiState == UiState::MoveDown || uiState == UiState::MoveTo || uiState == UiState::DriveControl;
    if (isInMovingState)
    {
        // Unlock brakes.
        gearboxState = GearboxState::UnlockingBrakes;
        return;
    }

    if (gearbox->getBrakeStateLeft() == GearboxCommunication::BRAKE_STATE_LOCKED && gearbox->getBrakeStateRight() == GearboxCommunication::BRAKE_STATE_LOCKED)
    {
        gearboxState = GearboxState::OnBrake;
    }
}

void InputController::gearboxUnlockingBrakes()
{
    const bool isInStationaryState = uiState == UiState::Idle;
    if (isInStationaryState)
    {
        // Lock brakes.
        gearboxState = GearboxState::LockingBrakes;
        return;
    }

    if (gearbox->getBrakeStateLeft() == GearboxCommunication::BRAKE_STATE_UNLOCKED && gearbox->getBrakeStateRight() == GearboxCommunication::BRAKE_STATE_UNLOCKED)
    {
        gearboxState = GearboxState::DriveMode;
    }
    else if (gearbox->getBrakeStateLeft() != GearboxCommunication::BRAKE_STATE_LOCKED && gearbox->getBrakeStateRight() != GearboxCommunication::BRAKE_STATE_LOCKED)
    {
        gearboxState = GearboxState::UnlockingDriveUp;
    }
}

void InputController::gearboxDriveMode()
{
    const bool isInStationaryState = uiState == UiState::Idle;
    if (isInStationaryState)
    {
        gearboxState = GearboxState::Stop;
    }
}

void InputController::gearboxUnlockingDriveUp()
{
    // Handle change in UI state.
    const bool isInStationaryState = uiState == UiState::Idle;
    if (isInStationaryState)
    {
        // Lock brakes.
        gearboxState = GearboxState::Stop;
        return;
    }

    // Check if brakes are unlocked.
    if (gearbox->getBrakeStateLeft() == GearboxCommunication::BRAKE_STATE_UNLOCKED && gearbox->getBrakeStateRight() == GearboxCommunication::BRAKE_STATE_UNLOCKED)
    {
        gearboxState = GearboxState::DriveMode;
    }
}

void InputController::gearboxStop()
{
    // Handle change in UI state.
    if (uiState == UiState::MoveUp || uiState == UiState::MoveDown || uiState == UiState::MoveTo)
    {
        gearboxState = GearboxState::UnlockingBrakes;
        return;
    }

    if (lastPositionLeft == gearbox->getPositionLeft() && lastPositionRight == gearbox->getPositionRight())
    {
        // No change in position -> No movement -> Lock brakes.
        gearboxState = GearboxState::LockingBrakes;
    }
}

void InputController::performUnlockingDriveUp()
{
    if (firstRun)
    {
        firstRun = false;
        startPosition = max(gearbox->getPositionLeft(), gearbox->getPositionRight());
        targetPosition = startPosition + UNLOCKING_DRIVE_UP_DISTANCE;
    }

    gearbox->driveTo(targetPosition);

    // Check if target position is reached.
    if (gearbox->getPositionLeft() >= targetPosition && gearbox->getPositionRight() >= targetPosition)
    {
        // Target position is reached and brakes are still locked, drive up again.
        targetPosition = targetPosition + UNLOCKING_DRIVE_UP_DISTANCE;
    }
}

void InputController::performDriveMode()
{
    if (uiState == UiState::MoveUp)
    {
        gearbox->driveUp();
    }

    if (uiState == UiState::MoveDown)
    {
        gearbox->driveDown();
    }

    if (uiState == UiState::MoveTo)
    {
        gearbox->driveTo(40000);
    }
}
#pragma endregion Gearbox Methods