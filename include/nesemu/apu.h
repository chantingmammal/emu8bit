#pragma once

#include <nesemu/utils.h>

#include <cstdint>


namespace apu {

class APU {
public:
  // Execution
  void    clock();
  bool    hasIRQ() const { return has_irq_; }
  uint8_t readRegister(uint16_t address);
  void    writeRegister(uint16_t address, uint8_t data);

  bool    square_1_counter_halt_ = {false};
  uint8_t square_1_counter_      = {0};

private:
  bool has_irq_ = {false};

  // Registers
  uint8_t sound_[14] = {0};  // Sound Registers, mapped to CPU 0x4000 - 0x4013
  union {                    // Sound Channel Switch, mapped to CPU 0x4015 (W)
    uint8_t          raw;    //
    utils::RegBit<0> ch_1;   //  - Enable channel 1
    utils::RegBit<1> ch_2;   //  - Enable channel 2
    utils::RegBit<2> ch_3;   //  - Enable channel 3
    utils::RegBit<3> ch_4;   //  - Enable channel 4
    utils::RegBit<4> ch_5;   //  - Enable channel 5
                             //  - Unused
  } sound_en_ = {0};         //
  union {                    // Status, mapped to CPU 0x4015 (R)
    uint8_t          raw;    //
    utils::RegBit<0> ch_1_cnt;
    utils::RegBit<1> ch_2_cnt;
    utils::RegBit<2> ch_3_cnt;
    utils::RegBit<3> ch_4_cnt;
    utils::RegBit<4> dmc_active;
    utils::RegBit<6> frame_interrupt;
    utils::RegBit<7> dmc_interrupt;
  } status_ = {0};

  // Frame counter, mapped to CPU 0x4017 (W)
  bool     irq_inhibit_               = {false};   // If 0, inhibit IRQ generation
  bool     frame_counter_mode_        = {false};   // 0=4-step, 1=5-step
  uint16_t cycle_counter_reset_latch_ = {0xFFFF};  // Clock to trigger next frame counter reset, or 0xFFFF
  uint16_t cycle_count_               = {0};
};

}  // namespace apu
