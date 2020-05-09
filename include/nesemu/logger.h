#pragma once

#include <cstdio>


namespace logger {

enum Level {
  NONE         = 0x00,
  DEBUG_CPU    = 0x01,
  DEBUG_PPU    = 0x02,
  DEBUG_APU    = 0x04,
  DEBUG_BUS    = 0x08,
  DEBUG_MAPPER = 0x10,
  DEBUG_ALL    = DEBUG_CPU | DEBUG_PPU | DEBUG_APU | DEBUG_BUS | DEBUG_MAPPER,
};

extern Level level;

template <Level L, typename... T>
inline void log(const char* format, T... args) {
  if (L & level) {
    printf(format, args...);
  }
}


}  // namespace logger