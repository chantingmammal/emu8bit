#include <nesemu/ppu.h>

#include <cstring>  // For memcpy
#include <iostream>


// =*=*=*=*= PPU Setup =*=*=*=*=

void ppu::PPU::loadCart(uint8_t* chr_mem, bool is_ram, Mirroring mirror) {
  chr_mem_        = chr_mem;
  chr_mem_is_ram_ = is_ram;
  mirroring_      = mirror;
}

void ppu::PPU::setPixelPtr(void* pixels) {
  pixels_ = pixels;
}

void ppu::PPU::setUpdateScreenPtr(std::function<void(void)> func) {
  update_screen_ = func;
}

void ppu::PPU::setTriggerVBlankPtr(std::function<void(void)> func) {
  trigger_vblank_ = func;
}


// =*=*=*=*= PPU Execution =*=*=*=*=

void ppu::PPU::tick() {
  uint16_t scanline_length = 341;

  if (scanline_ < 240) {  // Visible scanlines
    status_reg_.vblank = false;

  } else if (scanline_ < 241) {  // Post-render scanline (Idle)

  } else if (scanline_ < 261) {  // Vertical blanking scanlines
    if (scanline_ == 241 && cycle_ == 1) {
      status_reg_.vblank = true;
      if (ctrl_reg_1_.vblank_enable)
        trigger_vblank_();
    }
  } else {  // Pre-render scanline
    if (frame_is_odd_)
      scanline_length = 340;
  }


  cycle_++;
  if (cycle_ > scanline_length) {
    scanline_ = (scanline_ + 1) % 262;
    cycle_    = 0;
  }
}

uint8_t ppu::PPU::readRegister(uint16_t cpu_address) {
  switch (cpu_address) {
    case (0x2000):  // PPU Control Register 1
      return ctrl_reg_1_.raw;

    case (0x2001):  // PPU Control Register 2
      return ctrl_reg_2_.raw;

    case (0x2002): {  // PPU Status Register
      const uint8_t reg  = status_reg_.raw;
      status_reg_.vblank = false;
      return reg;
    }

    case (0x2003):  // Sprite Memory Address (Write-only)
      return 0;

    case (0x2004):  // Sprite Memory Data
      sprite_mem_addr_++;
      return spr_ram_[sprite_mem_addr_];

    case (0x2005):  // Screen Scroll Offsets (Write-only)
      return 0;

    case (0x2006):  // PPU Memory Address (Write-only)
      return 0;

    case (0x2007): {  // PPU Memory Data
      const uint8_t value = readByte(ppu_mem_addr_);
      ppu_mem_addr_ += (ctrl_reg_1_.vertical_write ? 32 : 1);
      return value;
    }

    default:  // Unknown register!
      return 0;
  }
}

void ppu::PPU::writeRegister(uint16_t cpu_address, uint8_t data) {
  static bool ppu_mem_addr_lower = true;
  static bool scroll_offset_vert = true;

  switch (cpu_address) {
    case (0x2000):  // PPU Control Register 1
      ctrl_reg_1_.raw = data;
      break;

    case (0x2001):  // PPU Control Register 2
      ctrl_reg_2_.raw = data;
      break;

    case (0x2002):  // PPU Status Register (Read-only)
      break;

    case (0x2003):  // Sprite Memory Address
      sprite_mem_addr_ = data;
      break;

    case (0x2004):  // Sprite Memory Data
      sprite_mem_addr_++;
      spr_ram_[sprite_mem_addr_] = data;
      break;

    case (0x2005):  // Screen Scroll Offsets
      if (scroll_offset_vert) {
        if (data <= 239)
          vert_scroll_offset_ = data;
      } else {
        horiz_scroll_offset_ = data;
      }
      scroll_offset_vert != scroll_offset_vert;
      break;

    case (0x2006):  // PPU Memory Address
      if (ppu_mem_addr_lower) {
        ppu_mem_addr_ &= 0xFF00;
        ppu_mem_addr_ |= data;
      } else {
        ppu_mem_addr_ &= 0x00FF;
        ppu_mem_addr_ |= (data << 8);
      }
      ppu_mem_addr_lower != ppu_mem_addr_lower;
      break;

    case (0x2007):  // PPU Memory Data
      writeByte(ppu_mem_addr_, data);
      ppu_mem_addr_ += (ctrl_reg_1_.vertical_write ? 32 : 1);
      break;

    default:  // Unknown register!
      break;
  }
}

void ppu::PPU::spriteDMAWrite(uint8_t* data) {
  memcpy(spr_ram_, data, 256);
}


// =*=*=*=*= PPU Internal Operations =*=*=*=*=

uint8_t ppu::PPU::readByte(uint16_t address) {
#if DEBUG
  const uint8_t data = readByteInternal(address);
  std::cout << "Read PPU" << unsigned(data) << " from $(" << address << ")\n";
  return data;
}

uint8_t ppu::PPU::readByteInternal(uint16_t address) {
#endif
  address &= 0x3FFF;

  // Cartridge VRAM/VROM
  if (address < 0x2000) {
    return chr_mem_[address];
  }

  // Nametables
  else if (address < 0x3000) {
    address = ((address - 0x2000) & 0x3FFF) | (0x400 * ctrl_reg_1_.nametable_address);
    switch (mirroring_) {
      case Mirroring::none:  // Four-screen VRAM layout
        return ram_[address];
      case Mirroring::vertical:  // Vertical mirroring_
        return ram_[address & ~0x800];
      case Mirroring::horizontal:  // Horizontal mirroring_
        return ram_[address & ~0x400];
    }
  }

  // Palettes
  else {
    return ram_[(address - 0x2000)];
  }
}

void ppu::PPU::writeByte(uint16_t address, uint8_t data) {
#if DEBUG
  std::cout << std::hex << "Write PPU ($" << unsigned(address) << ")=" << unsigned(data) << "\n";
#endif
  address &= 0x3FFF;

  // Cartridge VRAM/VROM
  if (address < 0x2000) {
    if (chr_mem_is_ram_)  // Uses VRAM, not VROM
      chr_mem_[address] = data;
  }

  // Nametables
  else if (address < 0x3000) {
    address = ((address - 0x2000) & 0x3FFF) | (0x400 * ctrl_reg_1_.nametable_address);

    switch (mirroring_) {
      case Mirroring::none:  // Four-screen VRAM layout
        ram_[address] = data;
        break;
      case Mirroring::vertical:  // Vertical mirroring_
        ram_[address & ~0x800] = data;
        break;
      case Mirroring::horizontal:  // Horizontal mirroring_
        ram_[address & ~0x400] = data;
        break;
    }
  }

  // Palettes
  else {
    ram_[(address - 0x2000)] = data;
  }
}
