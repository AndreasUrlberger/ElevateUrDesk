#pragma once

#include <Arduino.h>
#include "GearboxCommunication.hpp"
#include "ButtonEvents.hpp"
#include <queue>

class InputEvent
{
public:
    ButtonId buttonId;
    ButtonEvent buttonEvent;

    InputEvent(ButtonId buttonId, ButtonEvent buttonEvent) : buttonId(buttonId), buttonEvent(buttonEvent) {}
    ~InputEvent() = default;
};

class InputController
{
private:
    static constexpr uint32_t MAX_GEARBOX_DEVIATION = 1000u;

    // UI State Machine
    enum class UiState
    {
        DriveControl,
        Idle,
        MoveUp,
        MoveDown,
        MoveTo
    };

    // Gearbox State Machine
    enum class GearboxState
    {
        OnBrake,
        LockingBrakes,
        UnlockingBrakes,
        DriveMode
    };

    GearboxCommunication *const gearbox{};
    std::queue<InputEvent *> eventQueue;
    UiState uiState{UiState::Idle};
    GearboxState gearboxState{GearboxState::OnBrake};

    void updateUiStateMachine();
    void updateGearboxStateMachine();

    // UI Methods
    void uiIdle(InputEvent *const event);
    void uiDriveControl(InputEvent *const event);
    void moveUp(InputEvent *const event);
    void moveDown(InputEvent *const event);
    void moveTo(InputEvent *const event);

    // Gearbox Methods
    void gearboxOnBrake();
    void gearboxLockingBrakes();
    void gearboxUnlockingBrakes();
    void gearboxDriveMode();

public:
    InputController(GearboxCommunication *const gearbox, std::queue<InputEvent *> eventQueue) : gearbox(gearbox), eventQueue(eventQueue) {}
    ~InputController() = default;

    void update();
};