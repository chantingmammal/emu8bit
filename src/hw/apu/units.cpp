#include <nesemu/hw/apu/units.h>


// =*=*=*=*= Envelope =*=*=*=*=

void hw::apu::unit::Envelope::clock() {
  if (!start_) {
    decay_level_ = 15;
    divider_.reload();
    return;
  }

  // Clock divider
  if (!divider_.clock()) {
    return;
  }

  // Clock decay level counter
  if (decay_level_ > 0) {
    decay_level_--;
  } else if (loop_) {
    decay_level_ = 15;
  }
}


// =*=*=*=*= Sweep =*=*=*=*=

void hw::apu::unit::Sweep::clock() {
  const uint16_t target_period = getTarget();

  if (divider_.clock() && enable_ && shift_count_ > 0 && target_period < 0x800 && *channel_period_ >= 8) {
    *channel_period_ = target_period;
  }

  if (reload_) {
    divider_.reload();
    reload_ = false;
  }
}

uint16_t hw::apu::unit::Sweep::getTarget() {
  return *channel_period_ + ((*channel_period_) >> shift_count_) * (negate_ ? -1 : 1) - (is_ch_2_ ? 0 : 1);
}

uint8_t hw::apu::unit::Sweep::getOutput(uint8_t input) {
  return (*channel_period_ < 8 || getTarget() > 0x7FF) ? 0 : input;
}


// =*=*=*=*= Length Counter =*=*=*=*=

void hw::apu::unit::LengthCounter::load(uint8_t code) {

  if (code % 2) {
    counter_ = (code == 1) ? 254 : code - 1;
  } else {
    // TODO: Make more elegant
    switch (code) {
      case 0x1E:
        counter_ = 32;
        break;
      case 0x1C:
        counter_ = 16;
        break;
      case 0x1A:
        counter_ = 72;
        break;
      case 0x18:
        counter_ = 192;
        break;
      case 0x16:
        counter_ = 96;
        break;
      case 0x14:
        counter_ = 48;
        break;
      case 0x12:
        counter_ = 24;
        break;
      case 0x10:
        counter_ = 12;
        break;

      case 0x0E:
        counter_ = 26;
        break;
      case 0x0C:
        counter_ = 14;
        break;
      case 0x0A:
        counter_ = 60;
        break;
      case 0x08:
        counter_ = 160;
        break;
      case 0x06:
        counter_ = 80;
        break;
      case 0x04:
        counter_ = 40;
        break;
      case 0x02:
        counter_ = 20;
        break;
      case 0x00:
        counter_ = 10;
        break;
    }
  }
}


void hw::apu::unit::LengthCounter::clock() {
  if (counter_ == 0 || halt_) {
    return;
  }
  counter_--;
}

uint8_t hw::apu::unit::LengthCounter::getOutput(uint8_t input) {
  return (counter_ == 0) ? 0 : input;
}


// =*=*=*=*= Linear Counter =*=*=*=*=

void hw::apu::unit::LinearCounter::clock() {
  if (reload_) {
    counter_ = reload_value_;
  } else if (counter_ > 0) {
    counter_--;
  }

  if (!control_) {
    reload_ = false;
  }
}