#include <nesemu/hw/apu/channels.h>

#include <nesemu/hw/apu/apu_clock.h>

#include <cstdint>


void hw::apu::channel::Square::writeReg(uint8_t reg, uint8_t data) {
  switch (reg & 0x03) {
    case 0x00:
      envelope_.divider_.setPeriod(data & 0x0F);
      envelope_.volume_       = (data & 0x0F);
      envelope_.const_volume_ = (data & 0x10);
      length_counter_.halt_   = (data & 0x20);
      envelope_.loop_         = (data & 0x20);
      duty_cycle_             = (data & 0xC0) >> 6;
      break;
    case 0x01:
      sweep_.shift_count_ = (data & 0x07);
      sweep_.negate_      = (data & 0x08);
      sweep_.divider_.setPeriod((data & 0x70) >> 4);
      sweep_.enable_ = (data & 0x80);
      sweep_.reload_ = true;
      break;
    case 0x02:
      period_ &= 0xFF00;
      period_ |= data;
      break;
    case 0x03:
      period_ &= 0x00FF;
      period_ |= (data & 0x7) << 8;
      envelope_.start_ = true;
      if (enabled_) {
        length_counter_.load(data >> 3);
      }
      sequencer_.reset();
      break;
  }
}


void hw::apu::channel::Square::clockCPU() {
  // Square wave is clocked every other CPU clock
  clock_is_even_ = !clock_is_even_;
  if (clock_is_even_ && timer_.clock()) {
    sequencer_.clock();
  }
}


void hw::apu::channel::Square::clockFrame(APUClock clock_type) {
  switch (clock_type) {
    case APUClock::QUARTER_FRAME:
      envelope_.clock();
      break;
    case APUClock::HALF_FRAME:
      length_counter_.clock();
      sweep_.clock();
      break;
      // No default
  }
}


uint8_t hw::apu::channel::Square::getOutput() {
  bool sequencer = SEQUENCE[duty_cycle_] & (1 << (8 - sequencer_.get()));
  return length_counter_.getOutput(sequencer ? sweep_.getOutput(envelope_.getOutput()) : 0);
}
