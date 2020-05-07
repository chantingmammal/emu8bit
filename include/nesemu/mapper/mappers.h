#pragma once

#include "internal/mapper_000.h"
#include "internal/mapper_004.h"
#include <nesemu/mapper/mapper_base.h>

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
    internal::make<internal::Mapper000>,  // Mapper 000 - Nintendo NROM
    internal::dummy,                      // Mapper 001
    internal::dummy,                      // Mapper 002
    internal::dummy,                      // Mapper 003
    internal::make<internal::Mapper004>,  // Mapper 004 - Nintendo MMC3
    internal::dummy,                      // Mapper 005
    internal::dummy,                      // Mapper 006
    internal::dummy,                      // Mapper 007
    internal::dummy,                      // Mapper 008
    internal::dummy,                      // Mapper 009
    internal::dummy,                      // Mapper 010
                                          // Etc
};

}  // namespace mapper