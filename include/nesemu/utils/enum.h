#pragma once


namespace utils {

// Enum utilities
template <typename Enumeration>
constexpr typename std::underlying_type<Enumeration>::type asInt(const Enumeration value) {
  return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

template <typename Enumeration>
constexpr Enumeration asEnum(const int value) {
  return static_cast<Enumeration>(value);
}

}  // namespace utils
