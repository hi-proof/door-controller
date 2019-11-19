/** Define an interface for some basic safety sensitive devices
 * 
 * This isn't intended to be anything more than enforcing that controllers
 * with steppers can be stopped in the same way.
 * 
 */

#pragma once

#ifndef HI_PROOF_SAFETY_CRITICAL_COMPONENT_H_
#define HI_PROOF_SAFETY_CRITICAL_COMPONENT_H_

namespace hiproof {

namespace elevator {

class SafetyCriticalComponent {
 public:
  virtual ~SafetyCriticalComponent() {}
  virtual void enterSafeMode() = 0;
  virtual void emergencyStop() = 0;

  bool isSafe() { return safe_; }

 protected:
  bool safe_;
};

}  // elevator

}  // hiproof

#endif  // HI_PROOF_FLOOR_CONTROLLER_H_
