#pragma once

#include <Arduino.h>

// Pinout for Gearbox (ESP32)

// Communication I2C (with Master)
#define CommunicationSDA -1
#define CommunicationSCL -1

// DeskMotor Control
#define MotorStep 254 // Step Input of Motor Controller
#define MotorDir 253  // Direction Input of Motor Controller

// DeskMotor Configuration (SPI and GPIOs)
#define MotorSDI -1
#define MotorSCK -1
#define MotorCS -1
#define MotorSDO -1

#define MotorEnable -1
#define MotorDCO -1

#define MotorDiag0 -1 // Number of skipped Steps ?
#define MotorDiag1 -1 // Alert if motor is stalled ?

// Primary Brake
#define LARGE_BRAKE_1 23
#define LARGE_BRAKE_2 19
#define LARGE_BRAKE_3 18
#define LARGE_BRAKE_4 5

// Secondary Brake
#define SMALL_BRAKE_1 32
#define SMALL_BRAKE_2 33
#define SMALL_BRAKE_3 25
#define SMALL_BRAKE_4 26

// DeskMotor
#define DESK_MOTOR_CS_PIN 15
#define DESK_MOTOR_R_SENSE 0.11f
#define DESK_MOTOR_STEP_PIN 16
#define DESK_MOTOR_DIR_PIN 27
#define DESK_MOTOR_EN_PIN 17
#define DESK_MOTOR_SPI_MOSI 13
#define DESK_MOTOR_SPI_MISO 12
#define DESK_MOTOR_SPI_SCK 14
#define DESK_MOTOR_SPI_SS 15

// Lightgates
#define LIGHTGATE_LARGE_BRAKE_OPEN 39
#define LIGHTGATE_LARGE_BRAKE_CLOSED 36
#define LIGHTGATE_SMALL_BRAKE_OPEN 35
#define LIGHTGATE_SMALL_BRAKE_CLOSED 34

// Relay
#define RELAY_3V 2 // for turning the motor control board on and off

// Rotary Sensor
#define ROTARY_SDA -1
#define ROTARY_SCL -1

static int minSteps = 0;        // 10000000; // 10 million steps, has to be checked, step 0 is counted from floor height, minSteps starts from the min. achievable position (incl. some buffer)
static int maxSteps = 40000000; // 40 million steps, has to be checked, steps from bottom to top