#pragma once

#include <nesemu/utils.h>

#include <cstdint>


namespace joystick {

class Joystick {
private:
  template <unsigned bit_pos, unsigned n_bits = 1, typename T = uint8_t>
  using RegBit = typename utils::RegBit<bit_pos, n_bits, T>;

  // Registers
  union {                    // Joystick + Strobe, mapped to CPU 0x4016 or 0x4017 for Joy1 and Joy2, respectively (RW)
    uint8_t      raw;        //
    RegBit<0>    data;       //   - Joystick data, strobed. Sequence: A, B, SELECT, START, UP, DOWN, LEFT, RIGHT
    RegBit<1>    connected;  //   - 0=Connected
    RegBit<2, 6> unknown;    //   - Unknown, must be constant 0b000010
  } register_ = {0};         //
};

}  // namespace joystick
