#pragma once

#include <cstdint>


namespace hw::apu::unit {

/**
 * A simple clock divider, outputs a clock every N input clocks.
 *
 * Contains a counter which is decremented on the arrival of each clock. When it reaches 0, it is reloaded
 * with the period and an output clock is generated (`clock()` returns true).
 */
template <class T = uint8_t>
class Divider {
public:
  void setLoop(bool loop) { loop_ = loop; }       ///< Whether to loop when the counter reaches 0
  void setPeriod(T period) { period_ = period; }  ///< Set divider period
  void reload() { counter_ = period_; }           ///< Reload divider with period
  bool clock() {                                  ///< If 0, reload, and return true. Otherwise, decrement

    if (counter_ == 0) {
      if (loop_) {
        reload();
      }
      return true;
    }

    counter_--;
    return false;
  }

private:
  bool loop_    = {true};
  T    period_  = {0};
  T    counter_ = {0};
};


/**
 * A saw envelope generator with optional looping.
 *
 * Contains a `Divider`, whose output clocks a decay level counter. Each clock of the decay level decreases its value
 * from 15 to 0. If `loop_` is set, the decay level counter is reset to 15 if clocked when equal to 0.
 *
 * If `const_volume_` is set, the output is the constant `volume_`. Otherwise, the output is the decay level.
 *
 *
 * Should be clocked by the quarter-frame clock (240Hz)
 */
class Envelope {
public:
  Divider<uint8_t> divider_;  // 4-bit divider. Max = 15

  uint8_t volume_;           // 4-bit. The volume of the channel, if not using envelope
  bool    const_volume_;     // 1=Constant volume, 0=Envelope volume
  bool    loop_  = {false};  // Whether to loop when the decay level reaches 0
  bool    start_ = {false};  // Flag to indicate restart

  void    clock();
  uint8_t getVolume() { return const_volume_ ? volume_ : decay_level_; }

private:
  uint8_t decay_level_ = {0};  // 4-bit. 0 to 15. The current decay level
};


/**
 * The sweep unit can periodically adjust a square channel's period.
 *
 * Contains a `Divider`, whose output causes the channel's period to be updated with the target period if the unit is
 * enabled and is not muting the channel.
 *
 * The target period is calculated continuously, as follows:
 *  1) The channel's period is shifted by `shift_count_` bits
 *  2) If `negate_` is set, the result is negated. On the first square channel, the negated value is decremented by 1.
 *  3) The target period is the sum of the current period and the value calculated in step 2.
 *
 * If at any time the target period is greater than 0x7FF, the sweep unit mutes the channel, regardless of the state of
 * the `enable_` flag and the `Divider`.
 *
 * If at any time the current period is less than 8, the sweep unit mutes the channel.
 *
 *
 * Should be clocked by the half-frame clock (120Hz).
 */
class Sweep {
public:
  uint16_t* channel_period_ = {nullptr};
  bool      is_ch_2_        = {false};

  Divider<uint8_t> divider_;                // 3-bit divider. Max 7
  uint8_t          shift_count_ = {0};      // 3-bit. Number of bits to shift
  bool             negate_      = {false};  // Subtract from period, sweep toward higher frequencies
  bool             enable_      = {false};  // Whether to allow the period to be updated
  bool             reload_      = {false};  // Flag to force a reload

  void     clock();
  uint16_t getTarget();
  uint8_t  getOutput(uint8_t input);

private:
  bool mute();
};


/**
 * The length counter allows for automatic duration control.
 *
 * When clocked, if the half flag is clear and the counter is non-zero, the counter is decremented. When the counter is
 * zero, the corresponding channel is silenced.
 *
 * Should be clocked by the half-frame clock (120Hz).
 */
class LengthCounter {
public:
  uint8_t counter_ = {0};  // 0 to 254
  bool    halt_    = {false};

  void    load(uint8_t code);
  void    clock();
  uint8_t getOutput(uint8_t input);
};


/**
 * TODO
 */
class LinearCounter {
public:
  uint8_t counter_      = {0};  // 0 to 127
  uint8_t reload_value_ = {0};  // 0 to 127
  bool    control_      = {false};
  bool    reload_       = {false};

  void clock();
};

}  // namespace hw::apu::unit
