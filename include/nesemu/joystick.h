#pragma once

#include <nesemu/utils.h>

#include <cstdint>


namespace joystick {

class Joystick {
private:
  // Registers
  union {                           // Joystick, mapped to CPU 0x4016 or 0x4017 for Joy1 and Joy2, respectively (RW)
    uint8_t             raw;        //
    utils::RegBit<0>    data;       //   - Joystick data, strobed. Sequence: A, B, SELECT, START, UP, DOWN, LEFT, RIGHT
    utils::RegBit<1>    connected;  //   - 0=Connected
    utils::RegBit<2, 6> unknown;    //   - Unknown, must be constant 0b000010
  } register_ = {0};                //
};

}  // namespace joystick
