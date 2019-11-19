/** Implementation details for door controller
 *
 */

#include "door_controller.h"

using namespace hiproof::elevator;

StepControl DoorController::step_controller{25};
RotateControl DoorController::rotate_controller{25};

DoorController::DoorController(const int d1_step_pin, const int d1_dir_pin,
                               const int d1_home_pin, const int d1_overrun_pin,
                               const int d2_step_pin, const int d2_dir_pin,
                               const int d2_home_pin, const int d2_overrun_pin)
    : left_door_{.stepper = Stepper(d1_step_pin, d1_step_pin),
                 .home_pin = Bounce(),
                 .overrun_pin = Bounce()},
      right_door_{.stepper = Stepper(d2_step_pin, d2_step_pin),
                  .home_pin = Bounce(),
                  .overrun_pin = Bounce()} {
  left_door_.home_pin.attach(d1_home_pin, INPUT_PULLUP);
  left_door_.home_pin.interval(config::kPinDebounceInterval);
  left_door_.overrun_pin.attach(d1_overrun_pin, INPUT_PULLUP);
  left_door_.overrun_pin.interval(config::kPinDebounceInterval);
  right_door_.home_pin.attach(d2_home_pin, INPUT_PULLUP);
  right_door_.home_pin.interval(config::kPinDebounceInterval);
  right_door_.overrun_pin.attach(d2_overrun_pin, INPUT_PULLUP);
  right_door_.overrun_pin.interval(config::kPinDebounceInterval);
}

void DoorController::enterSafeMode() {
  rotate_controller.stop();
  step_controller.stop();
  open();
  safe_ = true;
}

void DoorController::emergencyStop() {
  step_controller.emergencyStop();
  rotate_controller.emergencyStop();
}

void DoorController::open() {
  left_door_.stepper.setMaxSpeed(-config::kDoorMaxLiveSpeed);
  right_door_.stepper.setMaxSpeed(config::kDoorMaxLiveSpeed);

  left_door_.stepper.setAcceleration(config::kDoorMaxLiveAccel);
  right_door_.stepper.setAcceleration(config::kDoorMaxLiveAccel);
  if (left_door_.home_pin.read() == 1 && right_door_.home_pin.read() == 1) {
    /// @todo open
    rotate_controller.rotateAsync(left_door_.stepper, right_door_.stepper);
  } else {
    if (left_door_.home_pin.read() == 1) {
      /// @todo open only left
      rotate_controller.rotateAsync(left_door_.stepper);
    } else {
      /// @todo open only right
      rotate_controller.rotateAsync(right_door_.stepper);
    }
  }
}

void DoorController::close() {
  left_door_.stepper.setMaxSpeed(config::kDoorMaxLiveSpeed);
  right_door_.stepper.setMaxSpeed(-config::kDoorMaxLiveSpeed);

  left_door_.stepper.setAcceleration(config::kDoorMaxLiveAccel);
  right_door_.stepper.setAcceleration(config::kDoorMaxLiveAccel);

  if (left_door_.overrun_pin.read() == 1 && right_door_.overrun_pin.read() == 1) {
    /// @todo close
    rotate_controller.rotateAsync(left_door_.stepper, right_door_.stepper);
  } else {
    if (left_door_.overrun_pin.read() == 1) {
      /// @todo close only left
      rotate_controller.rotateAsync(left_door_.stepper);
    } else {
      /// @todo close only right
      rotate_controller.rotateAsync(right_door_.stepper);
    }
  }
}

void DoorController::update() {
  bool left_stopped = false;
  bool right_stopped = false;
  if (left_door_.overrun_pin.fell() || left_door_.home_pin.fell()) {
    rotate_controller.stop();
    left_stopped = true;
  }
  if (right_door_.overrun_pin.fell() || right_door_.home_pin.fell()) {
    rotate_controller.stop();
    right_stopped = true;
  } else if (left_stopped) {
    /// @todo continue with moving right door until stop
  }
  if (!left_stopped && right_stopped &&
      left_door_.home_pin.read() == left_door_.overrun_pin.read()) {
    /// @todo continue moving left door until stop
  }
}
