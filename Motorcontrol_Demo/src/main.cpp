#include <Arduino.h>

#include <TMCStepper.h>
#include <AccelStepper.h>

#define DESK_MOTOR_CS_PIN 15
#define DESK_MOTOR_R_SENSE 0.11f
#define DESK_MOTOR_STEP_PIN 16
#define DESK_MOTOR_DIR_PIN 27
#define DESK_MOTOR_EN_PIN 17

#define SPI_MOSI 13
#define SPI_MISO 12
#define SPI_SCK 14
#define SPI_SS 15

#define RELAY_3V 2

#define MICROSTEPS 0
#define MAX_SPEED 1000
#define MAX_ACCELERATION 100

TMC2130Stepper driver = TMC2130Stepper(DESK_MOTOR_CS_PIN, DESK_MOTOR_R_SENSE); // Hardware SPI
AccelStepper deskMotor = AccelStepper(deskMotor.DRIVER, DESK_MOTOR_STEP_PIN, DESK_MOTOR_DIR_PIN);

uint32_t iterations = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting up...");

  pinMode(RELAY_3V, OUTPUT);
  digitalWrite(RELAY_3V, HIGH);

  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_SS);

  pinMode(DESK_MOTOR_CS_PIN, OUTPUT);
  digitalWrite(DESK_MOTOR_CS_PIN, LOW);

  driver.begin();           // Initiate pins and registeries
  driver.rms_current(600);  // Set stepper current to 600mA. The command is the same as command TMC2130.setCurrent(600, 0.11, 0.5);
  driver.en_pwm_mode(true); // Enable extremely quiet stepping
  driver.pwm_autoscale(true);
  driver.microsteps(MICROSTEPS);
  driver.intpol(true);

  deskMotor.setCurrentPosition(0);
  deskMotor.setMaxSpeed(MAX_SPEED);
  deskMotor.setAcceleration(MAX_ACCELERATION);
  deskMotor.setEnablePin(DESK_MOTOR_EN_PIN);
  deskMotor.setPinsInverted(false, false, true);
  deskMotor.enableOutputs();

  deskMotor.moveTo(50000);

  // const uint32_t lostSteps = driver.LOST_STEPS();

  // Serial.println("Main motor initialized");

  // Print current inpol setting
  Serial.print("inpol: ");
  Serial.println(driver.intpol() ? "true" : "false");
  // Print output of chopconf register
  Serial.print("chopconf: 0x");
  Serial.println(driver.CHOPCONF(), HEX);

  sleep(3);
}

void loop()
{
  // Serial.printf("Main motor running, iteration %d\n", iterations);
  iterations++;
  deskMotor.run();
  delayMicroseconds(10);
  // digitalWrite(DESK_MOTOR_DIR_PIN, ((iterations % 2) == 0) ? HIGH : LOW);

  // if (iterations % 2000 == 0)
  // {
  //   const uint32_t lostSteps = driver.LOST_STEPS();
  // }
}
