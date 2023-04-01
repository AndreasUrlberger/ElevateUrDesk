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
#define PrimaryBrake1 4 // Primary brake motor, input 1
#define PrimaryBrake2 0
#define PrimaryBrake3 5
#define PrimaryBrake4 15

// Secondary Brake
#define SecondaryBrake1 255 // Secondary brake motor, input 1
#define SecondaryBrake2 255
#define SecondaryBrake3 255
#define SecondaryBrake4 255

// Lightgates
#define PrimarySensorOpen -1   // Lightgatee for sensing if primary brake is opened
#define PrimarySensorClosed -1 // Lightgatee for sensing if primary brake is closed
#define SecondarySensorOpen -1
#define SecondarySensorClosed -1

// Relay
#define Relay3V -1  // for turning the motor control board on and off
#define Relay24V -1 // only if neccessary and pin is available

// Rotary Sensor
#define RotarySDA -1
#define RotarySCL -1

// other variables
static bool GearboxRun = false;
static bool DebugMode = true;

static int minSteps = 10000000; // 10 million steps, has to be checked, step 0 is counted from floor height, minSteps starts from the min. achievable position (incl. some buffer)
static int maxSteps = 40000000; // 40 million steps, has to be checked, steps from bottom to top

static int motorTimeout = 50; // 50ms, has to be checked, timeout for motor until it powers off by starting to decelerate

static uint8_t SoftwareVersion[3]{0x01, 0x00, 0x00}; // Major Version, Minor Version, BugFix