#pragma once

#include <TeensyStep.h>

const uint32_t HOMING_SPEED = 500;
const uint32_t HOMING_ACCEL = 500;

const uint32_t DOOR_SPEED = 5000;
const uint32_t DOOR_ACCEL = 1500;

const uint32_t FLOOR_SPEED = 1800;
const uint32_t FLOOR_ACCEL = 700;

enum {
  MOTOR_STOPPED,
  MOTOR_STOPPING,
  MOTOR_ROTATING,
  MOTOR_GOINGTO,  
};

class MotorAndSwitches
{
  public:

    uint8_t pin_dir, pin_pulse, pin_sw1, pin_sw2;

    int homing_speed;
    int homing_accel;

    int motor_speed;
    int motor_accel;

    // This is SUPER ghetto - we have 8 available timer channels and each control uses 2, so we're
    // limited to 4 controls system-wide. We'll have 3 StepControls for Door, Door, and Floor, and a
    // a shared RotateControl that will be used for calibration
    StepControl sc;
    RotateControl& rc;
    Stepper s;

    int32_t pending_pos;
    bool pending_goto;

    uint8_t stop_sw;

    MotorAndSwitches(uint8_t pin_pulse, uint8_t pin_dir, uint8_t pin_sw1, uint8_t pin_sw2, RotateControl& rc)
      : s(pin_pulse, pin_dir), sc(25), rc(rc),
        homing_speed(HOMING_SPEED), homing_accel(HOMING_ACCEL),
        pending_goto(false)
    {
      this->pin_dir = pin_dir;
      this->pin_pulse = pin_pulse;
      this->pin_sw1 = pin_sw1;
      this->pin_sw2 = pin_sw2;

      pinMode(this->pin_sw1, INPUT_PULLUP);
      pinMode(this->pin_sw2, INPUT_PULLUP);
    }

    bool sw1_hit()
    {
      return digitalRead(this->pin_sw1) == HIGH;
    }

    bool sw2_hit()
    {
      return digitalRead(this->pin_sw2) == HIGH;
    }

    // async
    void rotate(bool dir, int speed=0, int accel=0) 
    {
      if (speed == 0) {
        speed = motor_speed;
      }
      if (accel == 0) {
        accel = motor_accel;
      }
      
      stop(true);
      stop_sw = (dir) ? pin_sw2 : pin_sw1; 
      s.setMaxSpeed(dir ? speed : -speed);
      s.setAcceleration(accel);
      rc.rotateAsync(s);
    }

    void stop(bool sync=false)
    {
      if (sync) {
        rc.stop();
        sc.stop();
        return;
      }
        
      sc.stopAsync();
    }

    // async
    void goto_pos(int32_t position) 
    {
      stop_sw = (position < s.getPosition()) ? pin_sw1 : pin_sw2;
      s.setMaxSpeed(motor_speed);
      s.setAcceleration(motor_speed);
      s.setTargetAbs(position);
      sc.moveAsync(s);
    }

    // async
    void stop_and_goto_pos(int32_t position) {
      pending_goto = true;
      pending_pos = position;
      stop();
    }

    // sync
    void home()
    {
      if (!sw1_hit()) {
        rotate(false, homing_speed, homing_accel);
        while (!sw1_hit());
      }
      s.setPosition(0);
      stop(true);
    }

    // sync
    int32_t find_limit()
    {
      stop(true);
      if (sw2_hit()) {
        return s.getPosition();
      }
      rotate(true, homing_speed, homing_accel);
      while (!sw2_hit());
      int32_t ret = s.getPosition();
      stop(true);
      return ret;
    }

    void update()
    {
      if (!rc.isRunning() && pending_goto) {
        pending_goto = false;
        goto_pos(pending_pos);
      }
      if (rc.isRunning() || sc.isRunning()) {
        if (stop_sw == pin_sw1 && sw1_hit() ){
          s.setPosition(0);
          stop(true);
        }
        if (stop_sw == pin_sw2 && sw2_hit()) {
          stop(true);
        }
      }
    }
};

class Door : public MotorAndSwitches
{
  public:
    bool calibrated;
    int32_t closed_pos;
  
    Door(uint8_t pin_pulse, uint8_t pin_dir, uint8_t pin_sw1, uint8_t pin_sw2, RotateControl& rc)
      : MotorAndSwitches(pin_pulse, pin_dir, pin_sw1, pin_sw2, rc)
    {
      calibrated = false;
      closed_pos = 0;
      motor_speed = DOOR_SPEED;
      motor_accel = DOOR_ACCEL;
    }

    void calibrate()
    {
      home();
      closed_pos = find_limit();
      calibrated = true;
    }

    void open()
    {
      stop_and_goto_pos(0);
    }

    void close()
    {
      stop_and_goto_pos(closed_pos);
    }
};



class Floor : public MotorAndSwitches
{  
  public:
    bool calibrated;

    Floor(uint8_t pin_pulse, uint8_t pin_dir, uint8_t pin_sw1, uint8_t pin_sw2, RotateControl& rc)
      : MotorAndSwitches(pin_pulse, pin_dir, pin_sw1, pin_sw2, rc)
    {
      calibrated = false;
      motor_speed = FLOOR_SPEED;
      motor_accel = FLOOR_ACCEL;
    }

    // blocking
    void calibrate()
    {
      stop(true);
      home();
      calibrated = true;
    }

};
