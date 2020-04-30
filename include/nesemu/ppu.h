#pragma once

#include <nesemu/utils.h>

#include <cstdint>
#include <cstring>  //memcpy
#include <functional>


// Ricoh RP2A03 (based on MOS6502)
// Little endian
namespace ppu {

enum class Mirroring { none, vertical, horizontal };

class PPU {
public:
  // Setup
  void loadCart(uint8_t* chr_mem, bool is_ram, Mirroring mirror);
  void setPixelPtr(void* pixels);
  void setUpdateScreenPtr(std::function<void(void)> func);
  void setTriggerVBlankPtr(std::function<void(void)> func);


  // Execution
  void    tick();
  uint8_t readRegister(uint16_t cpu_address);
  void    writeRegister(uint16_t cpu_address, uint8_t data);
  void    spriteDMAWrite(uint8_t* data);  // CPU 0x4014. Load sprite memory with 256 bytes.


private:
  // Registers
  union PPUReg {
    utils::RegBit<0, 15, uint16_t> raw;
    utils::RegBit<0, 8, uint16_t>  lower;
    utils::RegBit<8, 8, uint16_t>  upper;
    utils::RegBit<0, 5, uint16_t>  coarse_x_scroll;   // X offset of the current tile
    utils::RegBit<5, 5, uint16_t>  coarse_y_scroll;   // Y offset of the current tile
    utils::RegBit<10, 2, uint16_t> nametable_select;  // 0=0x2000, 1=0x2400, 2=0x2800, 3=0x2C00
    utils::RegBit<12, 3, uint16_t> fine_y_scroll;     // Y offset of the scanline within a tile
  };

  union SpriteAttributes {
    uint8_t             raw;
    utils::RegBit<0, 2> palette;     // Palette (4 to 7) of sprite
    utils::RegBit<5, 1> priority;    // 0=In front of background, 1=Behind background
    utils::RegBit<6, 1> flip_horiz;  // Flip sprite horizontally
    utils::RegBit<7, 1> flip_vert;   // Flip sprite vertically TODO: Unsupported
  } __attribute__((__packed__));

  struct Sprite {

    Sprite& operator=(const Sprite& val) {
      // Cast used to suppress -Wclass-memaccess
      memcpy((uint8_t*) this, (uint8_t*) &val, sizeof(Sprite));
      return *this;
    }

    uint8_t y_position = {0};                   // Measured from top left
    union {                                     // 8x8 sprites use 8 bit index & pattern table select from ctrl_reg_1_
                                                // 8x16 sprites use 7 bit index & specify pattern table
      uint8_t             small_tile_index;     // Tile number within pattern table
      utils::RegBit<0, 1> large_pattern_table;  // Pattern table ($0000 or $1000)
      utils::RegBit<1, 7> large_tile_index;     // Tile number for top half of sprite within pattern table
                                                //   (Bottom half uses next tile)
    } __attribute__((__packed__));
    SpriteAttributes attributes;
    uint8_t          x_position = {0};  // Measured from top left
  } __attribute__((__packed__));


  // Background registers
  PPUReg              t_               = {0};
  PPUReg              v_               = {0};
  utils::RegBit<0, 3> fine_x_scroll_   = {0};    // X offset of the scanline within a tile
  bool                write_toggle_    = false;  // 0 indicates first write
  uint16_t            pattern_sr_a_    = {0};    // Lower byte of pattern, controls bit 0 of the color
  uint16_t            pattern_sr_b_    = {0};    // Upper byte of pattern, controls bit 1 of the color
  uint8_t             palette_sr_a_    = {0};    // Palette number, controls bit 2 of the color
  uint8_t             palette_sr_b_    = {0};    // Palette number, controls bit 3 of the color
  bool                palette_latch_a_ = {false};
  bool                palette_latch_b_ = {false};


  // Sprite registers
  union {
    uint8_t byte[0x100];
    Sprite  sprite[0x40];
  } primary_oam_ = {0};  // 256 byte/64 sprite ram, for curent frame (Object Attribute Memory)
  union {
    uint8_t byte[0x40];
    Sprite  sprite[0x08];
  } secondary_oam_                          = {0};      // 64 byte/8 sprite ram, for current scanline
  uint8_t          primary_oam_counter_     = {0};      // Position within primary OAM (0-64)
  uint8_t          secondary_oam_counter_   = {0};      // Position within secondary OAM (0-8)
  bool             has_sprite_zero          = {false};  // Whether the secondary OAM contains sprite 0
  uint8_t          sprite_pattern_sr_a_[8]  = {0};      // Lower byte of pattern, controls bit 0 of the color
  uint8_t          sprite_pattern_sr_b_[8]  = {0};      // Upper byte of pattern, controls bit 1 of the color
  SpriteAttributes sprite_palette_latch_[8] = {0};      //
  uint8_t          sprite_x_position_[8]    = {0};      //


  // Memory-mapped IO Registers
  union {                                         // PPU Control Register 1, mapped to CPU 0x2000 (RW)
    uint8_t raw;                                  //
    /* utils::RegBit<0, 2> nametable_address; */  //  - ** Mapped in temp_ register
    utils::RegBit<2> vertical_write;              //  - 0=PPU memory address increments by 1, 1=increments by 32
    utils::RegBit<3> sprite_pattern_table_addr;   //  - 0=0x0000, 1=0x1000
    utils::RegBit<4> screen_pattern_table_addr;   //  - 0=0x0000, 1=0x1000
    utils::RegBit<5> sprite_size;                 //  - 0=8x8, 1=0x8x16
    utils::RegBit<6> ppu_master_slave_mode;       //  - Unused
    utils::RegBit<7> vblank_enable;               //  - Generate interrupts on VBlank
  } ctrl_reg_1_ = {0};                            //
  union {                                         // PPU Control Register 2, mapped to CPU 0x2001 (RW)
    uint8_t             raw;                      //
    utils::RegBit<3, 2> render_enable;            //  - Read only, shortcut for checking if rendering is enabled
    utils::RegBit<0>    greyscale;                //  - Produce greyscale display
    utils::RegBit<1>    image_mask;               //  - Show left 8 columns of the screen
    utils::RegBit<2>    sprite_mask;              //  - Show sprites in left 8 columns
    utils::RegBit<3>    screen_enable;            //  - 0=Blank screen, 1=Show picture
    utils::RegBit<4>    sprites_enable;           //  - 0=Hide sprites, 1=Show sprites
    utils::RegBit<5, 3> background_color;         //  - 0=Black, 1=Blue, 2=Green, 4=Red. Do not use other numbers
  } ctrl_reg_2_ = {0};                            //
  union {                                         // PPU Status Register, mapped to CPU 0x2002 (R)
    uint8_t raw;                                  //
                                                  //  - Unknown
    utils::RegBit<6> hit;                         //  - Sprite refresh hit sprite #0. Reset when screen refresh starts.
    utils::RegBit<7> vblank;                      //  - PPU is in VBlank. Reset when VBlank ends or CPU reads 0x2002
  } status_reg_     = {0};                        //
  uint8_t oam_addr_ = {0};                        // Object Attribute Memory Address, mapped to CPU 0x2003 (W)


  // Memory
  uint8_t   ram_[0x2000]    = {0};        // 8KiB RAM, at address 0x2000-0x3FFF
  uint8_t*  chr_mem_        = {nullptr};  // Character VRAM/VROM, at address 0x0000-0x1FFF
  bool      chr_mem_is_ram_ = {false};    // Whether chr_mem is VRAM or VROM
  Mirroring mirroring_;                   // Nametable mirroring mode


  // Rendering
  uint32_t*                 pixels_       = {nullptr};  // 8 bits per channel
  uint16_t                  scanline_     = {0};        // 0 to 262
  uint16_t                  cycle_        = {1};        // 0 to 341
  bool                      frame_is_odd_ = true;
  std::function<void(void)> trigger_vblank_;
  std::function<void(void)> update_screen_;


// Internal operations
#if DEBUG
  uint8_t readByteInternal(uint16_t address);
#endif
  uint8_t readByte(uint16_t address);
  void    writeByte(uint16_t address, uint8_t data);
  void    renderPixel();

  inline void fetchTilesAndSprites(bool fetch_sprites);
  inline void fetchNextBGTile();
  inline void fetchNextSprite();
  inline void incrementCoarseX();
  inline void incrementFineY();
};

}  // namespace ppu
