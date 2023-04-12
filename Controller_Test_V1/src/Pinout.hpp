#pragma once

// Pinout for Controller (ESP32)

// Communication I2C (with Slaves)
#define GearboxCommunicationSDA -1
#define GearboxCommunicationSCL -1
#define GearboxLeftAddress -1  // I2C-Address of left Gearbox
#define GearboxRightAddress -1 // I2C-Address of right Gearbox

// Lightgates
#define TopSensor -1    // Lightgatee for sensing if desk has reached the top limit
#define BottomSensor -1 // Lightgate for sensing if desk has reached the bottom limit

// Relay
#define Relay12V -1 // for turning the 12V PSU on and off (LEDs)
#define Relay24V -1 // fpr turning the 24V PSU on and off (Motors)

// Screen
//  SPI Communication?

// Buttonpanel
//  UART?

// WLED
//  UART

// Monitor Adjustment Motors
//  I2C?

// other variables
static bool DebugMode = true;

static uint8_t SoftwareVersion[3]{0x01, 0x00, 0x00}; // Major Version, Minor Version, BugFix