#pragma once

#include <nesemu/hw/apu/apu_clock.h>
#include <nesemu/hw/apu/units.h>
#include <nesemu/utils/reg_bit.h>

#include <cstdint>


namespace hw::apu::channel {


class Channel {
public:
  virtual void enable(bool enabled) {
    enabled_ = enabled;
    if (!enabled_) {
      length_counter_.counter_ = 0;
    }
  }
  virtual bool status() { return length_counter_.counter_ > 0; }

  virtual void    writeReg(uint8_t reg, uint8_t data) = 0;
  virtual void    clockCPU()                          = 0;
  virtual void    clockFrame(APUClock clock_type)     = 0;
  virtual uint8_t getOutput()                         = 0;

protected:
  bool                enabled_ = {false};
  unit::LengthCounter length_counter_;
};


/**
 * Block Diagram:
 *
 *                     Sweep -----> Timer
 *                       |            |
 *                       |            v
 *                       |        Sequencer   Length Counter
 *                       |            |             |
 *                       v            v             v
 *    Envelope -------> Gate -----> Gate -------> Gate ---> (to mixer)
 */
class Square : public Channel {
public:
  Square(int channel) {
    sweep_.channel_period_ = &period_;
    sweep_.is_ch_2_        = (channel == 2);
    timer_.setExtPeriod(&period_);
  }

  void    writeReg(uint8_t reg, uint8_t data) override;
  void    clockCPU() override;
  void    clockFrame(APUClock clock_type) override;
  uint8_t getOutput() override;

private:
  static constexpr uint8_t SEQUENCE[4] = {0b01000000, 0b01100000, 0b01111000, 0b10011111};

  bool     clock_is_even_ = {false};
  uint8_t  duty_cycle_;  // 2-bit. 12.5%, 25%, 50%, or -25%
  uint16_t period_;      // 11-bit. Used to reload timer.

  unit::Divider<uint16_t>     timer_;  // 11-bit
  unit::Sequencer<uint8_t, 8> sequencer_;
  unit::Envelope              envelope_;
  unit::Sweep                 sweep_;
};


/**
 * Block Diagram:
 *
 *          Linear Counter   Length Counter
 *                |                |
 *                v                v
 *    Timer ---> Gate ----------> Gate ---> Sequencer ---> (to mixer)
 */
class Triangle : public Channel {
public:
  Triangle() { timer_.setExtPeriod(&period_); }

  void    writeReg(uint8_t reg, uint8_t data) override;
  void    clockCPU() override;
  void    clockFrame(APUClock clock_type) override;
  uint8_t getOutput() override { return SEQUENCE[sequencer_.get()]; };

private:
  static constexpr uint8_t SEQUENCE[32] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,  4,  3,  2,  1,  0,
                                           0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

  uint16_t period_;  // 11-bit. Used to reload timer.

  unit::Divider<uint16_t>      timer_;  // 11-bit
  unit::Sequencer<uint8_t, 32> sequencer_;
  unit::LinearCounter          linear_counter_;
};


/**
 * Block Diagram:
 *
 *       Timer --> Shift Register   Length Counter
 *                       |                |
 *                       v                v
 *    Envelope -------> Gate ----------> Gate ---> (to mixer)
 */
class Noise : public Channel {
public:
  void    writeReg(uint8_t reg, uint8_t data) override;
  void    clockCPU() override;
  void    clockFrame(APUClock clock_type) override;
  uint8_t getOutput() override;

private:
  static constexpr uint16_t PERIODS[16] = {4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068};

  uint16_t lfsr_ = {1};  // 15-bit linear feedback shift register. Initially loaded w/ 1
  bool     mode_ = {false};

  unit::Divider<uint16_t> timer_;  // 11-bit
  unit::Envelope          envelope_;
};


/**
 * Block Diagram:
 *
 *                             Timer
 *                               |
 *                               v
 *    Reader ---> Buffer ---> Shifter ---> Output level ---> (to mixer)
 */
struct DMC : public Channel {
  void    writeReg(uint8_t reg, uint8_t data) override;
  void    clockCPU() override {};
  void    clockFrame(APUClock clock_type) override;
  uint8_t getOutput() override { return 0; };

private:
  union {
    uint8_t             raw_0_ = {0};
    utils::RegBit<0, 4> frequency_;
    utils::RegBit<6, 1> loop_;
    utils::RegBit<7, 1> IRQ_enable_;
  };
  union {
    uint8_t             raw_1_ = {0};
    utils::RegBit<0, 7> load_counter_;
  };
  union {
    uint8_t raw_2_ = {0};
    uint8_t sample_address_;
  };
  union {
    uint8_t raw_3_ = {0};
    uint8_t sample_length_;
  };
};

}  // namespace hw::apu::channel
