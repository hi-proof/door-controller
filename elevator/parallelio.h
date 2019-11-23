#pragma once
#include "Bounce2.h"
#include "state.h"
#include "pins.h"

//--------------------------------------------------------------------------------------
// Shift register based inputs & outputs
// Extend bounce library to support parallel to serial shift register

class ParallelInputs {
  public:
    uint8_t pin_data, pin_clock, pin_latch;
    uint8_t values;

  public:
    ParallelInputs(uint8_t pin_data, uint8_t pin_clock, uint8_t pin_latch) {
      this->pin_data = pin_data;
      this->pin_clock = pin_clock;
      this->pin_latch = pin_latch;

      pinMode(pin_data, INPUT);
      pinMode(pin_clock, OUTPUT);
      pinMode(pin_latch, OUTPUT);
      
      this->values = 0;
    }

    void update() {
      digitalWrite(this->pin_latch, LOW);
      delay(10);
      digitalWrite(this->pin_latch, HIGH);
      delay(1);
      this->values = shiftIn(pin_data, pin_clock, MSBFIRST);
      //shell_printf("%X\r\n", values);
    }

    bool read(uint8_t pin) {
      return (this->values & (1 << pin));
    }
};

class ParallelOutputs {
  public:
    uint8_t pin_data, pin_clock, pin_latch;
    uint8_t values;

  public:
    ParallelOutputs(uint8_t pin_data, uint8_t pin_clock, uint8_t pin_latch) {
      this->pin_data = pin_data;
      this->pin_clock = pin_clock;
      this->pin_latch = pin_latch;

      pinMode(pin_data, OUTPUT);
      pinMode(pin_clock, OUTPUT);
      pinMode(pin_latch, OUTPUT);

      this->values = 0;
    }

    void update() {
      digitalWrite(this->pin_latch, LOW);
      shiftOut(this->pin_data, this->pin_clock, MSBFIRST, this->values & 0xFF);
      digitalWrite(this->pin_latch, HIGH);
    }

    void write(uint8_t pin, uint8_t val) {
      bitWrite(this->values, pin, val != 0);
    }
};

const uint8_t SEG_A =  0b01000000;
const uint8_t SEG_B =  0b10000000;
const uint8_t SEG_C =  0b00000001;
const uint8_t SEG_D =  0b00000010;
const uint8_t SEG_E =  0b00000100;
const uint8_t SEG_F =  0b00010000;
const uint8_t SEG_G =  0b00001000;
const uint8_t SEG_DP = 0b00100000;

class SSeg {
  public:
    uint8_t pin_data, pin_clock, pin_latch;
    uint8_t values[2];

  public:
    SSeg(uint8_t pin_data, uint8_t pin_clock, uint8_t pin_latch) {
      this->pin_data = pin_data;
      this->pin_clock = pin_clock;
      this->pin_latch = pin_latch;

      pinMode(pin_data, OUTPUT);
      pinMode(pin_clock, OUTPUT);
      pinMode(pin_latch, OUTPUT);

      this->values[0] = 0;
      this->values[1] = 0;
    }

    void update() {
      digitalWrite(this->pin_latch, LOW);
      shiftOut(this->pin_data, this->pin_clock, MSBFIRST, this->values[0]);
      shiftOut(this->pin_data, this->pin_clock, MSBFIRST, this->values[1]);
      digitalWrite(this->pin_latch, HIGH);
    }

    void set(uint8_t l, uint8_t r) {
      this->values[0] = l;
      this->values[1] = r;
    }

    static uint8_t digit(uint8_t number) {
      switch (number) {
        case 0: return SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F;
        case 1: return SEG_B | SEG_C;
        case 2: return SEG_A | SEG_B | SEG_G | SEG_E | SEG_D;
        case 3: return SEG_A | SEG_B | SEG_C | SEG_D | SEG_G;
        case 4: return SEG_F | SEG_G | SEG_B | SEG_C;
        case 5: return SEG_A | SEG_F | SEG_G | SEG_C | SEG_D;
        case 6: return SEG_A | SEG_F | SEG_G | SEG_E | SEG_D | SEG_C;
        case 7: return SEG_A | SEG_B | SEG_C;
        case 8: return SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G;
        case 9: return SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G;
        default: return SEG_DP;
      }
    }

    static uint8_t character(char c) {
    switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        return SSeg::digit(c - '0');
      case 'A':
      case 'a':
        return SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G;
      case 'B':
      case 'b':
        return SEG_C | SEG_D | SEG_E | SEG_F | SEG_G;
      case 'C':
      case 'c':
        return SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G;
      case 'D':
      case 'd':
        return SEG_B | SEG_B | SEG_D | SEG_E | SEG_G;
      case 'E':
      case 'e':
        return SEG_A | SEG_F | SEG_G | SEG_E | SEG_D;
      case 'F':
      case 'f':
        return SEG_A | SEG_F | SEG_G | SEG_E;
      case 'L':
        return SEG_F | SEG_E | SEG_D;
      case 'H':
        return SEG_B | SEG_C | SEG_E | SEG_F | SEG_G;
      case '_':
        return SEG_D;
      case '-':
        return SEG_G;
      case '>':
        return SEG_G | SEG_B | SEG_C;
      case '<':
        return SEG_G | SEG_F | SEG_E;
      default:
        return SEG_DP;
    }
  }
};

class ParallelBounce : public Bounce {
    ParallelInputs& pi;

  public:
    ParallelBounce(ParallelInputs& pi, uint8_t pin) : pi(pi)
    {
      this->attach(pin);
    }

    ParallelBounce(const ParallelBounce& p2) 
    : pi(p2.pi)
    {
      this->attach(p2.pin);
    }

    virtual bool readCurrentState() {
      return this->pi.read(this->pin);
      return 0;
    }

    virtual void setPinMode(int pin, int mode) {
      // no-op
      return;
    }
};

class OutputPinBase
{
  public:
    uint8_t pin;
    OutputPinBase(uint8_t pin) {
      this->pin = pin;
      setPinMode(pin, OUTPUT);
    }

    virtual void write(int8_t value) {
      digitalWrite(pin, value);
    }

  protected:
    virtual void setPinMode(int pin, int mode) {
      pinMode(pin, mode);
    }

    
};

class ParallelOutputPin : public OutputPinBase {
  
  public:
    ParallelOutputs& po;
  
    ParallelOutputPin(ParallelOutputs& po, uint8_t pin) : po(po), OutputPinBase(pin)
    {
    }

    ParallelOutputPin(const ParallelOutputPin &p2)
    : po(p2.po), OutputPinBase(p2.pin)
    {
    }

    virtual void setPinMode(int pin, int mode) 
    {
      // no-op
      return;
    }

    virtual void write(int8_t value) {
      po.write(pin, value);
    }
};

//--------------------------------------------------------------------------------------

class FancyButton
{ 
  public:
    ParallelBounce input;
    ParallelOutputPin out;
    uint8_t button_id;
    
    bool held_called;
    bool on;
    bool on_on_press;

    bool clicked;
    bool held;
    bool pressed;
    bool released;
   
    FancyButton(ParallelBounce input, ParallelOutputPin out, uint8_t button_id) 
    : input(input), out(out), button_id(button_id), on_on_press(true), held_called(true),
      clicked(false), on(false), held(false)
    {
    }

    void update_inputs(void (*on_clicked)() = NULL, void (*on_held)() = NULL) {
      held = false;
      clicked = false;
      pressed = input.fell();
      released = input.rose();

      input.update();
      if (input.rose() && !held_called) {
        shell_printf("Button %d clicked\r\n", button_id);
        clicked = true;
        if(on_clicked) {
          on_clicked();
        }
      }
      if (input.read() == LOW && !held_called && input.duration() >= 2000) {
        shell_printf("Button %d held\r\n", button_id);
        held = true;
        held_called = true;
        if (on_held) {
         on_held();
        }
      }
      
      if (input.rose()) {
        held_called = false;
      }
    }

    void update_outputs()
    {
      bool is_on = on || (on_on_press && input.read() == LOW);
      out.write(is_on);      
    }
};

//--------------------------------------------------------------------------------------

class IOPanel
{
public:

  FancyButton b1;
  FancyButton b2;
  FancyButton b3;
  FancyButton b4;
  FancyButton b5;
  FancyButton b6;
  SSeg& sseg;

//  FancyButton& button_star;
//  FancyButton& button_13f;
//  FancyButton& button_14f;
//  FancyButton& button_bell;
//  FancyButton& button_close;
//  FancyButton& button_open;
//  FancyButton& button_call;

  IOPanel(ParallelInputs& buttons, ParallelOutputs& button_leds, SSeg& sseg)
    : b1(ParallelBounce(buttons, 0), ParallelOutputPin(button_leds, 3), BTN_STAR),
      b2(ParallelBounce(buttons, 1), ParallelOutputPin(button_leds, 2), BTN_13),
      b3(ParallelBounce(buttons, 2), ParallelOutputPin(button_leds, 1), BTN_14),
      b4(ParallelBounce(buttons, 3), ParallelOutputPin(button_leds, 0), BTN_BELL),
      b5(ParallelBounce(buttons, 7), ParallelOutputPin(button_leds, 4), BTN_CLOSE),
      b6(ParallelBounce(buttons, 6), ParallelOutputPin(button_leds, 5), BTN_OPEN),
      sseg(sseg)
  {
  }

  void update_inputs()
  {
    b1.update_inputs();
    b2.update_inputs();
    b3.update_inputs();
    b4.update_inputs();
    b5.update_inputs();
    b6.update_inputs();
  } 

  void update_outputs()
  {
    b1.update_outputs();
    b2.update_outputs();
    b3.update_outputs();
    b4.update_outputs();
    b5.update_outputs();
    b6.update_outputs();    
  }

  uint8_t buttons_state()
  {
    uint8_t ret = 
      ((b1.input.read() == LOW) << 0) |
      ((b2.input.read() == LOW) << 1) |
      ((b3.input.read() == LOW) << 2) |
      ((b4.input.read() == LOW) << 3) |
      ((b5.input.read() == LOW) << 4) |
      ((b6.input.read() == LOW) << 5);
    return ret;
  }
};

ParallelInputs buttons(SW_DATA, SW_CLOCK, SW_LATCH);

ParallelOutputs button_leds(BL_DATA, BL_CLOCK, BL_LATCH);

SSeg sseg(SSEG_DATA, SSEG_CLOCK, SSEG_LATCH);

IOPanel panel(buttons, button_leds, sseg);
