#pragma once

#include <nesemu/hw/mapper/mapper_base.h>


namespace hw::mapper::internal {

class Mapper003 : public Mapper {
public:
  using Mapper::Mapper;  // "Inherit" constructor

  uint32_t decodePPUAddress(uint16_t addr) const override { return (0x2000 * (chr_bank_)) | (addr & 0x1FFF); };

  void write(uint16_t addr, uint8_t data) override { chr_bank_ = data; };

private:
  uint8_t chr_bank_ = {0};
};

}  // namespace hw::mapper::internal
