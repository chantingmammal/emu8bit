#include <nesemu/apu.h>


#include <nesemu/logger.h>


uint8_t lookup(uint8_t val) {
  switch (val) {
    case 0x01:
      return 254;
    case 0x03:
      return 2;
    case 0x05:
      return 4;
    case 0x07:
      return 6;
    case 0x09:
      return 8;
    case 0x0B:
      return 10;
    case 0x0D:
      return 12;
    case 0x0F:
      return 14;
    case 0x11:
      return 16;
    case 0x13:
      return 18;
    case 0x15:
      return 20;
    case 0x17:
      return 22;
    case 0x19:
      return 24;
    case 0x1B:
      return 26;
    case 0x1D:
      return 28;
    case 0x1F:
      return 30;

    case 0x00:
      return 10;
    case 0x02:
      return 20;
    case 0x04:
      return 40;
    case 0x06:
      return 80;
    case 0x08:
      return 160;
    case 0x0A:
      return 60;
    case 0x0C:
      return 14;
    case 0x0E:
      return 26;
    case 0x10:
      return 12;
    case 0x12:
      return 24;
    case 0x14:
      return 48;
    case 0x16:
      return 96;
    case 0x18:
      return 192;
    case 0x1A:
      return 72;
    case 0x1C:
      return 16;
    case 0x1E:
      return 32;
  }
}


void clockLengthCounter(apu::APU* apu) {
  if (!apu->square_1_counter_halt_ && apu->square_1_counter_ != 0) {
    apu->square_1_counter_--;
  }
}


void apu::APU::clock() {

  if (cycle_counter_reset_latch_ == cycle_count_) {
    cycle_count_ = 0;

    // If 5-step mode, clock everything
    if (frame_counter_mode_) {
      clockLengthCounter(this);
    }
  }


  switch (cycle_count_++) {
    case 7457:  // Clock envelope and linear counter
      break;

    case 14913:  // Clock length counter and sweep
                 // Clock envelope and linear counter
      clockLengthCounter(this);
      break;

    case 22371:  // Clock envelope and linear counter
      break;

    case 29828:  // (4-step only) Set frame interrupt flag if inhibit is clear
      if (!frame_counter_mode_ && !irq_inhibit_) {
        has_irq_ = true;
      }
      break;

    case 29829:  // (4-step only) Clock length counter and sweep
                 //               Clock envelope and linear counter
                 //               Set frame interrupt flag if inhibit is clear
      if (!frame_counter_mode_) {
        clockLengthCounter(this);
        if (!irq_inhibit_) {
          has_irq_ = true;
        }
      }
      break;

    case 29830:  // (4-step only) Set frame interrupt flag if inhibit is clear
                 //               Reset counter
      if (!frame_counter_mode_) {
        if (!irq_inhibit_) {
          has_irq_ = true;
        }
        cycle_count_ = 0;
      }
      break;

    case 37281:  // (5-step only) Clock length counter and sweep
                 //               Clock envelope and linear counter
                 //               Set frame interrupt flag if inhibit is clear
      clockLengthCounter(this);
      break;

    case 37282:  // (5-step only) Reset counter
      cycle_count_ = 0;
      break;
  }
}


uint8_t apu::APU::readRegister(uint16_t address) {
  switch (address) {
    case 0x4000:
    case 0x4001:
    case 0x4002:
    case 0x4003:
      return 0;  // TODO: Square 1

    case 0x4004:
    case 0x4005:
    case 0x4006:
    case 0x4007:
      // TODO: Square 2
      return 0;

    case 0x4008:
    case 0x4009:
    case 0x400A:
    case 0x400B:
      // TODO: Triangle
      return 0;

    case 0x400C:
    case 0x400D:
    case 0x400E:
    case 0x400F:
      // TODO: Noise
      return 0;

    case 0x4010:
    case 0x4011:
    case 0x4012:
    case 0x4013:
      // TODO: DMC
      return 0;

    case 0x4015:
      status_.ch_1_cnt        = square_1_counter_ > 0;
      status_.frame_interrupt = has_irq_;
      has_irq_                = false;
      logger::log<logger::DEBUG_APU>("Read $%02X from Status (0x4015)\n", status_.raw);
      return status_.raw;

    case 0x4017:  // Frame counter: Write only - Read should be redirected to joystick
      logger::log<logger::ERROR>("Internal error, 0x4017 WRITE should never occur on APU\n");
      return 0;

    default:
      logger::log<logger::ERROR>("Internal error, APU read register from unknown address $%04X\n", address);
      return 0;
  }
}

void apu::APU::writeRegister(uint16_t address, uint8_t data) {
  switch (address) {

    // TODO: Square 1
    case 0x4000:
      square_1_counter_halt_ = data & 0x20;
      logger::log<logger::DEBUG_APU>("Write $%02X to Square 1 (0x4000)\n", data);
      break;
    case 0x4001:
    case 0x4002:
      break;
    case 0x4003:
      if (sound_en_.ch_1) {
        square_1_counter_ = lookup(data >> 3);
      }
      logger::log<logger::DEBUG_APU>("Write $%02X to Square 1 (0x4003)\n", data);
      break;

    case 0x4004:
    case 0x4005:
    case 0x4006:
    case 0x4007:
      // TODO: Square 2
      break;

    case 0x4008:
    case 0x4009:
    case 0x400A:
    case 0x400B:
      // TODO: Triangle
      break;

    case 0x400C:
    case 0x400D:
    case 0x400E:
    case 0x400F:
      // TODO: Noise
      break;

    case 0x4010:
    case 0x4011:
    case 0x4012:
    case 0x4013:
      // TODO: DMC
      break;

    case 0x4015:
      sound_en_.raw = data;
      if (!sound_en_.ch_1)
        square_1_counter_ = 0;

      has_irq_ = false;
      logger::log<logger::DEBUG_APU>("Write $%02X to Status (0x4015)\n", data);
      break;

    case 0x4017:  // Frame counter
      irq_inhibit_        = data & 0x40;
      frame_counter_mode_ = data & 0x80;

      if (cycle_count_ % 2) {  // If odd:
        cycle_counter_reset_latch_ = cycle_count_ + 2;
      } else {
        cycle_counter_reset_latch_ = cycle_count_ + 3;
      }
      break;
  }
}