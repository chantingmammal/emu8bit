#pragma once

#include <nesemu/utils.h>

#include <cstdint>


namespace apu {

class APU {
private:
  template <unsigned bit_pos, unsigned n_bits = 1, typename T = uint8_t>
  using RegBit = typename utils::RegBit<bit_pos, n_bits, T>;

  // Registers
  uint8_t sound_[14] = {0};  // Sound Registers, mapped to CPU 0x4000 - 0x4013
  union {                    // Sound Channel Switch, mapped to CPU 0x4015 (W)
    uint8_t   raw;           //
    RegBit<1> ch_1;          //  - Enable channel 1
    RegBit<2> ch_2;          //  - Enable channel 2
    RegBit<3> ch_3;          //  - Enable channel 3
    RegBit<4> ch_4;          //  - Enable channel 4
    RegBit<5> ch_5;          //  - Enable channel 5
                             //  - Unused
  } sound_en_ = {0};         //
};

}  // namespace apu
