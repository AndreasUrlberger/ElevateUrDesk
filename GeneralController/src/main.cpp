#include <Arduino.h>
#include <chrono>
#include <thread>
#include <Wire.h>
#include "GearboxCommunication.hpp"
#include "InputController.hpp"
#include "ControlPanelCommunication.hpp"
#include <queue>

static constexpr uint8_t GEARBOX_LEFT_ADDRESS = 0x33;
static constexpr uint8_t GEARBOX_RIGHT_ADDRESS = 0x88;

// Gearboxes I2C Connection.
static constexpr int I2C_SDA_PIN = 21;
static constexpr int I2C_SCL_PIN = 22;
static constexpr uint32_t I2C_FREQ = 100000u;

// Control Panel Uart Connection.
static constexpr int8_t UART_TX_PIN = 17;
static constexpr int8_t UART_RX_PIN = 16;
static constexpr uint32_t UART_CONFIG = SERIAL_8N1;
static constexpr uint32_t UART_BAUDRATE = 115200u;

std::chrono::steady_clock::time_point start;
std::chrono::steady_clock::time_point target;
std::chrono::steady_clock::duration iterationDuration = std::chrono::milliseconds(10);

GearboxCommunication gearbox(GEARBOX_LEFT_ADDRESS, GEARBOX_RIGHT_ADDRESS, &Wire, I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
std::queue<InputEvent *> eventQueue;
InputController inputController(&gearbox, &eventQueue);
ControlPanelCommunication controlPanelCommunication(&eventQueue, UART_TX_PIN, UART_RX_PIN, UART_CONFIG, UART_BAUDRATE);

void setup()
{
  // Initialize Serial communication
  Serial.begin(115200);

  // Initialize start time
  start = std::chrono::steady_clock::now();
  target = start;
}

void loop()
{
  // Wait till the next iteration should start.
  std::this_thread::sleep_until(target);

  // Read all messages from the control panel.
  while (controlPanelCommunication.update())
  {
  }

  inputController.update();

  // Update the target time for the next iteration.
  target += iterationDuration;
}