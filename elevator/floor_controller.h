/** Define an interface for the floor movement controller
 *
 */

#pragma once

#ifndef HI_PROOF_FLOOR_CONTROLLER_H_
#define HI_PROOF_FLOOR_CONTROLLER_H_

#include "Bounce2.h"
#include "TeensyStep.h"
#include "elevator_config.h"
#include "safety_critical_component.h"

namespace hiproof {

namespace elevator {

class FloorController final : SafetyCriticalComponent {
 public:
  static constexpr uint8_t kNumNamedStops = 4;

  FloorController(const int step_pin, const int dir_pin, const int home_pin, const int overrun_pin);
  ~FloorController();
  void seekToHome();
  void moveToStop(uint8_t stop_number);
  void update();

  // API for "maintenance mode" tasks
  void setStop(uint8_t stop_number);
  void rotate(bool clockwise);
  void stopRotation();
  // void rotateWithStep(uint16_t steps, bool clockwise);

  void enterSafeMode();
  void emergencyStop();

 private:
  static constexpr uint16_t kHomingSpeed = 1000;
  static constexpr uint16_t kHomingAccel = 1000;
  static constexpr uint16_t kMaxLiveSpeed = 1500;
  static constexpr uint16_t kMaxLiveAccel = 500;
  static constexpr uint16_t kDebounceInterval = 25;

  uint32_t named_stops_[kNumNamedStops];
  Stepper stepper_;
  Bounce home_pin_ ;
  Bounce overrun_pin_;
  uint16_t run_speed{kMaxLiveSpeed};
  uint16_t run_accel{kMaxLiveAccel};

  static StepControl step_controller;
  static RotateControl rotate_controller;
};

}  // elevator

}  // hiproof

#endif  // HI_PROOF_FLOOR_CONTROLLER_H_
