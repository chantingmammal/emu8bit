#include <nesemu/hw/apu/apu.h>

#include <nesemu/hw/apu/apu_clock.h>
#include <nesemu/logger.h>
#include <nesemu/ui/speaker.h>


// =*=*=*=*= APU Setup =*=*=*=*=

void hw::apu::APU::setSpeaker(ui::Speaker* speaker) {
  speaker_ = speaker;
}


// =*=*=*=*= APU Execution =*=*=*=*=

void hw::apu::APU::clockFrame(APUClock clock_type) {
  for (auto&& channel : channels) {
    channel->clockFrame(clock_type);
  }
}

void hw::apu::APU::clock() {
  if (frame_counter_reset_counter_ == cycle_count_) {
    cycle_count_                 = 0;
    frame_counter_reset_counter_ = 0xFFFF;

    // If 5-step mode, clock everything
    if (frame_counter_mode_) {
      clockFrame(APUClock::HALF_FRAME);
      clockFrame(APUClock::QUARTER_FRAME);
    }
  }


  switch (cycle_count_++) {
    case 7457:
      clockFrame(APUClock::QUARTER_FRAME);
      break;

    case 14913:
      clockFrame(APUClock::HALF_FRAME);
      clockFrame(APUClock::QUARTER_FRAME);
      break;

    case 22371:
      clockFrame(APUClock::QUARTER_FRAME);
      break;

    case 29828:  // (4-step only) Set frame interrupt flag if inhibit is clear
      if (!frame_counter_mode_ && !irq_inhibit_) {
        has_irq_ = true;
      }
      break;

    case 29829:  // (4-step only) Set frame interrupt flag if inhibit is clear
      if (!frame_counter_mode_) {
        clockFrame(APUClock::HALF_FRAME);
        clockFrame(APUClock::QUARTER_FRAME);
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

    case 37281:  // (5-step only) Set frame interrupt flag if inhibit is clear
      clockFrame(APUClock::HALF_FRAME);
      clockFrame(APUClock::QUARTER_FRAME);
      break;

    case 37282:  // (5-step only) Reset counter
      cycle_count_ = 0;
      break;
  }

  for (auto&& channel : channels) {
    channel->clockCPU();
  }

  // TODO: Nicer mixer? Maybe a LUT?
  float square_sum = square_1.getOutput() + square_2.getOutput();
  float square_out = square_sum == 0 ? 0 : 95.88 / (8128 / square_sum + 100);
  float tnd_sum    = triangle.getOutput() / 8227.0 + noise.getOutput() / 12241.0 + dmc.getOutput() / 22638.0;
  float tnd_out    = tnd_sum == 0 ? 0 : 159.79 / (1 / tnd_sum + 100);

  uint8_t buffer[1] = {(square_out + tnd_out) * 255};
  speaker_->update(buffer, 1);
}


uint8_t hw::apu::APU::readRegister(uint16_t address) {
  if (address != 0x4015) {
    // Unknown register, or read-only register. Note that all registers except 0x4015 are write-only
    // To ensure open bus behaviour is maintained, this func should only be called with addr == 0x4015
    logger::log<logger::Level::WARNING>("Attempted to read a write-only APU register ($%02X)!\n", address);
    return 0;
  }

  // Status
  static registers::StatusControl status = {0};
  status.ch_1                            = square_1.length_counter.counter_ > 0;
  status.ch_2                            = square_2.length_counter.counter_ > 0;
  status.ch_3                            = triangle.length_counter.counter_ > 0;
  status.ch_4                            = noise.length_counter.counter_ > 0;
  status.ch_5                            = false;  // TODO
  status.frame_interrupt                 = has_irq_;
  status.dmc_interrupt                   = false;  // TODO

  has_irq_ = false;
  logger::log<logger::DEBUG_APU>("Read $%02X from Status (0x4015)\n", status.raw);
  return status.raw;
}

void hw::apu::APU::writeRegister(uint16_t address, uint8_t data) {
  switch (address) {

    // Channel 1 (Square 1)
    case 0x4000:
    case 0x4001:
    case 0x4002:
    case 0x4003:
      square_1.writeReg(address & 0x03, data);
      logger::log<logger::DEBUG_APU>("Write $%02X to Square 1 ($%04X)\n", data, address);
      break;

      // Channel 2 (Square 2)
    case (0x4004):
    case (0x4005):
    case (0x4006):
    case (0x4007):
      square_2.writeReg(address & 0x03, data);
      logger::log<logger::DEBUG_APU>("Write $%02X to Square 2 ($%04X)\n", data, address);
      break;

    // Triangle
    case 0x4008:
    case 0x4009:
    case 0x400A:
    case 0x400B:
      triangle.writeReg(address & 0x03, data);
      logger::log<logger::DEBUG_APU>("Write $%02X to Triangle ($%04X)\n", data, address);
      break;

    // Noise
    case 0x400C:
    case 0x400D:
    case 0x400E:
    case 0x400F:
      noise.writeReg(address & 0x03, data);
      logger::log<logger::DEBUG_APU>("Write $%02X to Noise ($%04X)\n", data, address);
      break;

    // TODO: DMC
    case 0x4010:
    case 0x4011:
    case 0x4012:
    case 0x4013:
      break;

    // Control
    case (0x4015):
      sound_en_.raw = data;
      square_1.enable(sound_en_.ch_1);
      square_2.enable(sound_en_.ch_2);
      triangle.enable(sound_en_.ch_3);
      noise.enable(sound_en_.ch_4);
      dmc.enable(sound_en_.ch_5);
      if (!sound_en_.ch_5) {
        ;  // TODO: Set bytes remaining to 0
      } else {
        ;  // TODO: Restart DMC sample only if bytes remaining > 0
      }
      logger::log<logger::DEBUG_APU>("Write $%02X to Status (0x4015)\n", data);
      break;

    // Frame counter
    case 0x4017:
      irq_inhibit_        = data & 0x40;
      frame_counter_mode_ = data & 0x80;

      if (irq_inhibit_) {
        has_irq_ = false;
      }

      // TODO: These timings are wrong
      if (cycle_count_ % 2) {  // If odd:
        frame_counter_reset_counter_ = cycle_count_ + 2;
      } else {
        frame_counter_reset_counter_ = cycle_count_ + 2;
      }
      break;

    default:
      logger::log<logger::ERROR>("Attempted to write invalid APU addr $%02X\n", address);
      break;
  }
}
