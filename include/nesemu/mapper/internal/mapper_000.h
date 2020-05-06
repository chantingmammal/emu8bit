#pragma once

#include <nesemu/mapper/mapper_base.h>


namespace mapper::internal {

class Mapper000 : public Mapper {
public:
  using Mapper::Mapper;  // "Inherit" constructor

  uint16_t decodeCPUAddress(uint16_t addr) const override { return addr & (prg_banks_ > 1 ? 0x7FFF : 0x3FFF); };
};

}  // namespace mapper::internal