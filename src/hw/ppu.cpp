#include <nesemu/hw/ppu.h>

#include <nesemu/debug.h>
#include <nesemu/hw/mapper/mapper_base.h>
#include <nesemu/logger.h>
#include <nesemu/temp_mapping.h>
#include <nesemu/ui/screen.h>
#include <nesemu/utils/enum.h>

#include <cstring>  // For memcpy


// =*=*=*=*= PPU Setup =*=*=*=*=

void hw::ppu::PPU::loadCart(mapper::Mapper* mapper, uint8_t* chr_mem, bool is_ram) {
  mapper_         = mapper;
  chr_mem_        = chr_mem;
  chr_mem_is_ram_ = is_ram;
}

void hw::ppu::PPU::setScreen(ui::Screen* screen) {
  screen_ = screen;
}


// =*=*=*=*= PPU Execution =*=*=*=*=

bool hw::ppu::PPU::hasNMI() {
  return (vblank_suppression_counter_ == 0) && status_reg_.vblank && ctrl_reg_1_.vblank_enable;
}

void hw::ppu::PPU::clock() {
  uint16_t scanline_length = 341;


  // =*=*=*=*= Visible scanline =*=*=*=*=
  if (scanline_ < 240) {
    fetchTilesAndSprites(true);

    // Cycles 0-255: Render
    if (cycle_ < 256) {
      renderPixel();
    }
  }

  // =*=*=*=*=  Post-render scanline (Idle) =*=*=*=*=
  else if (scanline_ < 241) {
    if (cycle_ == 0) {
      screen_->update(pixels_);
    }
  }


  // =*=*=*=*=  Vertical blanking scanlines =*=*=*=*=
  else if (scanline_ < 261) {
    if (scanline_ == 241 && cycle_ == 1 && vblank_suppression_counter_ == 0) {
      status_reg_.vblank = true;
    }
  }


  // =*=*=*=*= Pre-render scanline =*=*=*=*=
  else {
    if (cycle_ == 1) {
      did_hit_sprite_zero_ = false;
      status_reg_.overflow = false;
      status_reg_.hit      = false;
      status_reg_.vblank   = false;
    }

    // On odd frames, shorted pre_render scanline by 1
    if (frame_is_odd_ && ctrl_reg_2_.render_enable) {
      scanline_length = 340;
    }

    if (280 <= cycle_ && cycle_ <= 304 && ctrl_reg_2_.render_enable) {
      v_.coarse_y_scroll  = t_.coarse_y_scroll;
      v_.nametable_select = (v_.nametable_select & 0x01) | (t_.nametable_select & 0x02);
      v_.fine_y_scroll    = t_.fine_y_scroll;
      mapper_->decodePPUAddress(v_.raw);
    }

    fetchTilesAndSprites(false);
  }


  if (vblank_suppression_counter_ > 0) {
    vblank_suppression_counter_--;
    if (vblank_suppression_counter_ == 0) {
      status_reg_.vblank = false;
    }
  }

  // Cycle & scanline increment
  cycle_++;
  if (cycle_ >= scanline_length) {
    scanline_++;
    if (scanline_ >= 262) {
      frame_is_odd_ = !frame_is_odd_;
    }
    scanline_ %= 262;
    cycle_ = 0;
  }
}

uint8_t hw::ppu::PPU::readRegister(uint16_t cpu_address) {
  switch (cpu_address) {
    case (utils::asInt(MemoryMappedIO::PPUCTRL)):  // PPU Control Register 1 (Write-only)
      logger::log<logger::DEBUG_PPU>("Read $%02X from PPUCTRL (Open bus)\n", io_latch_);
      break;

    case (utils::asInt(MemoryMappedIO::PPUMASK)):  // PPU Control Register 2 (Write-only)
      logger::log<logger::DEBUG_PPU>("Read $%02X from PPUMASK (Open bus)\n", io_latch_);
      break;

    case (utils::asInt(MemoryMappedIO::PPUSTATUS)): {  // PPU Status Register
      io_latch_                   = status_reg_.raw;
      status_reg_.vblank          = false;
      write_toggle_               = false;
      vblank_suppression_counter_ = 2;
      logger::log<logger::DEBUG_PPU>("Read $%02X from PPUSTATUS\n", io_latch_);
    } break;

    case (utils::asInt(MemoryMappedIO::OAMADDR)):  // Sprite Memory Address (Write-only)
      logger::log<logger::DEBUG_PPU>("Read $%02X from OAMADDR (Open bus)\n", io_latch_);
      break;

    case (utils::asInt(MemoryMappedIO::OAMDATA)):  // Sprite Memory Data
      if (1 <= cycle_ && cycle_ <= 64) {
        io_latch_ = 0xFF;
      } else {
        io_latch_ = primary_oam_.byte[oam_addr_];
      }
      logger::log<logger::DEBUG_PPU>("Read $%02X from OAMDATA[$%02X]\n", io_latch_, oam_addr_);
      break;

    case (utils::asInt(MemoryMappedIO::PPUSCROLL)):  // Screen Scroll Offsets (Write-only)
      logger::log<logger::DEBUG_PPU>("Read $%02X from PPUSCROLL (Open bus)\n", io_latch_);
      break;

    case (utils::asInt(MemoryMappedIO::PPUADDR)):  // PPU Memory Address (Write-only)
      logger::log<logger::DEBUG_PPU>("Read $%02X from PPUADDR (Open bus)\n", io_latch_);
      break;

    case (utils::asInt(MemoryMappedIO::PPUDATA)): {  // PPU Memory Data
      static uint8_t read_buffer;
      if (v_.raw < 0x3F00) {
        io_latch_   = read_buffer;
        read_buffer = readByte(v_.raw);
      } else {
        // TODO: Double check this buffer
        read_buffer = readByte(0x2000 | (v_.raw & 0x0FFF));
        io_latch_   = readByte(v_.raw);
      }

      if ((scanline_ == 261 || scanline_ < 240) && ctrl_reg_2_.render_enable) {
        incrementCoarseX();
        incrementFineY();
      } else {
        v_.raw += (ctrl_reg_1_.vertical_write ? 32 : 1);
        mapper_->decodePPUAddress(v_.raw);
      }

      logger::log<logger::DEBUG_PPU>("Read $%02X from PPUDATA\n", io_latch_);
    } break;

    default:
      logger::log<logger::ERROR>("Attempted to read invalid PPU addr $%02X\n", cpu_address);
      break;
  }

  return io_latch_;
}

void hw::ppu::PPU::writeRegister(uint16_t cpu_address, uint8_t data) {
  io_latch_ = data;

  switch (cpu_address) {
    case (utils::asInt(MemoryMappedIO::PPUCTRL)):  // PPU Control Register 1
      ctrl_reg_1_.raw     = data;
      t_.nametable_select = data & 0x03;
      logger::log<logger::DEBUG_PPU>("Set PPUCTRL = $%02X\n", data);
      break;

    case (utils::asInt(MemoryMappedIO::PPUMASK)):  // PPU Control Register 2
      ctrl_reg_2_.raw = data;
      logger::log<logger::DEBUG_PPU>("Set PPUMASK = $%02X\n", data);
      break;

    case (utils::asInt(MemoryMappedIO::PPUSTATUS)):  // PPU Status Register (Read-only)
      logger::log<logger::DEBUG_PPU>("Set PPUSTATUS = $%02X (Open bus)\n", data);
      break;

    case (utils::asInt(MemoryMappedIO::OAMADDR)):  // Sprite Memory Address
      oam_addr_ = data;
      logger::log<logger::DEBUG_PPU>("Set OAMADDR = $%02X\n", data);
      break;

    case (utils::asInt(MemoryMappedIO::OAMDATA)):  // Sprite Memory Data
      logger::log<logger::DEBUG_PPU>("Set OAMDATA[$%02X] to $%02X\n", oam_addr_, io_latch_);
      primary_oam_.byte[oam_addr_] = data;
      oam_addr_++;
      break;

    case (utils::asInt(MemoryMappedIO::PPUSCROLL)):  // Screen Scroll Offsets
      if (write_toggle_) {
        t_.fine_y_scroll   = data & 0x07;
        t_.coarse_y_scroll = data >> 3;
      } else {
        fine_x_scroll_     = data & 0x07;
        t_.coarse_x_scroll = data >> 3;
      }
      write_toggle_ = !write_toggle_;
      logger::log<logger::DEBUG_PPU>("Set PPUSCROLL = $%02X\n", data);
      break;

    case (utils::asInt(MemoryMappedIO::PPUADDR)):  // PPU Memory Address
      if (write_toggle_) {                         // Write lower byte on second write
        t_.lower = data;
        v_.raw   = t_.raw;
        mapper_->decodePPUAddress(v_.raw);
      } else {  // Write upper byte on first write
        t_.upper = data & 0x3F;
      }
      write_toggle_ = !write_toggle_;
      logger::log<logger::DEBUG_PPU>("Set PPUADDR = $%02X\n", data);
      break;

    case (utils::asInt(MemoryMappedIO::PPUDATA)):  // PPU Memory Data
      writeByte(v_.raw, data);
      if ((scanline_ == 261 || scanline_ < 240) && ctrl_reg_2_.render_enable) {
        incrementCoarseX();
        incrementFineY();
      } else {
        v_.raw += (ctrl_reg_1_.vertical_write ? 32 : 1);
        mapper_->decodePPUAddress(v_.raw);
      }
      logger::log<logger::DEBUG_PPU>("Set PPUDATA = $%02X\n", data);
      break;

    default:
      logger::log<logger::ERROR>("Attempted to write invalid PPU reg $%02X\n", cpu_address);
      break;
  }
}

void hw::ppu::PPU::spriteDMAWrite(uint8_t* data) {
  memcpy(primary_oam_.byte + oam_addr_, data, 256 - oam_addr_);
  memcpy(primary_oam_.byte, data + 256 - oam_addr_, oam_addr_);
}


// =*=*=*=*= PPU Internal Operations =*=*=*=*=

uint8_t hw::ppu::PPU::readByte(uint16_t address) const {
  uint8_t data;
  address &= 0x3FFF;

  // Cartridge VRAM/VROM
  if (address < 0x2000) {
    data = chr_mem_[mapper_->decodePPUAddress(address)];
  }

  // Nametables & palettes
  else {
    data = ram_[mapper_->decodeCIRAMAddress(address)];
  }

  return data;
}


void hw::ppu::PPU::writeByte(uint16_t address, uint8_t data) {
  address &= 0x3FFF;

  // Cartridge VRAM/VROM
  if (address < 0x2000) {
    if (chr_mem_is_ram_)  // Uses VRAM, not VROM
      chr_mem_[mapper_->decodePPUAddress(address)] = data;
  }

  // Nametables & palettes
  else {
    ram_[mapper_->decodeCIRAMAddress(address)] = data;
  }
}

void hw::ppu::PPU::renderPixel() {

  // Determine background pixel and palette
  uint8_t bg_pixel = ((pattern_sr_a_ >> (15 - fine_x_scroll_)) & 0x01)
                     | (((pattern_sr_b_ >> (15 - fine_x_scroll_)) << 1) & 0x02);
  const uint8_t bg_palette = ((palette_sr_a_ >> (7 - fine_x_scroll_)) & 0x01)
                             | (((palette_sr_b_ >> (7 - fine_x_scroll_)) << 1) & 0x02);

  // If background renderind is disabled or left column is masked, set pixel to 0
  if (!ctrl_reg_2_.bg_enable || (cycle_ < 8 && !ctrl_reg_2_.bg_mask)) {
    bg_pixel = 0;
  }

  uint8_t sprite_pixel    = 0;
  uint8_t sprite_palette  = 0;
  uint8_t sprite_priority = 0;
  for (unsigned i = 0; i < 8; i++) {

    // Decrement x position until cur dot intersects sprite
    if (sprite_x_position_[i] != 0) {
      sprite_x_position_[i]--;
    }

    // Otherwise, determine the pixel of the sprite
    else {
      const bool pixel_found = sprite_pixel != 0;

      if (sprite_palette_latch_[i].flip_horiz) {

        // Skip if pixel already found, or if sprite rendering is disabled or left column is masked
        if (!pixel_found && ctrl_reg_2_.sprite_enable && (cycle_ > 7 || ctrl_reg_2_.sprite_mask)) {
          sprite_pixel = (sprite_pattern_sr_a_[i] & 0x01) | ((sprite_pattern_sr_b_[i] << 1) & 0x02);
        }
        sprite_pattern_sr_a_[i] >>= 1;
        sprite_pattern_sr_b_[i] >>= 1;
      } else {

        // Skip if pixel already found, or if sprite rendering is disabled or left column is masked
        if (!pixel_found && ctrl_reg_2_.sprite_enable && (cycle_ > 7 || ctrl_reg_2_.sprite_mask)) {
          sprite_pixel = ((sprite_pattern_sr_a_[i] >> 7) & 0x01) | ((sprite_pattern_sr_b_[i] >> 6) & 0x02);
        }
        sprite_pattern_sr_a_[i] <<= 1;
        sprite_pattern_sr_b_[i] <<= 1;
      }

      // Sprite zero hit
      // if (i == 0 && sr_has_sprite_zero_ && !did_hit_sprite_zero_) {
      //   printf("s=%d\tc=%d\tbg_pixel=%d, sprite_pixel=%d\n", scanline_, cycle_, bg_pixel, sprite_pixel);
      // }
      if (i == 0 && sr_has_sprite_zero_          // Sprite is #0
          && cycle_ != 255                       // Not right-most pixel
          && bg_pixel != 0 && sprite_pixel != 0  // Both BG and sprite are opaque
          && !did_hit_sprite_zero_) {            // Hasn't already hit this frame
        // printf("Hit, S=%d\n", scanline_);
        did_hit_sprite_zero_ = true;
        status_reg_.hit      = true;
      }

      if (!pixel_found) {
        sprite_palette  = sprite_palette_latch_[i].palette;
        sprite_priority = sprite_palette_latch_[i].priority;
      }
    }
  }

  uint8_t palette_addr;
  // 43210
  // |||||
  // |||++- Pixel value from tile data
  // |++--- Palette number from attribute table or OAM
  // +----- Background/Sprite select

  // Determine address to read color for current pixel
  // TODO: If bg is disabled, render black, not bg palette?
  if (bg_pixel == 0 && sprite_pixel == 0) {
    palette_addr = 0;
  } else if (sprite_pixel == 0 || (bg_pixel != 0 && sprite_priority == 1)) {
    palette_addr = bg_pixel | (bg_palette << 2);
  } else {
    palette_addr = sprite_pixel | (sprite_palette << 2) | 0x10;
  }


  // Write the pixel to the screen buffer
  const uint8_t color                 = readByte(0x3F00 | palette_addr) & (ctrl_reg_2_.greyscale ? 0x30 : 0xFF);
  pixels_[(scanline_ * 256) + cycle_] = decodeColor(color);

  // Shift SRs left
  pattern_sr_a_ <<= 1;
  pattern_sr_b_ <<= 1;
  palette_sr_a_ <<= 1;
  palette_sr_b_ <<= 1;

  // Load palette SRs from latch
  palette_sr_a_ |= static_cast<uint8_t>(palette_latch_a_);
  palette_sr_b_ |= static_cast<uint8_t>(palette_latch_b_);
}

void hw::ppu::PPU::fetchTilesAndSprites(bool fetch_sprites) {

  // Sprite evaluation fsm
  SpriteEvaluationFSM& fsm = sprite_eval_fsm_;

  // Cycle 0: Idle
  if (cycle_ < 1) {

    // Initialize Sprite Evaluation state machine
    fsm.state_      = SpriteEvaluationFSM::State::CHECK_Y_IN_RANGE;
    fsm.poam_index_ = 0;
    fsm.soam_index_ = 0;
    fsm.initialize_ = true;

    // Save sprite_zero status
    sr_has_sprite_zero_  = oam_has_sprite_zero_;
    oam_has_sprite_zero_ = false;
  }

  // Cycles 1-64: Background: Fetch tiles
  //              Sprite:     Clear secondary OAM
  else if (cycle_ < 65) {
    if (ctrl_reg_2_.render_enable && (cycle_ % 8) == 0) {
      fetchNextBGTile();
    }

    // Clear out the SOAM
    if ((cycle_ % 2) == 1) {
      fsm.latch_ = 0xFF;
    } else if (fsm.initialize_) {
      secondary_oam_.byte[fsm.soam_index_++] = fsm.latch_;
      if (fsm.soam_index_ == 32) {
        fsm.initialize_ = false;
        fsm.soam_index_ = 0;
      }
    }
  }


  // Cycles 65-256: Background: Fetch tiles
  //                Sprite:     Sprite evaluation
  else if (cycle_ < 257) {
    if (ctrl_reg_2_.render_enable) {
      if ((cycle_ % 8) == 0) {
        fetchNextBGTile();
      }

      const uint8_t sprite_height = ctrl_reg_1_.large_sprites ? 16 : 8;

      if ((cycle_ % 2) == 1) {
        fsm.latch_ = primary_oam_.byte[fsm.poam_index_];
      } else {

        switch (fsm.state_) {
          using State = SpriteEvaluationFSM::State;

          // If Y val in range, copy over rest of sprite data into secondary OAM
          case State::CHECK_Y_IN_RANGE:
            secondary_oam_.byte[fsm.soam_index_] = fsm.latch_;
            if (fsm.latch_ <= scanline_ && uint8_t(fsm.latch_ + sprite_height) > scanline_) {
              if (fsm.poam_index_ == 0) {
                oam_has_sprite_zero_ = true;
              }
              // if (fsm.latch_ == 238 || fsm.latch_ == 239) {
              //   printf("Y=%d, S=%d\n", fsm.latch_, scanline_);
              // }
              fsm.state_         = State::COPY_SPRITE;
              fsm.state_counter_ = 3;
              fsm.soam_index_++;
              fsm.poam_index_++;
            } else {
              fsm.poam_index_ += 4;
              if (fsm.poam_index_ == 0) {
                fsm.state_ = State::DONE;
              }
            }
            break;

          // Copy sprite data into seconary OAM
          case State::COPY_SPRITE:
            secondary_oam_.byte[fsm.soam_index_++] = fsm.latch_;
            fsm.poam_index_++;

            fsm.state_counter_--;
            if (fsm.state_counter_ == 0) {

              // Primary OAM index overflowed, therefore all 64 sprites have been evaluated
              if (fsm.poam_index_ == 0) {
                fsm.state_ = State::DONE;
              }

              // Secondary OAM not yet full, so continue scanning through Primary OAM
              else if (fsm.soam_index_ < 32) {
                fsm.state_ = State::CHECK_Y_IN_RANGE;
              }

              // Secondary OAM is full, check for sprite overflow
              else {
                fsm.state_ = State::OVERFLOW;
              }
            }
            break;

          // Check if Y val in range
          case State::OVERFLOW:
            if (fsm.latch_ <= scanline_ && uint8_t(fsm.latch_ + sprite_height) > scanline_) {
              status_reg_.overflow = true;

              fsm.poam_index_++;
              fsm.state_         = State::DUMMY_READ;
              fsm.state_counter_ = 3;
            } else {
              fsm.poam_index_ += 4;
              if ((fsm.poam_index_ / 4) == 0) {
                fsm.state_ = State::DONE;
              } else {
                fsm.poam_index_ = (fsm.poam_index_ & 0xFC) | ((fsm.poam_index_ + 1) & 0x03);
              }
            }

            break;

          // Dummy read
          case State::DUMMY_READ:
            fsm.poam_index_++;
            fsm.state_counter_--;
            if (fsm.state_counter_ == 0) {
              fsm.state_ = State::OVERFLOW;
            }
            break;

          // Do nothing; Just read next sprite, and discard result
          case State::DONE:
            fsm.poam_index_ += 4;
            break;

            // No default
        }
      }
    }
  }

  // Cycles 257-320: Fetch sprites for the next scanline
  else if (cycle_ < 321) {
    if (cycle_ == 257) {
      if (ctrl_reg_2_.render_enable) {
        v_.coarse_x_scroll  = t_.coarse_x_scroll;
        v_.nametable_select = (t_.nametable_select & 0x01) | (v_.nametable_select & 0x02);
      }
      num_sprites_fetched_ = 0;

      // Clear old sprites
      for (unsigned i = 0; i < 8; i++) {
        sprite_pattern_sr_a_[i]      = 0;
        sprite_pattern_sr_b_[i]      = 0;
        sprite_palette_latch_[i].raw = 0;
        sprite_x_position_[i]        = 0;
      }
    }

    if (ctrl_reg_2_.render_enable && (cycle_ % 8) == 0) {
      fetchNextSprite();
    }
  }

  // Cycles 321-336: Fetch first two background tiles for next scanline
  else if (cycle_ < 337) {

    // Load tile 0
    if (ctrl_reg_2_.render_enable) {
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
  }

  // Cycles 337-340: 2 extra fetches to background tile 2 of next scanline
  else {
    if (cycle_ % 2) {
      readByte(0x2000 | (v_.raw & 0x0FFF));
    }
  }
}

void hw::ppu::PPU::fetchNextBGTile() {
  const uint16_t pattern_addr = (readByte(0x2000 | (v_.raw & 0x0FFF)) << 4)    // Base tile address
                                | ctrl_reg_1_.screen_pattern_table_addr << 12  // Pattern table
                                | v_.fine_y_scroll;                            // Y offset (Tile slice)

  const uint16_t at_entry = (v_.coarse_x_scroll >> 2)             // Coarse X / 4
                            | ((v_.coarse_y_scroll & 0x1C) << 1)  // Coarse Y / 4
                            | (v_.nametable_select << 10);        // Nametable select

  const uint8_t at_subentry = (v_.coarse_x_scroll & 2) | ((v_.coarse_y_scroll & 2) << 1);
  // 76543210
  // ||||||||
  // ||||||++- Top left     (0)
  // ||||++--- Top right    (2)
  // ||++----- Bottom left  (4)
  // ++------- Bottom right (6)

  const uint8_t palette = readByte(0x23C0 + at_entry);
  palette_latch_a_      = (palette >> at_subentry) & 0x01;
  palette_latch_b_      = (palette >> (at_subentry | 1)) & 0x01;

  pattern_sr_a_ &= 0xFF00;
  pattern_sr_b_ &= 0xFF00;
  pattern_sr_a_ |= readByte(pattern_addr);
  pattern_sr_b_ |= readByte(pattern_addr | 8);


  // Coarse X increment, at end of each tile
  incrementCoarseX();

  // Fine Y increment at the end of each line
  if (cycle_ == 256) {
    incrementFineY();
  }
}

void hw::ppu::PPU::fetchNextSprite() {
  if (num_sprites_fetched_ < (sprite_eval_fsm_.soam_index_ / 4)) {

    const Sprite& sprite = secondary_oam_.sprite[num_sprites_fetched_];
    uint8_t       tile_index;
    uint8_t       y_pos;

    // Large (8x16) sprites
    if (ctrl_reg_1_.large_sprites) {
      const bool top_tile   = scanline_ - sprite.y_position < 8;
      const bool first_tile = top_tile != static_cast<bool>(sprite.attributes.flip_vert);

      tile_index = sprite.large_tile_index * 2 + (first_tile ? 0 : 1);
      y_pos      = sprite.y_position + (top_tile ? 0 : 8);
    }

    // Small (8x8) sprites
    else {
      tile_index = sprite.small_tile_index;
      y_pos      = sprite.y_position;
    }

    const uint8_t  y_slice      = sprite.attributes.flip_vert                    // Horizontal slice of the sprite
                                      ? 7 - (scanline_ - y_pos)                  //   - Vertically flipped
                                      : scanline_ - y_pos;                       //   - Not flipped
    const uint16_t pattern_addr = tile_index << 4                                // Base tile address
                                  | ctrl_reg_1_.sprite_pattern_table_addr << 12  // Pattern table
                                  | y_slice;                                     // Horizontal slice

    readByte(0x2000 | (v_.raw & 0x0FFF));  // Garbage NT fetch
    readByte(0x2000 | (v_.raw & 0x0FFF));  // Garbage NT fetch
    sprite_pattern_sr_a_[num_sprites_fetched_]      = readByte(pattern_addr);
    sprite_pattern_sr_b_[num_sprites_fetched_]      = readByte(pattern_addr | 8);
    sprite_palette_latch_[num_sprites_fetched_].raw = sprite.attributes.raw;
    sprite_x_position_[num_sprites_fetched_]        = sprite.x_position;
  } else {
    const uint16_t pattern_addr = 0xFF << 4                                       // Base tile address
                                  | ctrl_reg_1_.sprite_pattern_table_addr << 12;  // Pattern table
    readByte(0x2000 | (v_.raw & 0x0FFF));                                         // Garbage NT fetch
    readByte(0x2000 | (v_.raw & 0x0FFF));                                         // Garbage NT fetch
    readByte(pattern_addr);
    readByte(pattern_addr | 8);
  }
  num_sprites_fetched_++;
}

void hw::ppu::PPU::incrementCoarseX() {
  if (ctrl_reg_2_.render_enable) {
    v_.coarse_x_scroll += 1;
    if (v_.coarse_x_scroll == 0) {                       // Overflow
      v_.nametable_select = v_.nametable_select ^ 0x01;  // Switch horiz nametable
    }
  }
}

void hw::ppu::PPU::incrementFineY() {
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
    mapper_->decodePPUAddress(v_.raw);
  }
}
