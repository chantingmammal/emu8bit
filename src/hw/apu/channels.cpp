#include <nesemu/hw/apu/channels.h>

#include <cstdio>


// =*=*=*=*= Square =*=*=*=*=

void hw::apu::channel::Square::writeReg(uint8_t reg, uint8_t data) {
  switch (reg & 0x03) {
    case 0x00:
      envelope_.divider_.setPeriod(data & 0x0F);
      envelope_.volume_       = (data & 0x0F);
      envelope_.const_volume_ = (data & 0x10);
      length_counter.halt_    = (data & 0x20);
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
        length_counter.load(data >> 3);
      }
      resetSequencer();
      break;
  }
}

void hw::apu::channel::Square::clockCPU() {
  // Square wave is clocked every other CPU clock
  clock_is_even_ = !clock_is_even_;
  if (clock_is_even_ && timer_.clock()) {
    advanceSequencer();
  }
}

void hw::apu::channel::Square::clockFrame(APUClock clock_type) {
  switch (clock_type) {
    case APUClock::QUARTER_FRAME:
      envelope_.clock();
      break;
    case APUClock::HALF_FRAME:
      length_counter.clock();
      sweep_.clock();
      break;
      // No default
  }
}

uint8_t hw::apu::channel::Square::getOutput() {
  return length_counter.getOutput(getSequencerOutput() ? sweep_.getOutput(envelope_.getOutput()) : 0);
}


// =*=*=*=*= Triangle =*=*=*=*=

void hw::apu::channel::Triangle::clockCPU() {
  if (timer.clock() && linear_counter.getOutput(true) && length_counter.getOutput(true)) {
    advanceSequencer();
  }
}

void hw::apu::channel::Triangle::clockFrame(APUClock clock_type) {
  switch (clock_type) {
    case APUClock::QUARTER_FRAME:
      linear_counter.clock();
      break;
    case APUClock::HALF_FRAME:
      length_counter.clock();
      break;
      // No default
  }
}


// =*=*=*=*= Noise =*=*=*=*=

void hw::apu::channel::Noise::loadPeriod(uint8_t code) {
  switch (code) {
    case 0:
      timer.setPeriod(4);
      break;
    case 1:
      timer.setPeriod(8);
      break;
    case 2:
      timer.setPeriod(16);
      break;
    case 3:
      timer.setPeriod(32);
      break;
    case 4:
      timer.setPeriod(64);
      break;
    case 5:
      timer.setPeriod(96);
      break;
    case 6:
      timer.setPeriod(128);
      break;
    case 7:
      timer.setPeriod(160);
      break;
    case 8:
      timer.setPeriod(202);
      break;
    case 9:
      timer.setPeriod(254);
      break;
    case 10:
      timer.setPeriod(380);
      break;
    case 11:
      timer.setPeriod(508);
      break;
    case 12:
      timer.setPeriod(762);
      break;
    case 13:
      timer.setPeriod(1016);
      break;
    case 14:
      timer.setPeriod(2034);
      break;
    case 15:
      timer.setPeriod(4068);
      break;
    default:
      __builtin_unreachable();
  }
}

void hw::apu::channel::Noise::clockCPU() {
  if (timer.clock()) {
    const bool feedback = (lfsr & 0x01) ^ ((mode ? (lfsr >> 6) : (lfsr >> 1)) & 0x01);
    lfsr >>= 1;
    lfsr |= (feedback << 14);
  }
}

void hw::apu::channel::Noise::clockFrame(APUClock clock_type) {
  switch (clock_type) {
    case APUClock::QUARTER_FRAME:
      envelope.clock();
      break;
    case APUClock::HALF_FRAME:
      length_counter.clock();
      break;
      // No default
  }
}

uint8_t hw::apu::channel::Noise::getOutput() {
  return (lfsr & 0x01) == 0 ? 0 : length_counter.getOutput(envelope.getOutput());
}


// =*=*=*=*= Noise =*=*=*=*=

void hw::apu::channel::DMC::clockFrame(APUClock clock_type) {
  switch (clock_type) {
    case APUClock::QUARTER_FRAME:
      break;
    case APUClock::HALF_FRAME:
      length_counter.clock();
      break;
      // No default
  }
}
