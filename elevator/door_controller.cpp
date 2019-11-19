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

DoorController::DoorController(config::EthernetConnectorPins left,
                               config::EthernetConnectorPins right)
    : left_door_{.stepper = Stepper(left.step_pin, left.dir_pin),
                 .home_pin = Bounce(),
                 .overrun_pin = Bounce()},
      right_door_{.stepper = Stepper(right.step_pin, right.dir_pin),
                  .home_pin = Bounce(),
                  .overrun_pin = Bounce()} {
  left_door_.home_pin.attach(left.home_pin, INPUT_PULLUP);
  left_door_.home_pin.interval(config::kPinDebounceInterval);
  left_door_.overrun_pin.attach(left.overrun_pin, INPUT_PULLUP);
  left_door_.overrun_pin.interval(config::kPinDebounceInterval);
  right_door_.home_pin.attach(right.home_pin, INPUT_PULLUP);
  right_door_.home_pin.interval(config::kPinDebounceInterval);
  right_door_.overrun_pin.attach(right.overrun_pin, INPUT_PULLUP);
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

void DoorController::init() {
  /// @todo Interactive door setup (through CLI?)
}

void DoorController::open() {
  left_door_.stepper.setMaxSpeed(-config::kDoorMaxLiveSpeed);
  right_door_.stepper.setMaxSpeed(-config::kDoorMaxLiveSpeed);

  left_door_.stepper.setAcceleration(config::kDoorMaxLiveAccel);
  right_door_.stepper.setAcceleration(config::kDoorMaxLiveAccel);

  left_door_.stepper.setTargetAbs(0);
  right_door_.stepper.setTargetAbs(0);

  if (left_door_.home_pin.read() == 1 && right_door_.home_pin.read() == 1) {
    /// @todo open
    step_controller.moveAsync(left_door_.stepper, right_door_.stepper);
  } else {
    if (left_door_.home_pin.read() == 1) {
      /// @todo open only left
      step_controller.moveAsync(left_door_.stepper);
    } else {
      /// @todo open only right
      step_controller.moveAsync(right_door_.stepper);
    }
  }
}

void DoorController::close() {
  left_door_.stepper.setMaxSpeed(config::kDoorMaxLiveSpeed);
  right_door_.stepper.setMaxSpeed(config::kDoorMaxLiveSpeed);

  left_door_.stepper.setAcceleration(config::kDoorMaxLiveAccel);
  right_door_.stepper.setAcceleration(config::kDoorMaxLiveAccel);

  left_door_.stepper.setTargetAbs(left_door_.max_travel);
  right_door_.stepper.setTargetAbs(right_door_.max_travel);

  if (left_door_.overrun_pin.read() == 1 &&
      right_door_.overrun_pin.read() == 1) {
    step_controller.moveAsync(left_door_.stepper, right_door_.stepper);
  } else {
    if (left_door_.overrun_pin.read() == 1) {
      step_controller.moveAsync(left_door_.stepper);
    } else {
      step_controller.moveAsync(right_door_.stepper);
    }
  }
}

void DoorController::stopAsync() { step_controller.stopAsync(); }

void DoorController::homingRoutine() {
  /// @todo Can we home asyncronously?
}

void DoorController::update() {
  bool left_stopped = false;
  bool right_stopped = false;
  if (left_door_.overrun_pin.fell() || left_door_.home_pin.fell()) {
    step_controller.stop();
    left_stopped = true;
  }
  if (right_door_.overrun_pin.fell() || right_door_.home_pin.fell()) {
    step_controller.stop();
    right_stopped = true;
  } else if (left_stopped) {
    /// @todo continue with moving right door until stop
  }
  if (!left_stopped && right_stopped &&
      left_door_.home_pin.read() == left_door_.overrun_pin.read()) {
    /// @todo continue moving left door until stop
  }
}

// Some debug functionality
void DoorController::openLeft() {
  left_door_.stepper.setMaxSpeed(-config::kDoorMaxLiveSpeed);

  left_door_.stepper.setAcceleration(config::kDoorMaxLiveAccel);

  left_door_.stepper.setTargetAbs(0);

  if (left_door_.home_pin.read() == 1) {
    step_controller.moveAsync(left_door_.stepper);
  }
}
void DoorController::closeLeft() {
  left_door_.stepper.setMaxSpeed(config::kDoorMaxLiveSpeed);

  left_door_.stepper.setAcceleration(config::kDoorMaxLiveAccel);

  left_door_.stepper.setTargetAbs(left_door_.max_travel);

  if (left_door_.overrun_pin.read() == 1) {
    step_controller.moveAsync(left_door_.stepper);
  }
}
void DoorController::homeLeft() {
  left_door_.stepper.setAcceleration(config::kDoorHomingAccel);

  // open first
  if (!left_door_.home_pin.read()) {
    left_door_.stepper.setMaxSpeed(-config::kDoorHomingSpeed);
    rotate_controller.rotateAsync(left_door_.stepper);
    while (!left_door_.home_pin.read())
      ;
    rotate_controller.stop();
  }

  // reset the open position to 0
  left_door_.stepper.setPosition(0);
  left_door_.stepper.setMaxSpeed(config::kDoorHomingSpeed);

  if (!left_door_.overrun_pin.read()) {
    left_door_.stepper.setMaxSpeed(config::kDoorHomingSpeed);
    rotate_controller.rotateAsync(left_door_.stepper);
    while (!left_door_.overrun_pin.read())
      ;
    rotate_controller.stop();
  }

  // motor is now is now in the closed position
  left_door_.max_travel = left_door_.stepper.getPosition();

  left_door_.stepper.setMaxSpeed(config::kDoorMaxLiveSpeed);
  left_door_.stepper.setAcceleration(config::kDoorMaxLiveAccel);
}

void DoorController::openRight() {
  right_door_.stepper.setMaxSpeed(-config::kDoorMaxLiveSpeed);

  right_door_.stepper.setAcceleration(config::kDoorMaxLiveAccel);

  right_door_.stepper.setTargetAbs(right_door_.max_travel);

  if (right_door_.home_pin.read() == 1) {
    step_controller.moveAsync(right_door_.stepper);
  }
}
void DoorController::closeRight() {
  right_door_.stepper.setMaxSpeed(config::kDoorMaxLiveSpeed);

  right_door_.stepper.setAcceleration(config::kDoorMaxLiveAccel);

  right_door_.stepper.setTargetAbs(right_door_.max_travel);

  if (right_door_.overrun_pin.read() == 1) {
    step_controller.moveAsync(right_door_.stepper);
  }
}
void DoorController::homeRight() {
  right_door_.stepper.setAcceleration(config::kDoorHomingAccel);

  // open first
  if (!right_door_.home_pin.read()) {
    right_door_.stepper.setMaxSpeed(-config::kDoorHomingSpeed);
    rotate_controller.rotateAsync(right_door_.stepper);
    while (!right_door_.home_pin.read())
      ;
    rotate_controller.stop();
  }

  // reset the open position to 0
  right_door_.stepper.setPosition(0);
  right_door_.stepper.setMaxSpeed(config::kDoorHomingSpeed);

  if (!right_door_.overrun_pin.read()) {
    right_door_.stepper.setMaxSpeed(config::kDoorHomingSpeed);
    rotate_controller.rotateAsync(right_door_.stepper);
    while (!right_door_.overrun_pin.read())
      ;
    rotate_controller.stop();
  }

  // motor is now is now in the closed position
  right_door_.max_travel = right_door_.stepper.getPosition();

  right_door_.stepper.setMaxSpeed(config::kDoorMaxLiveSpeed);
  right_door_.stepper.setAcceleration(config::kDoorMaxLiveAccel);
}
