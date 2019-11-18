/** Define an interface for the floor movement controller
 *
 */

#pragma once

#ifndef HI_PROOF_FLOOR_CONTROLLER_H_
#define HI_PROOF_FLOOR_CONTROLLER_H_

#include <array>

#include "safety_critical_component.h"
#include "TeensyStep.h"

namespace hiproof {

namespace elevator {

class FloorController final : SafetyCriticalComponent {
 public:
  static constexpr uint8_t kNumNamedStops = 4;

  FloorController(const int step_pin, const int dir_pin);
  virtual ~FloorController();
  bool seekToHome();
  bool moveToStop(uint8_t stop_number);

  // API for "maintenence mode" tasks
  bool enterMaintainenceMode();
  bool setStop(uint8_t stop_number);
  bool rotate(bool clockwise);
  bool stopRotation();
  bool rotateWithStep(uint16_t steps, bool clockwise);
  bool exitMaintainenceMode();

  void enterSafeMode() = 0;
  void emergencyStop() = 0;

 private:
  static constexpr uint16_t kHomingSpeed = 1000;
  static constexpr uint16_t kHomingAccel = 1000;

  enum class MovementState { Stopped, Stepping, Rotating, Unknown };
  MovementState activity_state_{MovementState::Stopped};
  std::array<uint32_t, kNumNamedStops> named_stops_;
  Stepper stepper_;

  static StepControl step_controller;
  static RotateControl rotate_controller;
};

}  // elevator

}  // hiproof

#endif  // HI_PROOF_FLOOR_CONTROLLER_H_
