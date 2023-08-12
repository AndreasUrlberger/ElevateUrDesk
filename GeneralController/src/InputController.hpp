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
    static constexpr uint32_t MAX_GEARBOX_DEVIATION = 800u;
    static constexpr uint32_t UNLOCKING_DRIVE_UP_DISTANCE = 40u;
    static constexpr uint32_t MAX_DEVIATION_STOP_RECOVERY = 0u;

    static constexpr uint32_t SWITCH_ON_GEARBOX_POWER_TIME{10u};
    static constexpr uint32_t SWITCH_ON_MOTOR_POWER_SUPPLY_TIME{10u};
    static constexpr uint32_t SWITCH_ON_MOTOR_CONTROL_POWER_TIME{10u};
    static constexpr uint32_t SWITCH_ON_MOTOR_CONTROL_TIME{10u};
    static constexpr uint32_t UNLOCK_BRAKES_TIME{10u};
    static constexpr uint32_t UNLOCK_DRIVE_UP_TIME{10u};
    static constexpr uint32_t LOCK_BRAKES_TIME{10u};
    static constexpr uint32_t SWITCH_OFF_MOTOR_CONTROL_TIME{10u};
    static constexpr uint32_t SWITCH_OFF_MOTOR_CONTROL_POWER_TIME{10u};
    static constexpr uint32_t SWITCH_OFF_MOTOR_POWER_SUPPLY_TIME{10u};
    static constexpr uint32_t SWITCH_OFF_GEARBOX_POWER_TIME{10u};

    static constexpr uint32_t MAX_BRAKE_UNLOCKING_TIME{250u};
    static constexpr uint32_t MAX_BRAKE_LOCKING_TIME{1000u};

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
        Stop,
        DriveMode,
        EmergencyStop,
        EmergencyStopRecovery
    };

    enum class UnlockingBrakeState
    {
        SwitchOnGearboxPower,
        SwitchOnMotorPowerSupply,
        SwitchOnMotorControlPower,
        SwitchOnMotorControl,
        UnlockBrakes,
        UnlockDriveUp
    };

    enum class LockingBrakeState
    {
        LockBrakes,
        SwitchOffMotorControl,
        SwitchOffMotorControlPower,
        SwitchOffMotorPowerSupply,
        SwitchOffGearboxPower
    };

    // Gearbox Unlocking Drive Up
    bool isFirstRunDriveUp{true};
    uint32_t startPositionDriveUp{0u};
    uint32_t targetPositionDriveUp{0u};

    // gearbox stop
    uint32_t lastPositionLeft{0u};
    uint32_t lastPositionRight{0u};

    GearboxCommunication *const gearbox{};
    std::queue<InputEvent *> *const eventQueue{};
    UiState uiState{UiState::Idle};
    UnlockingBrakeState unlockingBrakeState{UnlockingBrakeState::SwitchOnGearboxPower};
    LockingBrakeState lockingBrakeState{LockingBrakeState::LockBrakes};
    uint32_t lastUnlockTransition{0u};
    GearboxState gearboxState{GearboxState::OnBrake};
    uint32_t lastLockTransition{0u};
    // Max uint32_t value is used as invalid position.
    uint32_t emergencyStopRecoverPosition = UINT32_MAX;

    void updateUiStateMachine();
    void updateGearboxStateMachine();
    bool isInMovingUiState();

    // UI Methods
    void uiIdle(InputEvent *const event);
    void uiDriveControl(InputEvent *const event);
    void moveUp(InputEvent *const event);
    void moveDown(InputEvent *const event);
    void moveTo(InputEvent *const event);

    void checkTransitionOnBrake();
    void checkTransitionLockingBrakes();
    void checkTransitionUnlockingBrakes();
    void checkTransitionStop();
    void checkTransitionDriveMode();
    void checkTransitionEmergencyStop();
    void checkTransitionEmergencyStopRecovery();

    // Unlocking Brakes Sub Methods
    void checkTransitionSwitchOnGearboxPower();
    void checkTransitionSwitchOnMotorPowerSupply();
    void checkTransitionSwitchOnMotorControlPower();
    void checkTransitionSwitchOnMotorControl();
    void checkTransitionUnlockBrakes();
    void checkTransitionUnlockDriveUp();

    // Locking Brakes Sub Methods
    void checkTransitionLockBrakes();
    void checkTransitionSwitchOffMotorControl();
    void checkTransitionSwitchOffMotorControlPower();
    void checkTransitionSwitchOffMotorPowerSupply();
    void checkTransitionSwitchOffGearboxPower();

    void performOnBrake();
    void performLockingBrakes();
    void performUnlockingBrakes();
    void performStop();
    void performDriveMode();
    void performEmergencyStop();
    void performEmergencyStopRecovery();

    // Unlocking Brakes Sub Methods
    void performSwitchOnGearboxPower();
    void performSwitchOnMotorPowerSupply();
    void performSwitchOnMotorControlPower();
    void performSwitchOnMotorControl();
    void performUnlockBrakes();
    void performUnlockDriveUp();

    // Locking Brakes Sub Methods
    void performLockBrakes();
    void performSwitchOffMotorControl();
    void performSwitchOffMotorControlPower();
    void performSwitchOffMotorPowerSupply();
    void performSwitchOffGearboxPower();

public:
    InputController(GearboxCommunication *const gearbox, std::queue<InputEvent *> *const eventQueue);
    ~InputController() = default;

    void update();
};