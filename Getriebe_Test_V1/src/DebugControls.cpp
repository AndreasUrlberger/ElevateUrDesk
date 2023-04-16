// #include "DebugControls.hpp"
// #include "TextInput.hpp"
// #include <chrono>
// #include <thread>

// DeskMotor *DebugControls::mDeskMotor;
// Communication *DebugControls::mCommunication;
// bool DebugControls::isDebugButtonPressed = false;
// TaskHandle_t DebugControls::moveProcessTaskHandle;

// long targetPosition = 0; // target position for motor, should be set by DeskMotor to the current position of the motor
// int stepsToAdd = 1000;   // only for debug purposes

// void DebugControls::mainMenu() // main menu for Gearbox debug mode
// {
//   Serial.printf("Target loop : %d \n", mDeskMotor->targetPosition);

//   Serial.println("Debugmode active:");
//   Serial.println("1. I2C-Debug Mode");
//   Serial.println("2. OTA Update Mode");
//   Serial.println("3. WLED Debug Mode");
//   Serial.println("4. Monitor Debug Mode");
//   Serial.println("5. DeskMotor Debug Mode");
//   int debugFeature = TextInput::getIntInput();
//   handleDebugMode(debugFeature);
// }

// void DebugControls::debugButton1Press(bool isPressed)
// {
//   // action to do if DebugButton1 is pressed

//   // increase targetPosition by xxx steps
//   // targetPosition += stepsToAdd;
//   // mDeskMotor->setNewTargetPosition(targetPosition);
//   // Serial.printf("targetPosition increased by %d steps to %d \n", stepsToAdd, targetPosition);

//   isDebugButtonPressed = isPressed;
// }

// void DebugControls::buttonPressProcess(void *arg)
// {
//   // Create chrono start time
//   const auto start = std::chrono::high_resolution_clock::now();
//   // Chrono iteration time
//   const auto iteration = std::chrono::milliseconds(mDeskMotor->moveInputIntervalMS);
//   // chrono target time
//   auto target = start;

//   Serial.println("buttonPressProcess started");

//   while (true)
//   {
//     // Execute code
//     if (isDebugButtonPressed)
//     {
//       mCommunication->moveUp();
//     }

//     // Give Arduino time to do other things.
//     yield();

//     target += iteration;
//     std::this_thread::sleep_until(target);
//   }
// }

// void DebugControls::handleDebugMode(int debugFeature)
// {
//   switch (debugFeature)
//   {
//   case 1: // mode for debugging I2C-Communication
//     Serial.println("I2C-Debug Mode activated");
//     break;
//   case 2:
//     Serial.println("OTA Update Mode selection activated");
//     break;
//   case 3:
//     Serial.println("WLED Debug Mode activated");
//     break;
//   case 4:
//     Serial.println("Monitor Debug Mode activated");
//     break;
//   case 5: // mode for debugging motor control
//     Serial.println("DeskMotor Debug Mode activated");
//     motorDebug();
//     break;
//   default:
//     Serial.println("unknown debug mode");
//     break;
//   }
// }

// void DebugControls::motorDebug()
// {
//   Serial.println("choose function of motor to debug:");
//   Serial.println("1. set motor acceleration, speed and steps");
//   Serial.println("2. custom motor test");
//   switch (TextInput::getIntInput())
//   {
//   case 1:
//     // simple motor test
//     break;
//   case 2:
//     // custom motor test
//     customMotorTest();
//     break;
//   default:
//     Serial.println("unknown motor debug mode");
//     break;
//   }
// }

// void DebugControls::customMotorTest()
// {
//   // custom motor test
//   Serial.println("custom motor test started");

//   int repeat = 3;       // repeat motor test with same settings if == 1, with new settings if == 2
//   long currentPos = 0;  // current position of motor
//   int speed = 0;        // speed of motor
//   int acceleration = 0; // acceleration of motor

//   do
//   {
//     // read out current position
//     currentPos = mDeskMotor->getCurrentPosition();

//     // motor setup
//     Serial.println("max motor speed: ");
//     if (repeat != 1)
//     {
//       speed = TextInput::getIntInput();
//     }
//     Serial.println(speed);

//     Serial.println("motor acceleration: ");
//     if (repeat != 1)
//     {
//       acceleration = TextInput::getIntInput();
//     }
//     mDeskMotor->setMaxAcceleration(acceleration);
//     Serial.println(acceleration);

//     // set target position
//     Serial.println("target position: ");
//     if (repeat != 1)
//     {
//       targetPosition = TextInput::getIntInput();
//     }
//     Serial.println(targetPosition);

//     // set amount of steps to add with each button press
//     Serial.println("steps to add with each button press: ");
//     if (repeat != 1)
//     {
//       stepsToAdd = TextInput::getIntInput();
//     }
//     Serial.println(stepsToAdd);

//     mDeskMotor->setCurrentPosition(currentPos);
//     // run motor
//     mDeskMotor->setup();
//     mDeskMotor->init(speed);
//     mDeskMotor->setNewTargetPosition(targetPosition);
//     mDeskMotor->start();

//     // reset current position to previous position
//     // mDeskMotor->setCurrentPosition(currentPos);

//     // decide on next action
//     Serial.println("press 1 for same settings, 2 for new setup: ");
//     repeat = TextInput::getIntInput();
//     if (repeat == 1)
//     {
//       Serial.println("--------------------");
//       Serial.println("running again with following settings:");
//     }
//     else if (repeat == 2)
//     {
//       Serial.println("--------------------");
//       Serial.println("enter new values:");
//     }

//   } while (repeat == 1 || repeat == 2);
//   Serial.println("exiting motor debug mode");
// }

// void DebugControls::I2Cdebug()
// {
//   // enable communication with slave devices
//   // first: select address of slave device
//   // second: define data to send to slave device
//   // third: display answer of I2C slave device
// }

// void DebugControls::initDebugControls(Communication *const communication)
// {
//   mCommunication = communication;
//   mDeskMotor = &(communication->gearbox.deskMotor);

//   // Run task on main thread, not on the motor thread!
//   const BaseType_t currentCoreId = xPortGetCoreID();

//   // xTaskCreatePinnedToCore(
//   //     buttonPressProcess,     /* Function to implement the task */
//   //     "Task2",                /* Name of the task */
//   //     2048,                   /* Stack size in words, no idea what size to use */
//   //     NULL,                   /* Task input parameter */
//   //     0,                      /* Priority of the task */
//   //     &moveProcessTaskHandle, /* Task handle. */
//   //     currentCoreId);         /* Core where the task should run */
// }