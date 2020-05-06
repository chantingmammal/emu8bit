#pragma once

#include "internal/mapper_000.h"

#include <iostream>


namespace mapper {

namespace internal {

template <class T>
Mapper* make(uint8_t prg_banks, uint8_t chr_banks) {
  return new T(prg_banks, chr_banks);
}

Mapper* dummy(uint8_t /*prg_banks*/, uint8_t /*chr_banks*/) {
  std::cerr << "Mapper not supported!" << std::endl;
  return nullptr;
}

}  // namespace internal


Mapper* (*mappers[])(uint8_t, uint8_t) = {
    internal::make<internal::Mapper000>,  // Mapper 000
    internal::dummy,                      // Mapper 001 - Not yet implemented
    internal::dummy,                      // Mapper 002 - Not yet implemented
    internal::dummy,                      // Mapper 003 - Not yet implemented
    internal::dummy,                      // Mapper 004 - Not yet implemented
    internal::dummy,                      // Mapper 005 - Not yet implemented
    internal::dummy,                      // Mapper 006 - Not yet implemented
    internal::dummy,                      // Mapper 007 - Not yet implemented
    internal::dummy,                      // Mapper 008 - Not yet implemented
    internal::dummy,                      // Mapper 009 - Not yet implemented
    internal::dummy,                      // Mapper 010 - Not yet implemented
                                          // Etc
};

}  // namespace mapper