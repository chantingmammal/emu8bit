#pragma once

#include <cstdint>


// Forward declarations
namespace apu {
class APU;
}

namespace cpu {
class CPU;
}

namespace ppu {
class PPU;
}

namespace joystick {
class Joystick;
}

namespace mapper {
class Mapper;
}


namespace system_bus {

/**
 * Represents the CPU address space and interrupt lines
 */
class SystemBus {
public:
  // Setup
  void connectChips(apu::APU* apu, cpu::CPU* cpu, ppu::PPU* ppu, joystick::Joystick* joy_1, joystick::Joystick* joy_2);
  void loadCart(mapper::Mapper* mapper, uint8_t* prg_rom, uint8_t* expansion_ram);


  // Execution
  bool    hasIRQ() const;
  bool    hasNMI() const;
  uint8_t read(uint16_t address) const;
  void    write(uint16_t address, uint8_t data);


private:
  // Memory
  mapper::Mapper* mapper_        = {nullptr};
  uint8_t         ram_[0x800]    = {0};        // 2KiB RAM, mirrored 4 times, at address 0x0000-0x1FFF
  uint8_t*        expansion_ram_ = {nullptr};  // Optional cartridge RAM,     at address 0x7000-0x7FFF
  uint8_t*        prg_rom_       = {nullptr};  // Unmapped program ROM,       at address 0x8000-0xFFFF


  // Chips
  apu::APU*           apu_;
  cpu::CPU*           cpu_;
  ppu::PPU*           ppu_;
  joystick::Joystick* joy_1_;
  joystick::Joystick* joy_2_;
};

}  // namespace system_bus
