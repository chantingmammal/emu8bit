#pragma once

#include <nesemu/hw/mapper/mapper_base.h>
#include <nesemu/logger.h>


namespace hw::mapper::internal {

// PRG ROM:
//      0x8000-0xFFFF: Switchable

// CHR ROM/RAM:
//      0x0000-0x1FFF: Unbanked, but 4K auto-switchable

class Mapper163 : public Mapper {
public:
  using Mapper::Mapper;  // "Inherit" constructor

  uint32_t decodeCPUAddress(uint16_t addr) const override {
    const uint8_t bank_num = bank_select_ | (bank_sel_low_ ? 0b00 : 0b11);
    return (0x8000 * bank_num) | (addr & 0x7FFF);
  }

  uint32_t decodePPUAddress(uint16_t addr) const override {
    // Latch A9 on A13 rising edge
    const bool a13 = addr >> 13;
    if (!ppu_a13_latch_ && a13) {
      ppu_a9_latch_ = !!(addr >> 9);
    }
    ppu_a13_latch_ = a13;

    if (chr_ram_switch_ && !a13) {
      return (addr & 0x0FFF) | (ppu_a9_latch_ << 12);
    } else {
      return (addr & 0x1FFF);
    }
  }

  void write(uint16_t addr, uint8_t data) override {
    const bool diff      = swap_d0_d1_ && ((data & 0x01) ^ ((data >> 1) & 0x01));
    const bool data_swap = data ^ (diff | diff << 1);

    switch (addr & 0xFF01) {
      case 0x5000:  // PRG Bank Low/CHR-RAM Switch
      case 0x5001:
        bank_select_    = (bank_select_ & 0xF0) | (data_swap & 0x0F);
        chr_ram_switch_ = data_swap & 0x80;
        logger::log<logger::DEBUG_MAPPER>("Set bank select: %d, CHR RAM swap: %d\n", bank_select_, chr_ram_switch_);
        break;
      case 0x5200:  // PRG Bank High
      case 0x5201:
        bank_select_ = (bank_select_ & 0x0F) | ((data_swap & 0x03) << 4);
        logger::log<logger::DEBUG_MAPPER>("Set bank select: %d\n", bank_select_);
        break;
      case 0x5100:  // Feedback Write (A=0)
        e_ = data_swap & 0x01;
        f_ = data_swap & 0x04;
        logger::log<logger::DEBUG_MAPPER>("Set f latch: %d\n", f_);
        break;
      case 0x5101: {  // Feedback Write (A=1)
        bool new_e = data_swap & 0x01;
        if (e_ && !new_e) {
          f_ = !f_;
        }
        e_ = new_e;
        logger::log<logger::DEBUG_MAPPER>("Set f latch: %d\n", f_);
      } break;
      case 0x5300:  // Mode
      case 0x5301:
        swap_d0_d1_   = (data & 0x01);
        bank_sel_low_ = (data & 0x04);
        logger::log<logger::DEBUG_MAPPER>("Swap D0,D1: %d, No force bank 0b11: %d\n", swap_d0_d1_, bank_sel_low_);
        break;
      default:
        logger::log<logger::ERROR>("Attempted to write invalid mapper addr $%02X\n", addr);
        break;
    }
  }

  void read(uint16_t addr, uint8_t& data) override {
    if ((addr & 0xF300) == 0x5500) {
      data = (!f_) << 2;
      logger::log<logger::DEBUG_MAPPER>("Get f latch: %d\n", data);
    }
  }

private:
  uint8_t bank_select_    = {0};      // 0x5000 (lower 4bit) | 0x5200 (upper 2bit)
  bool    chr_ram_switch_ = {false};  // CHR A12=PPU A9
  bool    swap_d0_d1_     = {false};  // Swap D0 and D1 on writes to 0x5000-0x5200
  bool    bank_sel_low_   = {false};  // Override bank_select_[0..1] = 0b11

  mutable bool ppu_a13_latch_ = {false};
  mutable bool ppu_a9_latch_  = {false};

  bool e_ = {false};
  bool f_ = {false};
};

}  // namespace hw::mapper::internal
