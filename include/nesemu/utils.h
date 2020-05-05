#pragma once

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>


namespace utils {

// Bitfield utilities
template <unsigned bit_pos, unsigned n_bits = 1, typename T = uint8_t>
struct RegBit {
  T                  data;
  static constexpr T mask = ((1 << n_bits) - 1) << bit_pos;

  operator T() const { return (data & mask) >> bit_pos; }

  RegBit& operator=(const RegBit& val) {
    *this = val.data;
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


// Enum utilities
template <typename Enumeration>
constexpr typename std::underlying_type<Enumeration>::type asInt(const Enumeration value) {
  return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

template <typename Enumeration>
constexpr Enumeration asEnum(const int value) {
  return static_cast<Enumeration>(value);
}


// Byte utilities
constexpr int8_t deComplement(uint8_t value) {
  return (value & 0xF0) ? -uint8_t(~value + 1) : value;
}

// TODO: Make more lightweight
inline std::string uint8_to_hex_string(const uint8_t* arr, const size_t size) {
  std::stringstream ss;

  ss << std::hex << std::setfill('0');

  for (unsigned i = 0; i < size; i++) {
    ss << "0x" << std::hex << std::setw(2) << static_cast<int>(arr[i]);
    if (i < size - 1) {
      ss << " ";
    }
  }

  return ss.str();
}

}  // namespace utils
