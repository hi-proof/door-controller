/** Configuration options and constants for the elevator
 * 
 */

#ifndef HI_PROOF_ELEVATOR_CONFIG_H_
#define HI_PROOF_ELEVATOR_CONFIG_H_

namespace hiproof {
namespace elevator {
namespace config {

using EthernetConnectorPins = struct {
  int step_pin;
  int dir_pin;
  int home_pin;
  int overrun_pin;
};

constexpr EthernetConnectorPins kEthPort1 = {
    .step_pin = 34, .dir_pin = 33, .home_pin = 35, .overrun_pin = 36};

constexpr EthernetConnectorPins kEthPort2 = {
    .step_pin = 34, .dir_pin = 33, .home_pin = 35, .overrun_pin = 36};

constexpr EthernetConnectorPins kEthPort3 = {
    .step_pin = 34, .dir_pin = 33, .home_pin = 35, .overrun_pin = 36};

constexpr EthernetConnectorPins kEthPort4 = {
    .step_pin = 34, .dir_pin = 33, .home_pin = 35, .overrun_pin = 36};

static constexpr uint16_t kFloorHomingSpeed = 1000;
static constexpr uint16_t kFloorHomingAccel = 1000;
static constexpr uint16_t kFloorMaxLiveSpeed = 1500;
static constexpr uint16_t kFloorMaxLiveAccel = 500;

static constexpr uint16_t kDoorHomingSpeed = 1000;
static constexpr uint16_t kDoorHomingAccel = 1000;
static constexpr uint16_t kDoorMaxLiveSpeed = 1500;
static constexpr uint16_t kDoorMaxLiveAccel = 500;

static constexpr uint16_t kPinDebounceInterval = 25;

}
}
} // namespace hiproof

#endif // HI_PROOF_ELEVATOR_CONFIG_H_
