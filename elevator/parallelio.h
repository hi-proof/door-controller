#pragma once
#include "Bounce2.h"

//--------------------------------------------------------------------------------------
// Shift register based inputs & outputs
// Extend bounce library to support parallel to serial shift register

class ParallelInputs {
    uint8_t pin_data, pin_clock, pin_latch;
    uint8_t values;

  public:
    ParallelInputs(uint8_t pin_data, uint8_t pin_clock, uint8_t pin_latch) {
      this->pin_data = pin_data;
      this->pin_clock = pin_clock;
      this->pin_latch = pin_latch;
      this->values = 0;
    }

    void update() {
      digitalWrite(this->pin_latch, HIGH);
      delayMicroseconds(20);
      digitalWrite(this->pin_latch, LOW);
      this->values = shiftIn(pin_data, pin_clock, MSBFIRST);
    }

    bool digitalRead(uint8_t pin) {
      return (this->values & (1 << pin));
    }
};

class ParallelOutputs {
    uint8_t pin_data, pin_clock, pin_latch;
    uint8_t values;

  public:
    ParallelOutputs(uint8_t pin_data, uint8_t pin_clock, uint8_t pin_latch) {
      this->pin_data = pin_data;
      this->pin_clock = pin_clock;
      this->pin_latch = pin_latch;
      this->values = 0;
    }

    void update() {
      digitalWrite(this->pin_latch, LOW);
      shiftOut(this->pin_data, this->pin_clock, MSBFIRST, this->values);
      digitalWrite(this->pin_latch, HIGH);
    }

    void digitalWrite(uint8_t pin, uint8_t val) {
      bitWrite(this->values, pin, val != 0);
    }
};

class ParallelBounce : public Bounce {
    ParallelInputs& pi;

  public:
    ParallelBounce(ParallelInputs& pi, uint8_t pin) : pi(pi)
    {
      this->attach(pin);
    }

    virtual bool readCurrentState() {
      return this->pi.digitalRead(this->pin);
    }

    virtual void setPinMode(int pin, int mode) {
      // no-op
      return;
    }
};

//--------------------------------------------------------------------------------------
