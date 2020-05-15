#pragma once

#include <nesemu/apu/units.h>
#include <nesemu/utils.h>

#include <cstdint>


namespace apu::channel {

class Channel {
public:
  virtual void    clock()     = 0;
  virtual uint8_t getOutput() = 0;
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
    sweep.channel_period_ = &timer;
    sweep.is_ch_2_        = (channel == 2);
  }

  const uint8_t sequence_[4] = {0b01000000, 0b01100000, 0b01111000, 0b10011111};

  uint8_t  duty_cycle;  // 2-bit. 12.5%, 25%, 50%, or -25%.
  uint16_t timer;       // 11-bit.

  unit::Envelope      envelope;
  unit::Sweep         sweep;
  unit::LengthCounter length_counter;

  void    clock() override;
  uint8_t getOutput() override;
};


/**
 * Block Diagram:
 *
 *          Linear Counter   Length Counter
 *                |                |
 *                v                v
 *    Timer ---> Gate ----------> Gate ---> Sequencer ---> (to mixer)
 */
struct Triangle : public Channel {
  union {
    uint8_t             raw_0 = {0};
    utils::RegBit<0, 7> linear_counter_load;
    utils::RegBit<7, 1> halt_control_flag;
  };
  union {
    uint8_t raw_1 = {0};
    uint8_t unused;
  };
  union {
    uint8_t raw_2 = {0};
    uint8_t timer_low;
  };
  union {
    uint8_t             raw_3 = {0};
    utils::RegBit<0, 3> timer_high;
    utils::RegBit<3, 5> length_counter_;
  };

  unit::LengthCounter length_counter;

  void    clock() override {}
  uint8_t getOutput() override {}
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
  void    clock() override;
  uint8_t getOutput() override;

private:
  uint16_t lfsr = {0};  // 15-bit linear feedback shift register
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

  void    clock() override {};
  uint8_t getOutput() override {};
};

}  // namespace apu::channel
