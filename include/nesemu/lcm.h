#pragma once


#if (__cplusplus < 201703L)

#include <type_traits>

namespace lcm {

template <typename _Tp>
constexpr typename std::enable_if<std::__and_<std::is_integral<_Tp>, std::is_signed<_Tp>>::value, _Tp>::type
    __abs_integral(_Tp __val) {
  return __val < 0 ? -__val : __val;
}

template <typename _Tp>
constexpr typename std::enable_if<std::__and_<std::is_integral<_Tp>, std::is_unsigned<_Tp>>::value, _Tp>::type
    __abs_integral(_Tp __val) {
  return __val;
}

template <typename _Mn, typename _Nn>
constexpr typename std::common_type<_Mn, _Nn>::type __gcd(_Mn __m, _Nn __n) {
  return __m == 0 ? __abs_integral(__n) : __n == 0 ? __abs_integral(__m) : __gcd(__n, __m % __n);
}

template <typename _Mn, typename _Nn>
constexpr typename std::common_type<_Mn, _Nn>::type lcm(_Mn __m, _Nn __n) {
  static_assert(std::is_integral<_Mn>::value, "lcm arguments are integers");
  static_assert(std::is_integral<_Nn>::value, "lcm arguments are integers");
  static_assert(!std::is_same<std::remove_cv<_Mn>, bool>::value, "lcm arguments are not bools");
  static_assert(!std::is_same<std::remove_cv<_Nn>, bool>::value, "lcm arguments are not bools");
  return (__m != 0 && __n != 0) ? (__abs_integral(__m) / __gcd(__m, __n)) * __abs_integral(__n) : 0;
}

}  // namespace lcm

#else

#include <numeric>

namespace lcm {
using std::lcm;
}

#endif
