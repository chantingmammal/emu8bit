#pragma once

#include <type_traits>


namespace utils {

// Enum utilities
template <typename Enumeration>
constexpr typename std::underlying_type_t<Enumeration> asInt(const Enumeration value) {
  return static_cast<typename std::underlying_type_t<Enumeration>>(value);
}

template <typename Enumeration>
constexpr Enumeration asEnum(const int value) {
  return static_cast<Enumeration>(value);
}

}  // namespace utils
