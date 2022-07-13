#include <nesemu/hw/apu/channels.h>

#include <nesemu/hw/apu/apu_clock.h>

#include <cstdint>


void hw::apu::channel::Noise::writeReg(uint8_t reg, uint8_t data) {
  switch (reg & 0x03) {
    case 0x00:
      envelope_.divider_.setPeriod(data & 0x0F);
      envelope_.volume_       = (data & 0x0F);
      envelope_.const_volume_ = (data & 0x10);
      length_counter_.halt_   = (data & 0x20);
      envelope_.loop_         = (data & 0x20);
      break;
    case 0x01:
      // Unused
      break;
    case 0x02:
      timer_.setPeriod(PERIODS[data & 0x0F]);
      mode_ = (data & 0x80);
      break;
    case 0x03:
      envelope_.start_ = true;
      if (enabled_) {
        length_counter_.load(data >> 3);
      }
      break;
  }
}


void hw::apu::channel::Noise::clockCPU() {
  if (timer_.clock()) {
    const bool feedback = (lfsr_ & 0x01) ^ ((mode_ ? (lfsr_ >> 6) : (lfsr_ >> 1)) & 0x01);
    lfsr_ >>= 1;
    lfsr_ |= (feedback << 14);
  }
}


void hw::apu::channel::Noise::clockFrame(APUClock clock_type) {
  switch (clock_type) {
    case APUClock::QUARTER_FRAME:
      envelope_.clock();
      break;
    case APUClock::HALF_FRAME:
      length_counter_.clock();
      break;
      // No default
  }
}


uint8_t hw::apu::channel::Noise::getOutput() {
  return (lfsr_ & 0x01) == 0 ? 0 : length_counter_.getOutput(envelope_.getOutput());
}
