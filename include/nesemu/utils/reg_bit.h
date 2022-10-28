#pragma once

#include <nesemu/utils/compat.h>

#include <cstdint>
#include <type_traits>


namespace utils {

// Bitfield utilities
template <unsigned bit_pos,
          unsigned n_bits = 1,
          typename T      = uint8_t,
          typename C      = std::conditional_t<(n_bits == 1), bool, T>>
PACKED(struct RegBit {
  T                  data;
  static constexpr T mask = ((1 << n_bits) - 1) << bit_pos;

  operator C() const {
    if constexpr (std::is_same_v<C, bool>) {
      return data & mask;
    } else {
      return (data & mask) >> bit_pos;
    }
  }

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
});


template <unsigned bit_pos,
          unsigned n_bits = 1,
          typename T      = std::conditional_t<(n_bits == 1), bool, uint8_t>,
          typename S      = uint8_t>
struct StructField {
  T data;

  inline operator T&() { return data; }
  inline operator T() const { return data; }

  T& operator=(T val) {
    data = val & ((1 << n_bits) - 1);
    return data;
  }

  T& from(S s) { return *this = (s >> bit_pos) & ((1 << n_bits) - 1); }

  S to() const { return data << bit_pos; }
};

}  // namespace utils
