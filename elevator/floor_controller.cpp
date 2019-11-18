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

void FloorController::enterSafeMode() { safe_ = true; };

void FloorController::emergencyStop() {
  step_controller.emergencyStop();
  rotate_controller.emergencyStop();
};

void FloorController::seekToHome() {
  stepper_.setMaxSpeed(run_speed);
  stepper_.setAcceleration(run_accel);
  stepper_.setTargetAbs(named_stops_.at(0));
  step_controller.moveAsync(stepper_);
};

void FloorController::moveToStop(uint8_t stop_number) {
  if (stop_number >= kNumNamedStops) {
    return;
  } else {
    if (activity_state_ == MovementState::Overrun) {
      /// @todo check stepper value against target, don't allow any more forward
    } else if (activity_state_ == MovementState::Rotating) {
      rotate_controller.stop();
    }
    stepper_.setMaxSpeed(run_speed);
    stepper_.setAcceleration(run_accel);
    stepper_.setTargetAbs(named_stops_.at(stop_number));
    step_controller.moveAsync(stepper_);
  }
}

void FloorController::update() {
  static uint8_t overrun_state = 0;
  static uint8_t home_state = 0;
  bool updated = false;

  if (overrun_pin_.fell()) {
    overrun_state = 0;
    updated = true;
  } else if (overrun_pin_.rose()) {
    overrun_state = 1;
    updated = true;
  }

  if (home_pin_.fell()) {
    home_state = 0;
    updated = true;
  } else if (home_pin_.rose()) {
    home_state = 1;
    updated = true;
  }

  if (updated) {
    uint8_t this_state = overrun_state << 1 | home_state;
    switch (this_state) {
      case 0:
        // Somewhere out in the wilderness
        if (activity_state_ == MovementState::Home) {
          activity_state_ = MovementState::Unknown;
        }
        break;
      case 1:
        // On our way to or from homing, no need for alarm
        if (activity_state_ == MovementState::Home) {
          activity_state_ = MovementState::Unknown;
        }
        break;
      case 2:
        // Overrun case, no bueno
        activity_state_ = MovementState::Overrun;
        step_controller.stop();
        rotate_controller.stop();
        break;
      case 3:
        // Home case, this is good methinks
        if (activity_state_ != MovementState::Home) {
          activity_state_ = MovementState::Home;
          step_controller.stop();
          rotate_controller.stop();
          stepper_.setPosition(0);
        }
        break;
      default:
        break;
    }
  }
}

// Maintenence Mode Tasks
void FloorController::enterMaintainenceMode() { in_maintenence_mode_ = true; }
void FloorController::setStop(uint8_t stop_number) {
  if (in_maintenence_mode_ && stop_number > 0 && stop_number < kNumNamedStops) {
    named_stops_.at(stop_number) = stepper_.getPosition();
  }
}
void FloorController::rotate(bool clockwise) {
  if (in_maintenence_mode_) {
    if ((clockwise && activity_state_ == MovementState::Overrun) ||
        (!clockwise && activity_state_ == MovementState::Home)) {
      // Don't allow movement if we can't move
      return;
    }
    rotate_controller.stop();
    stepper_.setMaxSpeed(clockwise ? kHomingSpeed : -kHomingSpeed);
    stepper_.setAcceleration(kHomingAccel);
    rotate_controller.rotateAsync(stepper_);
  }
}
void FloorController::stopRotation() {
  if (in_maintenence_mode_) {
    rotate_controller.stopAsync();
  }
}
void FloorController::rotateWithStep(uint16_t steps, bool clockwise) {
  if (in_maintenence_mode_) {
  }
}
void FloorController::exitMaintainenceMode() { in_maintenence_mode_ = false; }
