#include <AccelStepper.h>

// --- Define pins based on NEW wiring ---
// Motor 1
const int stepPin1 = 33;
const int dirPin1 = 32;
// Motor 2
const int stepPin2 = 26;
const int dirPin2 = 25;
// Motor 3
const int stepPin3 = 21;
const int dirPin3 = 22;
// Motor 4
const int stepPin4 = 16;
const int dirPin4 = 17;
// Motor 5
const int stepPin5 = 2;
const int dirPin5 = 4;
// --- End of Pin Definitions ---

// Define AccelStepper instances for each motor
AccelStepper motor1(AccelStepper::DRIVER, stepPin1, dirPin1);
AccelStepper motor2(AccelStepper::DRIVER, stepPin2, dirPin2);
AccelStepper motor3(AccelStepper::DRIVER, stepPin3, dirPin3);
AccelStepper motor4(AccelStepper::DRIVER, stepPin4, dirPin4);
AccelStepper motor5(AccelStepper::DRIVER, stepPin5, dirPin5);

// Array of motor pointers for easier iteration
AccelStepper* motors[] = {&motor1, &motor2, &motor3, &motor4, &motor5};
const int NUM_MOTORS = 5;

// Wave parameters
const float maxSpeed = 1000.0;     // Max steps per second (adjust for your motors)
const float acceleration = 500.0;  // Steps per second per second (adjust)
const long travelDistance = 1600;  // Steps for up/down travel (e.g., one full revolution if 1600 pulses/rev)

// Phase shift for the sine wave
const float phaseIncrement = 1.0 / NUM_MOTORS;

// Timing for the wave
unsigned long waveStartTime = 0;
const float wavePeriodSeconds = 5.0; // Time for one full wave cycle in seconds

// Delay before starting the motion
const unsigned long startupDelayMilliseconds = 10000; // 10 seconds

// Parameters for initial settling movement
const long settlingSteps = 100; // Number of steps to move for settling (e.g., a small "up" movement)
                               // Adjust this based on how much initial movement you want/need
const unsigned long settlingTimeoutMilliseconds = 3000; // Max time to wait for settling (3 seconds)

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("AccelStepper Wave Demo Initializing...");
  Serial.print("Waiting for ");
  Serial.print(startupDelayMilliseconds / 1000);
  Serial.println(" seconds before starting motors...");

  // --- ADDED STARTUP DELAY ---
  delay(startupDelayMilliseconds);
  // --- END OF ADDED STARTUP DELAY ---

  Serial.println("Initializing motor parameters for AccelStepper...");
  for (int i = 0; i < NUM_MOTORS; i++) {
    motors[i]->setMaxSpeed(maxSpeed);
    motors[i]->setAcceleration(acceleration);
    // We will set current position AFTER the settling movement
  }

  // --- INITIAL SETTLING MOVEMENT ---
  Serial.println("Performing initial settling movement...");
  for (int i = 0; i < NUM_MOTORS; i++) {
    motors[i]->setCurrentPosition(0); // Temporarily define current as 0 for this move
    motors[i]->moveTo(settlingSteps); // Tell all motors to move to the settling position
  }

  unsigned long settleMoveStartTime = millis();
  bool allMotorsSettled = false;
  while (!allMotorsSettled && (millis() - settleMoveStartTime < settlingTimeoutMilliseconds)) {
    allMotorsSettled = true; // Assume settled until a motor is found still moving
    for (int i = 0; i < NUM_MOTORS; i++) {
      motors[i]->run();
      if (motors[i]->distanceToGo() != 0) {
        allMotorsSettled = false; // If any motor still has distance to go, not all settled
      }
    }
  }
  if (allMotorsSettled) {
    Serial.println("Settling movement complete.");
  } else {
    Serial.println("Settling movement timed out.");
  }
  // --- END OF INITIAL SETTLING MOVEMENT ---

  // NOW, define the current physical position (after settling) as the logical zero for the wave
  for (int i = 0; i < NUM_MOTORS; i++) {
    motors[i]->setCurrentPosition(0);
  }

  waveStartTime = millis(); // Set the wave start time AFTER the delay and settling
  Serial.println("Setup complete. Motors should start wave motion NOW.");
  Serial.println("--- IMPORTANT: Vref should be set on ALL FIVE TMC2209 drivers! ---");
}

void loop() {
  // Calculate the current time within the wave period
  float currentTimeFraction = (float)(millis() - waveStartTime) / (wavePeriodSeconds * 1000.0);

  // Loop the wave
  if (currentTimeFraction >= 1.0) {
    currentTimeFraction -= 1.0; // Wrap around
    waveStartTime = millis();     // Reset start time for next cycle
  }

  // Calculate target position for each motor based on a sine wave
  for (int i = 0; i < NUM_MOTORS; i++) {
    // Calculate the phase for this motor
    float motorPhase = currentTimeFraction + (i * phaseIncrement);
    while (motorPhase >= 1.0) motorPhase -= 1.0;
    while (motorPhase < 0.0) motorPhase += 1.0;

    // Calculate target position using a sine wave
    long targetPosition = (long)(((sin(motorPhase * 2.0 * PI) + 1.0) / 2.0) * travelDistance);
    
    motors[i]->moveTo(targetPosition);
  }

  // Run all motors
  for (int i = 0; i < NUM_MOTORS; i++) {
    motors[i]->run();
  }
}