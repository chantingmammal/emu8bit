#pragma once

#include <cstdio>


namespace logger {

enum Level {
  NONE         = 0x00,
  ERROR        = 0x01,
  WARNING      = 0x02,
  INFO         = 0x04,
  DEBUG_CPU    = 0x08,
  DEBUG_PPU    = 0x10,
  DEBUG_APU    = 0x20,
  DEBUG_BUS    = 0x40,
  DEBUG_MAPPER = 0x80,
};

extern Level level;

template <Level L, typename... T>
inline void log(const char* format, T... args) {
  if (L & level) {
    printf(format, args...);
  }
}


}  // namespace logger