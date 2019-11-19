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
    : named_stops_{}, stepper_{step_pin, dir_pin}, home_pin_{}, overrun_pin_{} {
  home_pin_.attach(home_pin, INPUT_PULLUP);
  home_pin_.interval(kDebounceInterval);
  overrun_pin_.attach(overrun_pin, INPUT_PULLUP);
  overrun_pin_.interval(kDebounceInterval);
}

FloorController::~FloorController() {}

void FloorController::enterSafeMode() {
  seekToHome();
  safe_ = true;
};

void FloorController::emergencyStop() {
  step_controller.emergencyStop();
  rotate_controller.emergencyStop();
};

void FloorController::seekToHome() {
  stepper_.setMaxSpeed(run_speed);
  stepper_.setAcceleration(run_accel);
  stepper_.setTargetAbs(named_stops_[0]);
  step_controller.moveAsync(stepper_);
};

void FloorController::moveToStop(uint8_t stop_number) {
  if (stop_number >= kNumNamedStops) {
    return;
  } else {
    rotate_controller.stop();
    stepper_.setMaxSpeed(run_speed);
    stepper_.setAcceleration(run_accel);
    stepper_.setTargetAbs(named_stops_[stop_number]);
    step_controller.moveAsync(stepper_);
  }
}

void FloorController::update() {
  if (overrun_pin_.fell()) {
    if (home_pin_.read() == 1) {
      // Overrun case, no bueno
      step_controller.stop();
      rotate_controller.stop();
    } else if (home_pin_.rose()) {
      // Home case, this is good methinks
      step_controller.stop();
      rotate_controller.stop();
      stepper_.setPosition(0);
    }
  }
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
  rotate_controller.stop();
  stepper_.setMaxSpeed(clockwise ? kHomingSpeed : -kHomingSpeed);
  stepper_.setAcceleration(kHomingAccel);
  rotate_controller.rotateAsync(stepper_);
}
void FloorController::stopRotation() { rotate_controller.stopAsync(); }
