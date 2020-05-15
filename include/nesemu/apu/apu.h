#pragma once

#include <nesemu/apu/channels.h>
#include <nesemu/utils.h>

#include <cstdint>


namespace apu {

namespace registers {
union StatusControl {
  uint8_t          raw;
  utils::RegBit<0> ch_1;             // (W) Enable CH1    (R) CH1 length counter > 0
  utils::RegBit<1> ch_2;             // (W) Enable CH2    (R) CH2 length counter > 0
  utils::RegBit<2> ch_3;             // (W) Enable CH3    (R) CH3 length counter > 0
  utils::RegBit<3> ch_4;             // (W) Enable CH4    (R) CH4 length counter > 0
  utils::RegBit<4> ch_5;             // (W) Enable CH5    (R) DMC is active
                                     // Unused
  utils::RegBit<6> frame_interrupt;  // (W) Unused        (R) Frame interrupt flag is set
  utils::RegBit<7> dmc_interrupt;    // (W) Unused        (R) DMC interrupt flag is set
};
}  // namespace registers


class APU {
public:
  // Execution
  void    clock();
  bool    hasIRQ() const { return has_irq_; }
  uint8_t readRegister(uint16_t address);
  void    writeRegister(uint16_t address, uint8_t data);

private:
  inline void clockHalfFrame();
  inline void clockQuarterFrame();


  bool has_irq_ = {false};

  channel::Square          square_1 = {1};   // CPU 0x4000 - 0x4003
  channel::Square          square_2 = {2};   // CPU 0x4004 - 0x4007
  channel::Triangle        triangle;         // CPU 0x4008 - 0x400B
  channel::Noise           noise;            // CPU 0x400C - 0x400F
  channel::DMC             dmc;              // CPU 0x4010 - 0x4013
  registers::StatusControl sound_en_ = {0};  // CPU 0x4015


  // Frame counter: CPU 0x4017
  bool     irq_inhibit_               = {false};   // If 0, inhibit IRQ generation
  bool     frame_counter_mode_        = {false};   // 0=4-step, 1=5-step
  uint16_t cycle_counter_reset_latch_ = {0xFFFF};  // Clock to trigger next frame counter reset, or 0xFFFF
  uint16_t cycle_count_               = {0};
};

}  // namespace apu
