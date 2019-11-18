/** Define an interface for the floor movement controller
 *
 */

#pragma once

#ifndef HI_PROOF_FLOOR_CONTROLLER_H_
#define HI_PROOF_FLOOR_CONTROLLER_H_

#include <array>

#include "safety_critical_component.h"
#include "TeensyStep.h"
#include "Bounce2.h"

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

  // API for "maintenence mode" tasks
  void enterMaintainenceMode();
  void setStop(uint8_t stop_number);
  void rotate(bool clockwise);
  void stopRotation();
  void rotateWithStep(uint16_t steps, bool clockwise);
  bool inMaintenenceMode() { return in_maintenence_mode_; }
  void exitMaintainenceMode();

  void enterSafeMode() = 0;
  void emergencyStop() = 0;

 private:
  static constexpr uint16_t kHomingSpeed = 1000;
  static constexpr uint16_t kHomingAccel = 1000;
  static constexpr uint16_t kMaxLiveSpeed = 1500;
  static constexpr uint16_t kMaxLiveAccel = 500;
  static constexpr uint16_t kDebounceInterval = 25;

  bool in_maintenence_mode_{false};
  enum class MovementState { Stopped, Home, Stepping, Rotating, Overrun, Unknown };
  MovementState activity_state_{MovementState::Stopped};
  std::array<uint32_t, kNumNamedStops> named_stops_;
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
