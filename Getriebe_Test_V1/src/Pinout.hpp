#pragma once

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
#define LARGE_BRAKE_1 27
#define LARGE_BRAKE_2 14
#define LARGE_BRAKE_3 12
#define LARGE_BRAKE_4 13

// Secondary Brake
#define SMALL_BRAKE_1 32
#define SMALL_BRAKE_2 33
#define SMALL_BRAKE_3 25
#define SMALL_BRAKE_4 26

// Motor Pins
#define DESK_MOTOR_1 23
#define DESK_MOTOR_2 19
#define DESK_MOTOR_3 18
#define DESK_MOTOR_4 5

// Lightgates
#define LIGHTGATE_LARGE_BRAKE_OPEN 36
#define LIGHTGATE_LARGE_BRAKE_CLOSED 39
#define LIGHTGATE_SMALL_BRAKE_OPEN 34
#define LIGHTGATE_SMALL_BRAKE_CLOSED 35

// Relay
#define RELAT_3V -1  // for turning the motor control board on and off
#define RELAT_24V -1 // only if neccessary and pin is available

// Rotary Sensor
#define ROTARY_SDA -1
#define ROTARY_SCL -1

// only for debug purposes
#define DebugButton1 35
#define DebugButton2 -1

// other variables
static bool debugMode = true;

static int minSteps = 0;        // 10000000; // 10 million steps, has to be checked, step 0 is counted from floor height, minSteps starts from the min. achievable position (incl. some buffer)
static int maxSteps = 40000000; // 40 million steps, has to be checked, steps from bottom to top

static int lastCurrentPosition = minSteps; // last position of the desk before power off, is used to restore the desk position after power on, TODO: actually save it somewhere
static int currentPosition = minSteps;     // current position of the desk, standard value is minSteps

static int motorTimeout = 50; // 50ms, has to be checked, timeout for motor until it powers off by starting to decelerate

static uint8_t SoftwareVersion[3]{0x01, 0x00, 0x00}; // Major Version, Minor Version, Bugfix