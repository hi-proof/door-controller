/** Implementation details for door controller
 *
 */

#include "door_controller.h"

using namespace hiproof::elevator;

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
  left_door_.home_pin.interval(kDebounceInterval);
  left_door_.overrun_pin.attach(d1_overrun_pin, INPUT_PULLUP);
  left_door_.overrun_pin.interval(kDebounceInterval);
  right_door_.home_pin.attach(d2_home_pin, INPUT_PULLUP);
  right_door_.home_pin.interval(kDebounceInterval);
  right_door_.overrun_pin.attach(d2_overrun_pin, INPUT_PULLUP);
  right_door_.overrun_pin.interval(kDebounceInterval);
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
