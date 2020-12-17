#pragma once

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>


namespace utils {

// Byte utilities
constexpr int8_t deComplement(uint8_t value) {
  return (value & 0xF0) ? -uint8_t(~value + 1) : value;
}

// TODO: Make more lightweight
inline std::string uint8_to_hex_string(const uint8_t* arr, const std::size_t size) {
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
