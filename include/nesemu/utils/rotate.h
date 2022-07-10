#pragma once

#include <cstdint>
#include <limits>

template <typename T, int N = std::numeric_limits<T>::digits>
static inline T rotl(T n, unsigned int c) {
  static_assert(std::numeric_limits<T>::digits >= N);
  const unsigned int mask = ((1 << N) - 1);
  c %= N;
  return ((n << c) & mask) | (n >> (N - c));
}

template <typename T, int N = std::numeric_limits<T>::digits>
static inline T rotr(T n, unsigned int c) {
  static_assert(std::numeric_limits<T>::digits >= N);
  const unsigned int mask = ((1 << N) - 1);
  c %= N;
  return (n >> c) | ((n << (N - c)) & mask);
}
