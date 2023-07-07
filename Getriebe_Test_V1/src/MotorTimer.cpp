#include "MotorTimer.hpp"
#include <esp_task_wdt.h>

MotorTimer *MotorTimer::instance;
hw_timer_t *MotorTimer::timerHandle;
DeskMotor *MotorTimer::deskMotor;
Brake *MotorTimer::brake1;
Brake *MotorTimer::brake2;

void IRAM_ATTR onMotorTimer()
{
    MotorTimer::instance->dueTaskIterations.fetch_add(1);
}

MotorTimer::MotorTimer(DeskMotor *const deskMotor, Brake *const brake1, Brake *const brake2)
{
    instance = this;
    dueTaskIterations.store(0);

    this->deskMotor = deskMotor;
    this->brake1 = brake1;
    this->brake2 = brake2;

    startTimer();
}

void MotorTimer::runTask()
{
    Serial.println("RunTask started");

    esp_task_wdt_init(UINT32_MAX, false);
    esp_task_wdt_delete(NULL);

    Serial.println("WDT diabled on core " + String(xPortGetCoreID()));

    while (true)
    {
        if (dueTaskIterations.load() > 0)
        {
            dueTaskIterations.fetch_sub(1);
            deskMotor->step();
            brake1->step();
            brake2->step();
        }
    }
}

void MotorTimer::startTimer()
{
    Serial.begin(115200);
    // Create new task to handle the timer interrupt and start the motor timer task.
    Serial.println("Start MotorTimer task.");
    xTaskCreatePinnedToCore(
        [](void *param)
        {
            // Runs on core 0.
            Serial.println("MotorTimerTask is running on core " + String(xPortGetCoreID()) + ".");

            // The prescaler is used to divide the base clock frequency of the ESP32’s timer. The ESP32’s timer uses the APB clock (APB_CLK) as its base clock, which is normally 80 MHz. By setting the prescaler to 80, we are dividing the base clock frequency by 80, resulting in a timer tick frequency of 1 MHz (80 MHz / 80 = 1 MHz).
            MotorTimer::timerHandle = timerBegin(0, 80, true);
            timerAttachInterrupt(MotorTimer::timerHandle, &onMotorTimer, true);
            timerAlarmWrite(MotorTimer::timerHandle, iterationIntervalUS, true);
            timerAlarmEnable(MotorTimer::timerHandle);
            Serial.println("Just started timer.");

            MotorTimer::instance->runTask();
        },
        "MotorTimerTask",     // Task name
        10000,                // Stack size (bytes)
        NULL,                 // Parameter
        configMAX_PRIORITIES, // Task priority
        NULL,                 // Task handle
        0);                   // Core where the task should run
}