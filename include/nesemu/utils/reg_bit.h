#pragma once

#include <cstdint>


namespace utils {

// Bitfield utilities
template <unsigned bit_pos, unsigned n_bits = 1, typename T = uint8_t>
struct RegBit {
  T                  data;
  static constexpr T mask = ((1 << n_bits) - 1) << bit_pos;

  operator T() const { return (data & mask) >> bit_pos; }

  RegBit& operator=(const RegBit& val) {
    *this = (T) val;
    return *this;
  }

  RegBit& operator=(T val) {
    data = (data & ~mask) | ((val << bit_pos) & mask);
    return *this;
  }

  RegBit& operator+=(T val) {
    *this = (*this + val);
    return *this;
  }

  RegBit& operator|=(T val) {
    data |= ((val << bit_pos) & mask);
    return *this;
  }

  RegBit& operator&=(T val) {
    data &= ((val << bit_pos) & mask);
    return *this;
  }
} __attribute__((__packed__));

}  // namespace utils
