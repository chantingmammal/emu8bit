#include <nesemu/debug.h>
#include <nesemu/ppu.h>
#include <nesemu/temp_mapping.h>

#include <cstring>  // For memcpy
#include <iostream>


// =*=*=*=*= PPU Setup =*=*=*=*=

void ppu::PPU::loadCart(uint8_t* chr_mem, bool is_ram, Mirroring mirror) {
  chr_mem_        = chr_mem;
  chr_mem_is_ram_ = is_ram;
  mirroring_      = mirror;
}

void ppu::PPU::setPixelPtr(void* pixels) {
  pixels_ = static_cast<uint32_t*>(pixels);
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


  // =*=*=*=*= Visible scanline =*=*=*=*=
  if (scanline_ < 240) {
    fetchTilesAndSprites(true);

    // Cycles 0-255: Render
    if (cycle_ < 256 && ctrl_reg_2_.render_enable) {
      renderPixel();
    }
  }

  // =*=*=*=*=  Post-render scanline (Idle) =*=*=*=*=
  else if (scanline_ < 241) {
    if (cycle_ == 0) {
      update_screen_();
    }
  }


  // =*=*=*=*=  Vertical blanking scanlines =*=*=*=*=
  else if (scanline_ < 261) {
    if (scanline_ == 241 && cycle_ == 1) {
      status_reg_.vblank = true;
      if (ctrl_reg_1_.vblank_enable) {
        trigger_vblank_();
      }
    }
  }


  // =*=*=*=*= Pre-render scanline =*=*=*=*=
  else {
    if (cycle_ == 1) {
      status_reg_.vblank = false;
    }

    // On odd frames, shorted pre_render scanline by 1
    if (frame_is_odd_) {
      scanline_length = 340;
    }

    if (280 <= cycle_ && cycle_ <= 304 && ctrl_reg_2_.render_enable) {
      v_.coarse_y_scroll  = t_.coarse_y_scroll;
      v_.nametable_select = (v_.nametable_select & 0x01) | (t_.nametable_select & 0x02);
      v_.fine_y_scroll    = t_.fine_y_scroll;
    }

    fetchTilesAndSprites(false);
  }


  // Cycle & scanline increment
  cycle_++;
  if (cycle_ >= scanline_length) {
    scanline_ = (scanline_ + 1) % 262;
    cycle_    = 0;
  }
}

uint8_t ppu::PPU::readRegister(uint16_t cpu_address) {
  switch (cpu_address) {
    case (0x2000):  // PPU Control Register 1
      return ctrl_reg_1_.raw | v_.nametable_select;

    case (0x2001):  // PPU Control Register 2
      return ctrl_reg_2_.raw;

    case (0x2002): {  // PPU Status Register
      const uint8_t reg  = status_reg_.raw;
      status_reg_.vblank = false;
      write_toggle_      = false;
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
      const uint8_t value = readByte(v_.raw);
      if ((scanline_ == 261 || scanline_ < 256) && ctrl_reg_2_.render_enable) {
        incrementCoarseX();
        incrementFineY();
      } else {
        v_.raw += (ctrl_reg_1_.vertical_write ? 32 : 1);
      }
      return value;
    }

    default:  // Unknown register!
      return 0;
  }
}

void ppu::PPU::writeRegister(uint16_t cpu_address, uint8_t data) {
  switch (cpu_address) {
    case (0x2000):  // PPU Control Register 1
      ctrl_reg_1_.raw     = data;
      t_.nametable_select = data & 0x03;
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
      if (write_toggle_) {
        t_.fine_y_scroll   = data & 0x07;
        t_.coarse_y_scroll = data >> 3;
      } else {
        fine_x_scroll_     = data & 0x07;
        t_.coarse_x_scroll = data >> 3;
      }
      write_toggle_ = !write_toggle_;
      break;

    case (0x2006):          // PPU Memory Address
      if (write_toggle_) {  // Write lower byte on second write
        t_.lower = data;
        v_.raw   = t_.raw;
      } else {  // Write upper byte on first write
        t_.upper = data & 0x3F;
      }
      write_toggle_ = !write_toggle_;
      break;

    case (0x2007):  // PPU Memory Data
      writeByte(v_.raw, data);
      if ((scanline_ == 261 || scanline_ < 256) && ctrl_reg_2_.render_enable) {
        incrementCoarseX();
        incrementFineY();
      } else {
        v_.raw += (ctrl_reg_1_.vertical_write ? 32 : 1);
      }
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
  else if (address < 0x3F00) {
    address = (address & 0x0FFF) | (0x400 * v_.nametable_select);
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

    // TODO: Hackey
    switch (address) {
      case 0x3F10:
      case 0x3F14:
      case 0x3F18:
      case 0x3F1C:
        address &= ~0x0010;
    }

    return ram_[(address & 0x1F) + 0x1F00] & (ctrl_reg_2_.greyscale ? 0x30 : 0xFF);
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
  else if (address < 0x3F00) {
    address = ((address - 0x2000) & 0x0FFF) | (0x400 * v_.nametable_select);

    // std::cout << "Write NT " << address << " = " << unsigned(data) << std::endl;

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

    // TODO: Hackey
    switch (address) {
      case 0x3F10:
      case 0x3F14:
      case 0x3F18:
      case 0x3F1C:
        address &= ~0x0010;
    }

    // std::cout << "Write palette " << address << " = " << unsigned(data) << std::endl;
    ram_[(address & 0x1F) + 0x1F00] = data;
  }
}

void ppu::PPU::renderPixel() {

  // Debugging dumps:
  // dump_nt(ram_);
  // dump_patterns(chr_mem_ + (ctrl_reg_1_.screen_pattern_table_addr << 12));
  // print_bg(ram_, chr_mem_ + (ctrl_reg_1_.screen_pattern_table_addr << 12));
  // dump(ram_ + 0x1F00, 16);


  // TODO: Image mask and sprite mask

  uint32_t* pixel = pixels_ + (scanline_ * 256) + cycle_;

  uint8_t bg_pixel = ((pattern_sr_a_ >> (15 - fine_x_scroll_)) & 0x01)
                     | ((pattern_sr_b_ >> (14 - fine_x_scroll_)) & 0x02);
  const uint8_t sprite_pixel    = 0;
  const uint8_t sprite_priority = 0;

  uint8_t palette_addr;
  // 43210
  // |||||
  // |||++- Pixel value from tile data
  // |++--- Palette number from attribute table or OAM
  // +----- Background/Sprite select

  // Determine address to read color for current pixel
  // TODO: Screen and sprite enable
  if (bg_pixel == 0 && sprite_pixel == 0) {
    palette_addr = 0;
  } else if (sprite_pixel == 0 || sprite_priority == 1) {
    const uint8_t bg_palette = ((palette_sr_a_ >> (7 - fine_x_scroll_)) & 0x01)
                               | ((palette_sr_b_ >> (6 - fine_x_scroll_)) & 0x02);
    palette_addr = bg_pixel | (bg_palette << 2);
  } else {
    palette_addr = 0;  // TODO
  }


  // Display the pixel
  *pixel = decodeColor(readByte(0x3F00 | palette_addr));

  // Shift SRs left
  pattern_sr_a_ <<= 1;
  pattern_sr_b_ <<= 1;
  palette_sr_a_ <<= 1;
  palette_sr_b_ <<= 1;

  // Load palette SRs from latch
  palette_sr_a_ |= palette_latch_a_;
  palette_sr_b_ |= palette_latch_b_;
}

void ppu::PPU::fetchTilesAndSprites(bool fetch_sprites) {

  // Cycles 1-256: Fetch background tiles
  if (0 < cycle_ && cycle_ < 257) {
    if ((cycle_ % 8) == 0) {
      fetchNextBGTile();
    }
  }

  // Cycles 257-320: Fetch sprites for the next scanline
  else if (cycle_ < 321) {
    if (cycle_ == 257 && ctrl_reg_2_.render_enable) {
      v_.coarse_x_scroll  = t_.coarse_x_scroll;
      v_.nametable_select = (t_.nametable_select & 0x01) | (v_.nametable_select & 0x02);
    }

    if (fetch_sprites) {
      // TODO
    }
  }

  // Cycles 321-336: Fetch first two background tiles for next scanline
  else if (cycle_ < 337) {

    // Load tile 0
    if (cycle_ == 328) {
      fetchNextBGTile();
      pattern_sr_a_ <<= 8;
      pattern_sr_b_ <<= 8;
      palette_sr_a_ = palette_latch_a_ ? -1 : 0;
      palette_sr_b_ = palette_latch_b_ ? -1 : 0;
    }

    // Load tile 1
    else if (cycle_ == 336) {
      fetchNextBGTile();
    }
  }

  // Cycles 337-340: 2 extra fetches to background tile 2 of next scanline
  else {
    if (cycle_ % 2) {
      readByte(0x2000 | (v_.raw & 0x0FFF));
    }
  }
}

void ppu::PPU::fetchNextBGTile() {
  const uint16_t pattern_addr = (readByte(0x2000 | (v_.raw & 0x0FFF)) << 4)    // Base tile address
                                | ctrl_reg_1_.screen_pattern_table_addr << 12  // Pattern table
                                | v_.fine_y_scroll;                            // Y offset (Tile slice)

  const uint16_t at_entry = (v_.coarse_x_scroll >> 2)                         // Coarse X / 4
                            | ((v_.coarse_y_scroll & 0x1C) << 1)              // Coarse Y / 4
                            | (ctrl_reg_1_.screen_pattern_table_addr << 12);  // Pattern table

  const uint8_t at_subentry = (v_.coarse_x_scroll & 2) | ((v_.coarse_y_scroll & 2) << 1);
  // 76543210
  // ||||||||
  // ||||||++- Top left     (0)
  // ||||++--- Top right    (2)
  // ||++----- Bottom left  (4)
  // ++------- Bottom right (6)

  pattern_sr_a_ &= 0xFF00;
  pattern_sr_b_ &= 0xFF00;
  pattern_sr_a_ |= readByte(pattern_addr);
  pattern_sr_b_ |= readByte(pattern_addr | 8);

  const uint8_t palette = readByte(0x23C0 + at_entry);
  palette_latch_a_      = (palette >> at_subentry) & 0x01;
  palette_latch_b_      = (palette >> (at_subentry | 1)) & 0x01;


  // Coarse X increment, at end of each tile
  incrementCoarseX();

  // Fine Y increment at the end of each line
  if (cycle_ == 256) {
    incrementFineY();
  }
}

void ppu::PPU::incrementCoarseX() {
  if (ctrl_reg_2_.render_enable) {
    v_.coarse_x_scroll += 1;
    if (v_.coarse_x_scroll == 0) {                       // Overflow
      v_.nametable_select = v_.nametable_select ^ 0x01;  // Switch horiz nametable
    }
  }
}

void ppu::PPU::incrementFineY() {
  if (ctrl_reg_2_.render_enable) {
    v_.fine_y_scroll += 1;
    if (v_.fine_y_scroll == 0) {  // Overflow
      if (v_.coarse_y_scroll == 29) {
        v_.coarse_y_scroll  = 0;
        v_.nametable_select = v_.nametable_select ^ 0x02;  // Switch vert nametable
      } else {
        v_.coarse_y_scroll += 1;
      }
    }
  }
}
