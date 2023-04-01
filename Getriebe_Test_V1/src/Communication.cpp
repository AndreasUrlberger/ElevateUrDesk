#include "Communication.hpp"
#include "Gearbox.hpp"
#include "Pinout.hpp"

bool Communication::receiveMessage(uint8_t (&message)[32]) // receive Message from I2C
{
    bool success = false;
    int i = 0;
    while (Wire.available() > 0)
    {
        message[i] = Wire.read();
        i++;
    }
    if (i == 31)
    {
        success = true;
    }
    return success;
}

void Communication::handleRequest(uint8_t (&message)[32], uint8_t (&reply)[32]) // decide what to do, based on the recieved message
{
    // requests always beginn with 0x0Y, where Y is the request type
    // replys always beginn with 0xYY, where Y is the same as the request type
    // further bytes are specific to the request type
    byte requestType = message[0];
    memset(reply, 0, 32); // Clear the reply (all 32 bytes to 0)
    switch (requestType)
    {
    case 0x01: // setup-command
        // process input
        //  maxSpeed
        //  maxAcceleration

        // action

        // send reply
        break;
    case 0x02: // run-command
        // process input
        //  maxSpeed
        //  targetMathematicaclHeight

        // action
        //  delete timer
        GearboxRun = true;
        //  create new timer

        // send reply
        // current position (skipped steps subtracted)
        break;
    case 0x03: // stop-command
        // process input

        // action
        // delete timer
        // GearboxRun = false;
        // Gearbox gearbox("Gearbox", 0, 0);
        // gearbox.Stop();

        // send reply
        break;
    case 0x0D: // debug-command
        // process input
        byte debugCommand = message[1];
        reply[0] = 0xDD; // first byte of reply
        switch (debugCommand)
        {
        case 0x0D:                  // toggle debug mode
            if (message[2] == 0x01) // turn debug mode on
            {
                DebugMode = true;
                reply[1] = 0x11; // second byte of reply, indicates that debug mode is activated
                Serial.println("Debug mode turned on");
            }
            else if (message[2] == 0x02) // turn debug mode off
            {
                DebugMode = false;
                reply[1] = 0x22; // second byte of reply, indicates that debug mode is deactivated
                Serial.println("Debug mode turned off");
            }
            else if (message[2] == 0x03) // reply debug mode to master
            {
                if (DebugMode == true)
                {
                    reply[1] = 0x11; // second byte of reply, indicates that debug mode is activated
                    Serial.println("Debug mode is on");
                }
                else
                {
                    reply[1] = 0x22; // second byte of reply, indicates that debug mode is deactivated
                    Serial.println("Debug mode is off");
                }
            }
            else
            {
                reply[1] = 0xEE; // second byte of reply, indicates that the command is unknown
                Serial.println("unknown command");
            }
            break;
        case 0x0A: // turn OTA update mode on for specified time or off (if time is 0)
        {
            byte OTAtimeout = message[2];
            if (OTAtimeout == 0x00)
            {
                // stop ota function
                reply[1] = 0xA0; // second byte of reply, indicates that OTA update mode is deactivated
                Serial.println("OTA update funtion disabled");
            }
            else
            {
                // start ota function for (OTAtimeout) seconds
                reply[1] = 0xAA;                                               // second byte of reply, indicates that OTA update mode is activated
                reply[2] = message[2];                                         // third byte of reply, indicates the set timeout
                Serial.println("OTA update funtion started, timeout xx sec."); //  TODO: add timeout to string
            }
        }
        case 0x0B: // return Software version
        {
            reply[1] = 0xBB;               // second byte of reply, indicates that the following bytes are the software version
            reply[2] = SoftwareVersion[0]; // third byte of reply, indicates the major software version
            reply[3] = SoftwareVersion[1]; // third byte of reply, indicates the minor software version
            reply[4] = SoftwareVersion[2]; // third byte of reply, indicates the bugfix version
            Serial.println("OTA update funtion stopped");
        }
        default:
            reply[1] = 0xEE; // second byte of reply, indicates that the command is unknown
            Serial.println("unknown command");
            break;
        }
    default: // command is unknown
        // process input
        reply[0] = 0xEE; // second byte of reply, indicates that the command is unknown
        Serial.println("unknown command");

        // send reply
        break;
        // TODO: SerialPrint reply if DEBUGMODE is on
    }
}
