#pragma once

#include <nesemu/hw/apu/apu_clock.h>
#include <nesemu/hw/apu/units.h>
#include <nesemu/utils/reg_bit.h>

#include <cstddef>
#include <cstdint>


namespace hw::apu::channel {

class Channel {
public:
  virtual void    clockCPU()                      = 0;
  virtual void    clockFrame(APUClock clock_type) = 0;
  virtual uint8_t getOutput()                     = 0;
};

template <typename T, size_t SEQ_LEN>
class Sequencer {
public:
  void resetSequencer() { sequencer_pos_ = 0; };

protected:
  void            advanceSequencer() { sequencer_pos_ = (sequencer_pos_ + 1) % SEQ_LEN; };
  virtual uint8_t getSequencerOutput() = 0;
  T               sequencer_pos_       = 0;
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
class Square : public Channel, public Sequencer<uint8_t, 8> {
public:
  Square(int channel) {
    sweep.channel_period_ = &timer;
    sweep.is_ch_2_        = (channel == 2);
  }

  uint8_t  duty_cycle;  // 2-bit. 12.5%, 25%, 50%, or -25%
  uint16_t timer;       // 11-bit

  unit::Envelope      envelope;
  unit::Sweep         sweep;
  unit::LengthCounter length_counter;

  void    clockCPU() override;
  void    clockFrame(APUClock clock_type) override;
  uint8_t getOutput() override;

private:
  static constexpr uint8_t SEQUENCE[4] = {0b01000000, 0b01100000, 0b01111000, 0b10011111};

  bool     clock_is_even_ = {false};
  uint16_t cur_time_      = {0};  // 11-bit

  uint8_t getSequencerOutput() override { return SEQUENCE[duty_cycle] * (1 << (8 - sequencer_pos_)); };
};


/**
 * Block Diagram:
 *
 *          Linear Counter   Length Counter
 *                |                |
 *                v                v
 *    Timer ---> Gate ----------> Gate ---> Sequencer ---> (to mixer)
 */
class Triangle : public Channel, public Sequencer<uint8_t, 32> {
public:
  uint16_t timer;  // 11-bit

  unit::LinearCounter linear_counter;
  unit::LengthCounter length_counter;

  void    clockCPU() override;
  void    clockFrame(APUClock clock_type) override;
  uint8_t getOutput() override { return getSequencerOutput(); };

private:
  static constexpr uint8_t SEQUENCE[32] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,  4,  3,  2,  1,  0,
                                           0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

  uint16_t cur_time_ = 0;  // 11-bit

  uint8_t getSequencerOutput() override { return SEQUENCE[sequencer_pos_]; };
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
  bool mode = {false};

  unit::Divider<uint16_t> timer;
  unit::Envelope          envelope;
  unit::LengthCounter     length_counter;

  void    loadPeriod(uint8_t code);
  void    clockCPU() override;
  void    clockFrame(APUClock clock_type) override;
  uint8_t getOutput() override;

private:
  uint16_t lfsr = {1};  // 15-bit linear feedback shift register. Initially loaded w/ 1
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
  union {
    uint8_t             raw_0 = {0};
    utils::RegBit<0, 4> frequency;
    utils::RegBit<6, 1> loop;
    utils::RegBit<7, 1> IRQ_enable;
  };
  union {
    uint8_t             raw_1 = {0};
    utils::RegBit<0, 7> load_counter_;
  };
  union {
    uint8_t raw_2 = {0};
    uint8_t sample_address;
  };
  union {
    uint8_t raw_3 = {0};
    uint8_t sample_length;
  };

  unit::LengthCounter length_counter;

  void    clockCPU() override {};
  void    clockFrame(APUClock clock_type) override;
  uint8_t getOutput() override { return 0; };
};

}  // namespace hw::apu::channel
