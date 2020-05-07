#include <nesemu/system_bus.h>

#include <nesemu/cpu.h>
#include <nesemu/joystick.h>
#include <nesemu/mapper/mapper_base.h>
#include <nesemu/ppu.h>

void system_bus::SystemBus::connectChips(apu::APU*           apu,
                                         cpu::CPU*           cpu,
                                         ppu::PPU*           ppu,
                                         joystick::Joystick* joy_1,
                                         joystick::Joystick* joy_2) {
  apu_   = apu;
  cpu_   = cpu;
  ppu_   = ppu;
  joy_1_ = joy_1;
  joy_2_ = joy_2;
}

void system_bus::SystemBus::loadCart(mapper::Mapper* mapper, uint8_t* prg_rom, uint8_t* expansion_ram) {
  mapper_        = mapper;
  prg_rom_       = prg_rom;
  expansion_ram_ = expansion_ram;
}


uint8_t system_bus::SystemBus::read(uint16_t address) const {
#if DEBUG
  const uint8_t data = readByteInternal(address);
  std::cout << "Read " << unsigned(data) << " from $(" << address << ")\n";
  return data;
}

uint8_t system_bus::SystemBus::readInternal(uint16_t address) const {
#endif
  address &= 0xFFFF;

  if (address < 0x2000) {  // Stack and RAM
    return ram_[address & 0x07FF];
  }

  else if (address < 0x4000) {  // PPU Registers
    return ppu_->readRegister((address & 0x0007) | 0x2000);
  }

  else if (address < 0x4014) {  // Sound Registers
    return 0;                   // TODO
  }

  else if (address == 0x4014) {  // PPU DMA Access
    return 0;                    // Cannot read DMA register
  }

  else if (address == 0x4015) {  // Sound Channel Switch
    return 0;                    // Cannot read sound channel register
  }

  else if (address == 0x4016) {  // Joystick 1
    return joy_1_->read();
  }

  else if (address == 0x4017) {  // Joystick 2
    return joy_2_->read();
  }

  else if (address == 0x4020) {  // Unused
    return 0;
  }

  else if (address < 0x6000) {  // Expansion Modules, ie. Famicom Disk System
    return 0;
  }

  else if (address < 0x8000) {  // Cartridge RAM
    if (expansion_ram_) {
      return expansion_ram_[address - 0x6000];
    } else {
      return 0;  // No cartridge RAM
    }
  }

  else {  // Cartridge ROM
    return prg_rom_[mapper_->decodeCPUAddress(address)];
  }
}


void system_bus::SystemBus::write(uint16_t address, uint8_t data) {
#if DEBUG
  std::cout << "Write " << unsigned(data) << " to $(" << address << ")\n";
#endif
  address &= 0xFFFF;

  if (address < 0x2000) {  // Stack and RAM
    ram_[address & 0x07FF] = data;
  }

  else if (address < 0x4000) {  // PPU Registers
    ppu_->writeRegister((address & 0x0007) | 0x2000, data);
  }

  else if (address < 0x4014) {  // Sound Registers
    ;                           // TODO
  }

  else if (address == 0x4014) {  // PPU DMA Access
    if (data * 0x100 < 0x2000) {
      ppu_->spriteDMAWrite(ram_ + (data * 0x100));  // Internal RAM
    } else if (data * 0x100 < 0x6000) {
      ;  // Cannot DMA from MMIO
    } else if (data * 0x100 < 0x8000) {
      if (expansion_ram_) {
        ppu_->spriteDMAWrite(expansion_ram_ + (data * 0x100) - 0x6000);
      } else {
        ;  // No cartridge RAM
      }
    } else {
      ppu_->spriteDMAWrite(prg_rom_ + (data * 0x100) - 0x8000);  // Cartridge ROM
    }
  }

  else if (address == 0x4015) {  // Sound Channel Switch
    ;                            // TODO
  }

  else if (address == 0x4016) {  // Joystick Strobe
    joy_1_->write(data);
    joy_2_->write(data);
  }

  else if (address == 0x4017) {  // APU frame counter
    ;                            // TODO
  }

  else if (address == 0x4020) {  // Unused
    ;
  }

  else if (address < 0x6000) {  // Expansion Modules, ie. Famicom Disk System
    ;
  }

  else if (address < 0x8000) {  // Cartridge RAM
    if (expansion_ram_) {
      expansion_ram_[address - 0x6000] = data;
    } else {
      ;  // No cartridge RAM
    }
  }

  else {  // Mapper
    mapper_->write(address, data);
  }
}