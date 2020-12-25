#include <nesemu/hw/system_bus.h>

#include <nesemu/hw/apu/apu.h>
#include <nesemu/hw/clock.h>
#include <nesemu/hw/cpu.h>
#include <nesemu/hw/joystick.h>
#include <nesemu/hw/mapper/mapper_base.h>
#include <nesemu/hw/ppu.h>
#include <nesemu/logger.h>


void hw::system_bus::SystemBus::connectChips(clock::CPUClock*    clock,
                                             apu::APU*           apu,
                                             cpu::CPU*           cpu,
                                             ppu::PPU*           ppu,
                                             joystick::Joystick* joy_1,
                                             joystick::Joystick* joy_2) {
  clock_ = clock;
  apu_   = apu;
  cpu_   = cpu;
  ppu_   = ppu;
  joy_1_ = joy_1;
  joy_2_ = joy_2;
}

void hw::system_bus::SystemBus::loadCart(mapper::Mapper* mapper, uint8_t* prg_rom, uint8_t* expansion_ram) {
  mapper_        = mapper;
  prg_rom_       = prg_rom;
  expansion_ram_ = expansion_ram;
}


bool hw::system_bus::SystemBus::hasIRQ() const {
  return mapper_->hasIRQ() || apu_->hasIRQ();
}

bool hw::system_bus::SystemBus::hasNMI() const {
  return ppu_->hasNMI();
}

uint8_t hw::system_bus::SystemBus::read(uint16_t address) const {
  static uint8_t data;
  address &= 0xFFFF;

  if (address < 0x2000) {  // Stack and RAM
    data = ram_[address & 0x07FF];
  }

  else if (address < 0x4000) {  // PPU Registers
    data = ppu_->readRegister((address & 0x0007) | 0x2000);
  }

  else if (address < 0x4014) {  // Sound Registers
    // Open bus, these registers are write-only
  }

  else if (address == 0x4014) {  // PPU DMA Access
    // Open bus, cannot read DMA register
  }

  else if (address == 0x4015) {  // Sound Channel Switch
    data = apu_->readRegister(address);
  }

  else if (address == 0x4016) {  // Joystick 1
    data = joy_1_->read();
  }

  else if (address == 0x4017) {  // Joystick 2
    data = joy_2_->read();
  }

  else if (address < 0x4100) {  // Unallocated I/O space
    // Open bus
  }

  else if (address < 0x6000) {  // Expansion Modules, ie. Famicom Disk System
    // Open bus
  }

  else if (address < 0x8000) {  // Cartridge RAM
    if (expansion_ram_) {
      data = expansion_ram_[address - 0x6000];
    } else {
      // No cartridge RAM
      // Depending on the mapper, this should actually be open bus sometimes
      data = 0;
    }
  }

  else {  // Cartridge ROM
    data = prg_rom_[mapper_->decodeCPUAddress(address)];
  }

  logger::log<logger::DEBUG_BUS>("Read $%02X from $(%04X)\n", data, address);
  return data;
}


void hw::system_bus::SystemBus::write(uint16_t address, uint8_t data) {
  logger::log<logger::DEBUG_BUS>("Write $%02X to $(%04X)\n", data, address);
  address &= 0xFFFF;

  if (address < 0x2000) {  // Stack and RAM
    ram_[address & 0x07FF] = data;
  }

  else if (address < 0x4000) {  // PPU Registers
    ppu_->writeRegister((address & 0x0007) | 0x2000, data);
  }

  else if (address < 0x4014) {  // Sound Registers
    apu_->writeRegister(address, data);
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
    apu_->writeRegister(address, data);
  }

  else if (address == 0x4016) {  // Joystick Strobe
    joy_1_->write(data);
    joy_2_->write(data);
  }

  else if (address == 0x4017) {  // APU frame counter
    apu_->writeRegister(address, data);
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

void hw::system_bus::SystemBus::clock() {
  mapper_->clock();
  ppu_->clock();
  ppu_->clock();
  ppu_->clock();
  apu_->clock();
  // Note: Do not clock the CPU - This function is clocked by the CPU itself

  clock_->sleep();
}
