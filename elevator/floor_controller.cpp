/** Floor controller impl details
 *
 */

#include "floor_controller.h"

#include "Arduino.h"

using namespace hiproof::elevator;

// For future reference, there are the pulse widths of the controllers
StepControl FloorController::step_controller{25};
RotateControl FloorController::rotate_controller{25};

FloorController::FloorController(const int step_pin, const int dir_pin,
                                 const int home_pin, const int overrun_pin)
    : named_stops_{},
      stepper_{step_pin, dir_pin},
      home_pin_{},
      overrun_pin_{},
      rc_active{false},
      sc_active{false} {
  home_pin_.attach(home_pin, INPUT_PULLUP);
  home_pin_.interval(config::kPinDebounceInterval);
  overrun_pin_.attach(overrun_pin, INPUT_PULLUP);
  overrun_pin_.interval(config::kPinDebounceInterval);
}
FloorController::FloorController(config::EthernetConnectorPins conn)
    : named_stops_{},
      stepper_{conn.step_pin, conn.dir_pin},
      home_pin_{},
      overrun_pin_{} {
  home_pin_.attach(conn.home_pin, INPUT_PULLUP);
  home_pin_.interval(config::kPinDebounceInterval);
  overrun_pin_.attach(conn.overrun_pin, INPUT_PULLUP);
  overrun_pin_.interval(config::kPinDebounceInterval);
}

FloorController::~FloorController() {}

void FloorController::enterSafeMode() {
  seekToHome();
  safe_ = true;
};

void FloorController::emergencyStop() {
  step_controller.emergencyStop();
  rotate_controller.emergencyStop();
  sc_active = false;
  rc_active = false;
};

void FloorController::seekToHome() {
  stepper_.setMaxSpeed(run_speed);
  stepper_.setAcceleration(run_accel);
  stepper_.setTargetAbs(named_stops_[0]);
  startStepController();
};

void FloorController::moveToStop(uint8_t stop_number) {
  if (stop_number >= kNumNamedStops) {
    return;
  } else {
    stopRotateController();
    stopStepController();
    stepper_.setMaxSpeed(run_speed);
    stepper_.setAcceleration(run_accel);
    stepper_.setTargetAbs(named_stops_[stop_number]);
    startStepController();
  }
}

void FloorController::update() {
  if (overrun_pin_.fell()) {
    if (home_pin_.read() == 1) {
      // Overrun case, no bueno
      stopRotateController();
      stopStepController();
    } else if (home_pin_.rose()) {
      // Home case, this is good methinks
      stopRotateController();
      stopStepController();
      stepper_.setPosition(0);
    }
  }
}

void FloorController::stopAsync() {
  stopRotateController();
  stopStepController();
}

// Maintenence Mode Tasks

void FloorController::setStop(uint8_t stop_number) {
  if (stop_number > 0 && stop_number < kNumNamedStops) {
    named_stops_[stop_number] = stepper_.getPosition();
  }
}
void FloorController::rotate(bool clockwise) {
  if ((clockwise && overrun_pin_.read() == 0 && overrun_pin_.duration() > 0 &&
       home_pin_.read() == 1 && home_pin_.duration() > 0) ||
      (!clockwise && overrun_pin_.read() == 0 && overrun_pin_.duration() > 0 &&
       home_pin_.read() == 0 && home_pin_.duration() > 0)) {
    // Don't allow movement if we can't move
    return;
  }
  Serial.printf("Rotating %s\r\n",
                clockwise ? "Clockwise" : "Counterclockwise");
  stopRotateController();
  stopStepController();
  stepper_.setMaxSpeed(clockwise ? config::kFloorHomingSpeed
                                 : -config::kFloorHomingSpeed);
  stepper_.setAcceleration(config::kFloorHomingAccel);
  Serial.printf("Starting rotation\r\n");
  startRotateController();
}
void FloorController::stopRotation() { stopRotateController(); }

void FloorController::startStepController() {
  step_controller.moveAsync(stepper_);
  Serial.printf("StepStarted\r\n");
  sc_active = true;
}
void FloorController::stopStepController() {
  if (sc_active) {
    step_controller.stop();
    sc_active = false;
  }
}
void FloorController::startRotateController() {
  // rotate_controller.rotateAsync(stepper_);
  Serial.printf("RotationStarted\r\n");
  rc_active = true;
}
void FloorController::stopRotateController() {
  if (rc_active) {
    rotate_controller.stop();
    rc_active = false;
  }
}
