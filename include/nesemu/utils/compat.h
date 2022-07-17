#pragma once

namespace utils {

// std::unreachable()
#if defined(__GNUC__)  // GCC, Clang, etc
[[noreturn]] inline __attribute__((always_inline)) void unreachable() {
  __builtin_unreachable();
}
#elif defined(_MSC_VER)  // MSVC
[[noreturn]] __forceinline void unreachable() {
  __assume(false);
}
#else                    // ???
inline void unreachable() {}
#endif

// Packed structures
#if defined(__GNUC__)  // GCC, Clang, etc
#define PACKED(...) __VA_ARGS__ __attribute__((__packed__))
#elif defined(_MSC_VER)  // MSVC
#define PACKED(...) __pragma(pack(push, 1)) __VA_ARGS__ __pragma(pack(pop))
#else  // ???
#define PACKED(...) __VA_ARGS__
#endif

}  // namespace utils
