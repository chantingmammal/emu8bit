#include <nesemu/hw/apu/channels.h>

#include <cstdio>

// =*=*=*=*= Square =*=*=*=*=

void hw::apu::channel::Square::clock() {
  if (cur_time_ == 0) {
    cur_time_ = timer;
    advanceSequencer();
  } else {
    cur_time_--;
  }
};

uint8_t hw::apu::channel::Square::getOutput() {
  return length_counter.getOutput(getSequencerOutput() ? sweep.getOutput(envelope.getVolume()) : 0);
};

// =*=*=*=*= Triangle =*=*=*=*=

void hw::apu::channel::Triangle::clock() {
  if (cur_time_ == 0) {
    cur_time_ = timer;
    if (linear_counter.counter_ != 0 && length_counter.counter_ != 0) {
      advanceSequencer();
    }
  } else {
    cur_time_--;
  }
};


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
  }
}


// Clocked at CPU freq / 2
void hw::apu::channel::Noise::clock() {

  // Clock the timer
  if (!timer.clock()) {
    return;
  }

  // When the timer clocks the shift register
  const bool feedback = (lfsr & 0x01) ^ ((mode ? (lfsr >> 6) : (lfsr >> 1)) & 0x01);
  lfsr >>= 1;
  lfsr |= (feedback << 14);
}

uint8_t hw::apu::channel::Noise::getOutput() {

  // When bit 0 is set, output 0
  if ((lfsr & 0x01) == 0) {
    return 0;
  }

  return length_counter.getOutput(envelope.getVolume());
}
