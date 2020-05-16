#pragma once

#include <nesemu/mapper/mapper_base.h>


namespace mapper::internal {

// PRG ROM bank mode (control_ & 0x0C):
//
// 0, 1:
//      0x8000-0xFFFF: Switchable (prg_bank_, ignore LSB)
//
// 2:
//      0x8000-0xBFFF: Fixed, first bank
//      0xC000-0xFFFF: Switchable (prg_bank_)
//
// 3:
//      0x8000-0xBFFF: Switchable (prg_bank_)
//      0xC000-0xFFFF: Fixed, last bank

// CHR ROM/RAM bank mode (bank_select_ & 0x10):
//
// 0:
//      0x0000-0x1FFF: Switchable (chr_bank_0_, ignore LSB)
//
// 1:
//      0x0000-0x0FFF: Switchable (chr_bank_0_)
//      0x1000-0x1FFF: Switchable (chr_bank_2_)

class Mapper001 : public Mapper {
public:
  using Mapper::Mapper;  // "Inherit" constructor

  uint32_t decodeCPUAddress(uint16_t addr) const override {
    switch (control_ & 0x0C) {
      case (0x00):  // Mode 0
      case (0x04):  // Mode 1
        return (0x8000 * (prg_bank_ & 0xFE)) | (addr & 0x7FFF);

      case (0x08):  // Mode 2
        if (addr < 0xC000) {
          return addr & 0x3FFF;
        } else {
          return (0x4000 * prg_bank_) | (addr & 0x3FFF);
        }

      case (0x0C):  // Mode 3
        if (addr < 0xC000) {
          return (0x4000 * prg_bank_) | (addr & 0x3FFF);
        } else {
          return (0x4000 * (prg_banks_ - 1)) | (addr & 0x3FFF);
        }

      default:
        __builtin_unreachable();
    }
  };


  uint32_t decodePPUAddress(uint16_t addr) const override {
    addr &= 0x1FFF;

    if (control_ & 0x10) {
      if (addr < 0x1000) {
        return (0x1000 * chr_bank_0_) | (addr & 0x0FFF);
      } else {
        return (0x1000 * chr_bank_1_) | (addr & 0x0FFF);
      }
    } else {
      return (0x2000 * (chr_bank_0_ & 0xFE)) | (addr & 0x1FFF);
    }
  };

  void write(uint16_t addr, uint8_t data) override {
    // TODO: Ignore consecutive writes, caused by double-writes in RMW instructions

    if (data & 0x80) {
      shift_register_ = 0x10;
      return;
    }

    if (shift_register_ & 0x01) {
      shift_register_ >>= 1;
      shift_register_ |= (data & 0x01) << 4;
      switch (addr & 0xE000) {
        case 0x8000:
          control_ = shift_register_;
          break;
        case 0xA000:
          chr_bank_0_ = shift_register_;
          break;
        case 0xC000:
          chr_bank_1_ = shift_register_;
          break;
        case 0xE000:
          prg_bank_ = shift_register_;
          break;
      }

      shift_register_ = 0x10;
    } else {
      shift_register_ >>= 1;
      shift_register_ |= (data & 0x01) << 4;
    }
  };

private:
  uint8_t shift_register_ = {0x10};

  uint8_t control_    = {0};  // 0x8000-0x9FFF
  uint8_t chr_bank_0_ = {0};  // 0xA000-0xBFFF
  uint8_t chr_bank_1_ = {0};  // 0xC000-0xDFFF
  uint8_t prg_bank_   = {0};  // 0xE000-0xFFFF
};

}  // namespace mapper::internal