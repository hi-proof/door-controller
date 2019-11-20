#pragma once

#include <TeensyStep.h>

const uint32_t HOMING_SPEED = 500;
const uint32_t HOMING_ACCEL = 1500;

const uint32_t DOOR_SPEED = 6000;
const uint32_t DOOR_ACCEL = 1800;

const uint32_t FLOOR_SPEED = 1500;
const uint32_t FLOOR_ACCEL = 700;

class MotorAndSwitches
{
  public:

    uint8_t pin_dir, pin_pulse, pin_sw1, pin_sw2;

    int motor_speed;
    int motor_accel;

    int32_t pos_open;
    int32_t pos_closed;

    Stepper s;

    MotorAndSwitches(uint8_t pin_pulse, uint8_t pin_dir, uint8_t pin_sw1, uint8_t pin_sw2)
      : s(pin_pulse, pin_dir)
    {
      this->pin_dir = pin_dir;
      this->pin_pulse = pin_pulse;
      this->pin_sw1 = pin_sw1;
      this->pin_sw2 = pin_sw2;

      pinMode(this->pin_sw1, INPUT_PULLUP);
      pinMode(this->pin_sw2, INPUT_PULLUP);
    }  
};

class Door : public MotorAndSwitches
{
  public:
    Door(uint8_t pin_pulse, uint8_t pin_dir, uint8_t pin_sw1, uint8_t pin_sw2)
      : MotorAndSwitches(pin_pulse, pin_dir, pin_sw1, pin_sw2)
    {
      this->motor_speed = DOOR_SPEED;
      this->motor_accel = DOOR_ACCEL;
    }

    void homing_cycle(RotateControl& rc) {
      this->s.setAcceleration(HOMING_ACCEL);

      // open first
      if (!digitalRead(this->pin_sw1)) {
        this->s.setMaxSpeed(-HOMING_SPEED);
        rc.rotateAsync(this->s);
        while (!digitalRead(this->pin_sw1));
        rc.stop();
      }

      // reset the open position to 0
      this->s.setPosition(0);
      
      // We can end the homing cycle here if we don't want to measure the close position
      // this->pos_closed = 18000;
      // return;

      this->s.setMaxSpeed(HOMING_SPEED);
      if (!digitalRead(this->pin_sw2)) {
        this->s.setMaxSpeed(HOMING_SPEED);
        rc.rotateAsync(this->s);
        while (!digitalRead(this->pin_sw2));
        rc.stop();
      }

      // motor is now is now in the closed position
      this->pos_closed = this->s.getPosition();

      this->s.setMaxSpeed(this->motor_speed);
      this->s.setAcceleration(this->motor_accel);
    }
};



class Floor : MotorAndSwitches
{
  public:

    Floor(uint8_t pin_pulse, uint8_t pin_dir, uint8_t pin_sw1, uint8_t pin_sw2)
      : MotorAndSwitches(pin_pulse, pin_dir, pin_sw1, pin_sw2)
    {
      this->motor_speed = FLOOR_SPEED;
      this->motor_accel = FLOOR_ACCEL;
    }

    void homing_cycle(RotateControl& rc) {
      this->s.setAcceleration(HOMING_ACCEL);

      // open door until both switches are triggered
      if (!digitalRead(this->pin_sw1) && !digitalRead(this->pin_sw2)) {
        this->s.setMaxSpeed(-HOMING_SPEED);
        rc.rotateAsync(this->s);

        // wait for the second switch to be triggered
        while (true) {
          while (!digitalRead(this->pin_sw1));
          // if both switches are triggered we're at the base position
          if (digitalRead(this->pin_sw2)) {
             break;
          }
        }
        rc.stop();
      }

      // we're back home - reset position
      this->s.setPosition(0);
    }

    void goto_position(StepControl& sc, int32_t pos) {
      s.setMaxSpeed(motor_speed);
      s.setAcceleration(motor_accel);
      s.setTargetAbs(pos);
      sc.moveAsync(s);
    }

    void go(RotateControl& rc)
    {
       rc.stop();
       s.setMaxSpeed(HOMING_SPEED);
       s.setAcceleration(HOMING_ACCEL);
       rc.rotateAsync(s);
    }

    void back(RotateControl& rc)
    {
       rc.stop();
       s.setMaxSpeed(-HOMING_SPEED);
       s.setAcceleration(HOMING_ACCEL);
       rc.rotateAsync(s);
    }
};
