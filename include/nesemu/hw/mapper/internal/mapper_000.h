#pragma once

#include <nesemu/hw/mapper/mapper_base.h>


namespace hw::mapper::internal {

class Mapper000 : public Mapper {
public:
  using Mapper::Mapper;  // "Inherit" constructor

  uint32_t decodeCPUAddress(uint16_t addr) const override { return addr & (prg_banks_ > 1 ? 0x7FFF : 0x3FFF); };
};

}  // namespace hw::mapper::internal
