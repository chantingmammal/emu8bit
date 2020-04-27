#pragma once

#include <nesemu/utils.h>

#include <cstdint>


namespace joystick {

class Joystick {
public:
  void    write(uint8_t data);
  uint8_t read();

private:
  // Registers
  union {                           // Joystick, mapped to CPU 0x4016 or 0x4017 for Joy1 and Joy2, respectively (RW)
    uint8_t             raw;        //
    utils::RegBit<0>    data;       //   - Joystick data, strobed. Sequence: A, B, SELECT, START, UP, DOWN, LEFT, RIGHT
    utils::RegBit<1>    connected;  //   - 0=Connected
    utils::RegBit<2, 6> unknown;    //   - Open bus, should be constant 0x40 to match bus address
  } register_         = {0x40};     //
  uint8_t strobe_pos_ = {0};

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

}  // namespace joystick
