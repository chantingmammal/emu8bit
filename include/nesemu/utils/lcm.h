#pragma once


#if (__cplusplus < 201703L)

#include <type_traits>

namespace utils::lcm {

template <typename _Tp>
constexpr typename std::enable_if_t<std::is_integral_v<_Tp> && std::is_signed_v<_Tp>, _Tp>
    __abs_integral(_Tp __val) {
  return __val < 0 ? -__val : __val;
}

template <typename _Tp>
constexpr typename std::enable_if_t<std::is_integral_v<_Tp> && std::is_unsigned_v<_Tp>, _Tp>
    __abs_integral(_Tp __val) {
  return __val;
}

template <typename _Mn, typename _Nn>
constexpr typename std::common_type_t<_Mn, _Nn> __gcd(_Mn __m, _Nn __n) {
  return __m == 0 ? __abs_integral(__n) : __n == 0 ? __abs_integral(__m) : __gcd(__n, __m % __n);
}

template <typename _Mn, typename _Nn>
constexpr typename std::common_type_t<_Mn, _Nn> lcm(_Mn __m, _Nn __n) {
  static_assert(std::is_integral_v<_Mn>, "lcm arguments are integers");
  static_assert(std::is_integral_v<_Nn>, "lcm arguments are integers");
  static_assert(!std::is_same_v<std::remove_cv<_Mn>, bool>, "lcm arguments are not bools");
  static_assert(!std::is_same_v<std::remove_cv<_Nn>, bool>, "lcm arguments are not bools");
  return (__m != 0 && __n != 0) ? (__abs_integral(__m) / __gcd(__m, __n)) * __abs_integral(__n) : 0;
}

}  // namespace utils::lcm

#else

#include <numeric>

namespace utils::lcm {
using std::lcm;
}

#endif
