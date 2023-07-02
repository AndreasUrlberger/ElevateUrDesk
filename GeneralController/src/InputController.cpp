#include "InputController.hpp"

void InputController::update()
{
    updateUiStateMachine();
    updateGearboxStateMachine();

    Serial.print("Gearbox position left: ");
    Serial.print(gearbox->getPositionLeft());
    Serial.print(" right: ");
    Serial.println(gearbox->getPositionRight());
}

void InputController::updateUiStateMachine()
{
    // Process all input event in UI State Machine.
    while (!eventQueue.empty())
    {
        InputEvent *const event = eventQueue.front();
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

        eventQueue.pop();
        delete event;
    }
}

void InputController::updateGearboxStateMachine()
{
    // Update gearbox state machine.
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
    case GearboxState::DriveMode:
        gearboxDriveMode();
        break;
    }

    // // Calculate diff between position of gearboxes.
    const uint32_t gearboxLeftPosition = gearbox->getPositionLeft();
    const uint32_t gearboxRightPosition = gearbox->getPositionRight();
    const int32_t diff = static_cast<int32_t>(gearboxLeftPosition) - static_cast<int32_t>(gearboxRightPosition);
    if (abs(diff) > MAX_GEARBOX_DEVIATION)
    {
        // Emergency stop.
        gearbox->emergencyStop();
        // TODO Not perfect.
        gearboxState = GearboxState::LockingBrakes;
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

    // TODO Check if brakes are locked, then switch to on brake state.
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

    // TODO Check if brakes are unlocked, then switch to drive mode.
}

void InputController::gearboxDriveMode()
{
    const bool isInStationaryState = uiState == UiState::Idle;
    if (isInStationaryState)
    {
        // Lock brakes.
        gearboxState = GearboxState::LockingBrakes;
    }

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