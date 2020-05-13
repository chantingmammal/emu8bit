#pragma once

#include <cstdint>


namespace mapper {

constexpr unsigned PRG_ROM_OFFSET = 0x8000;
constexpr unsigned CHR_ROM_OFFSET = 0x0000;

class Mapper {
public:
  explicit Mapper(uint8_t prg_banks, uint8_t chr_banks) : prg_banks_(prg_banks), chr_banks_(chr_banks) {};
  virtual ~Mapper() = default;

  virtual uint32_t decodeCPUAddress(uint16_t addr) const { return addr - PRG_ROM_OFFSET; };
  virtual uint32_t decodePPUAddress(uint16_t addr) const { return addr - CHR_ROM_OFFSET; };
  virtual bool     hasIRQ() const { return false; }
  virtual void     write(uint16_t /*addr*/, uint8_t /*data*/) {};
  virtual void     clock() {};


protected:
  uint8_t prg_banks_;  ///< Number of 16KiB PRG ROM banks
  uint8_t chr_banks_;  ///< Number of 8KiB CHR ROM/RAM banks
};

}  // namespace mapper