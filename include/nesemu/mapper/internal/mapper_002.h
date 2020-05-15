#pragma once

#include <nesemu/mapper/mapper_base.h>


namespace mapper::internal {

class Mapper002 : public Mapper {
public:
  using Mapper::Mapper;  // "Inherit" constructor

  uint32_t decodeCPUAddress(uint16_t addr) const override {
    uint8_t bank_num = 0;

    if (addr < 0xC000) {                // 0x8000-0xBFFF
      bank_num = prg_bank_;             //
    } else {                            // 0xC000-0xFFFF
      bank_num = (prg_banks_ * 2) - 1;  //
    }

    return (0x2000 * bank_num) | (addr & 0x1FFF);
  };

  void write(uint16_t addr, uint8_t data) override { prg_bank_ = data; };

private:
  uint8_t prg_bank_ = {0};
};

}  // namespace mapper::internal