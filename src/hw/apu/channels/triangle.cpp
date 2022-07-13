#include <nesemu/hw/apu/channels.h>

#include <nesemu/hw/apu/apu_clock.h>

#include <cstdint>


void hw::apu::channel::Triangle::writeReg(uint8_t reg, uint8_t data) {
  switch (reg & 0x03) {
    case 0x00:
      linear_counter_.reload_value_ = (data & 0x7F);
      linear_counter_.control_      = (data & 0x80);
      length_counter_.halt_         = (data & 0x80);
      break;
    case 0x01:
      // Unused
      break;
    case 0x02:
      period_ &= 0xFF00;
      period_ |= data;
      break;
    case 0x03:
      period_ &= 0x00FF;
      period_ |= (data & 0x7) << 8;
      if (enabled_) {
        length_counter_.load(data >> 3);
      }
      linear_counter_.reload_ = true;
      break;
  }
}


void hw::apu::channel::Triangle::clockCPU() {
  if (timer_.clock() && linear_counter_.getOutput(true) && length_counter_.getOutput(true)) {
    sequencer_.clock();
  }
}


void hw::apu::channel::Triangle::clockFrame(APUClock clock_type) {
  switch (clock_type) {
    case APUClock::QUARTER_FRAME:
      linear_counter_.clock();
      break;
    case APUClock::HALF_FRAME:
      length_counter_.clock();
      break;
      // No default
  }
}
