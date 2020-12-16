#pragma once

#include <nesemu/utils/utils.h>

#include <cstdint>


namespace hw::joystick {

class Joystick {
public:
  Joystick(uint8_t port) : port_(port) {};

  void    write(uint8_t data);
  uint8_t read();

private:
  uint8_t port_;  // Controller port, 1 or 2 ($4016 or $4017)

  // Registers
  union {                            // Joystick, mapped to CPU 0x4016 or 0x4017 for Joy1 and Joy2, respectively (RW)
    uint8_t             raw;         //
    utils::RegBit<0>    standard;    //   - Standard controller, strobed: A, B, SELECT, START, UP, DOWN, LEFT, RIGHT
    utils::RegBit<1>    expansion;   //   - Famicom expansion port controller
    utils::RegBit<2>    microphone;  //   - Famicom microphone, controller 2 only
    utils::RegBit<3>    light;       //   - Zapper, light detected
    utils::RegBit<4>    trigger;     //   - Zapper, trigger pulled
    utils::RegBit<5, 3> unused;      //   - Open bus, should be constant 0x40 to match bus address
  } register_          = {0x40};
  uint8_t strobe_pos_  = {0};
  bool    prev_strobe_ = {false};

  union {
    uint8_t          raw;
    utils::RegBit<0> a;
    utils::RegBit<1> b;
    utils::RegBit<2> select;
    utils::RegBit<3> start;
    utils::RegBit<4> up;
    utils::RegBit<5> down;
    utils::RegBit<6> left;
    utils::RegBit<7> right;
  } state_ = {0};
};

}  // namespace hw::joystick
