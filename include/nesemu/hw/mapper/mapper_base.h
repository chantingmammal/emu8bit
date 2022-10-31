#pragma once

#include <nesemu/utils/compat.h>

#include <cstdint>


namespace hw::mapper {

constexpr unsigned PRG_ROM_OFFSET = 0x8000;
constexpr unsigned CHR_ROM_OFFSET = 0x0000;

enum class Mirroring { none, vertical, horizontal, single_lower, single_upper };

class Mapper {
public:
  explicit Mapper(uint8_t prg_banks, uint8_t chr_banks, Mirroring mirror)
      : prg_banks_(prg_banks), chr_banks_(chr_banks), mirroring_(mirror) {};
  virtual ~Mapper() = default;

  virtual uint32_t decodeCPUAddress(uint16_t addr) const { return addr - PRG_ROM_OFFSET; }

  virtual uint32_t decodePPUAddress(uint16_t addr) const {
    // Cartridge VRAM/VROM
    // 0x0000-0x1FFF
    return (addr & 0x1FFF) - CHR_ROM_OFFSET;
  }

  virtual uint32_t decodeCIRAMAddress(uint16_t addr) const {
    // Nametables
    // 0x2000-0x2FFF, mirrored to 0x3EFF
    // Mapped to 0x0000-0x0FFF
    if (addr < 0x3F00) {
      addr &= 0x0FFF;
      switch (mirroring_) {
        case Mirroring::none:  // Four-screen VRAM layout
          return addr;
        case Mirroring::vertical:  // Vertical mirroring
          return addr & ~0x0800;
        case Mirroring::horizontal:  // Horizontal mirroring
          return addr & ~0x0400;
        case Mirroring::single_lower:  // Single-screen, lower tilemap
          return addr & 0x03FF;
        case Mirroring::single_upper:  // Single-screen, upper tilemap
          return (addr & 0x03FF) | 0x0400;
          // No default
      }
    }

    // Palettes
    // 0x3F00-0x3F1F, mirrored to 0x3FFF
    // Mapped to 0x1F00-0x1F1F
    else {

      // Color #0 of each sprite palette mirrors the corresponding background palette
      addr &= 0x1F;
      switch (addr) {
        case 0x10:
        case 0x14:
        case 0x18:
        case 0x1C:
          addr &= ~0x10;
      }
      return 0x1F00 | addr;
    }
    utils::unreachable();
  }

  virtual bool hasIRQ() const { return false; }
  virtual void read(uint16_t /*addr*/, uint8_t& /*data*/) {};
  virtual void write(uint16_t /*addr*/, uint8_t /*data*/) {};
  virtual void clock() {};


protected:
  uint8_t   prg_banks_;  ///< Number of 16KiB PRG ROM banks
  uint8_t   chr_banks_;  ///< Number of 8KiB CHR ROM/RAM banks
  Mirroring mirroring_;  ///< Mirroring scheme
};

}  // namespace hw::mapper
