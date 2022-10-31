#pragma once

#include "internal/mapper_000.h"
#include "internal/mapper_001.h"
#include "internal/mapper_002.h"
#include "internal/mapper_003.h"
#include "internal/mapper_004.h"
#include "internal/mapper_163.h"
#include <nesemu/hw/mapper/mapper_base.h>
#include <nesemu/logger.h>

#include <map>


namespace hw::mapper {

namespace internal {

template <class T>
Mapper* make(uint8_t prg_banks, uint8_t chr_banks, Mirroring mirror) {
  return new T(prg_banks, chr_banks, mirror);
}

Mapper* dummy(uint8_t /*prg_banks*/, uint8_t /*chr_banks*/, Mirroring /*mirror*/) {
  return nullptr;
}

using mapper_generator = Mapper* (*) (uint8_t, uint8_t, Mirroring);

std::map<uint8_t, mapper_generator> mappers = {
    {0, internal::make<internal::Mapper000>},    // Mapper 000 - Nintendo NROM
    {1, internal::make<internal::Mapper001>},    // Mapper 001 - Nintendo MMC1
    {2, internal::make<internal::Mapper002>},    // Mapper 002 - Nintendo UxROM
    {3, internal::make<internal::Mapper003>},    // Mapper 003 - Nintendo CNROM
    {4, internal::make<internal::Mapper004>},    // Mapper 004 - Nintendo MMC3
    {163, internal::make<internal::Mapper163>},  // Mapper 163 - Nánjīng FC-001
};

}  // namespace internal

internal::mapper_generator getMapper(uint8_t mapper_num) {
  if (internal::mappers.find(mapper_num) != internal::mappers.end()) {
    return internal::mappers[mapper_num];
  } else {
    logger::log<logger::ERROR>("Mapper #%d not supported!\n", mapper_num);
    return internal::dummy;
  }
}

}  // namespace hw::mapper
