#pragma once

#include <nesemu/utils.h>

#include <cstdint>
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
  template <unsigned bit_pos, unsigned n_bits = 1, typename T = uint8_t>
  using RegBit = typename utils::RegBit<bit_pos, n_bits, T>;

  // Registers
  union {                                    // PPU Control Register 1, mapped to CPU 0x2000 (RW)
    uint8_t      raw;                        //
    RegBit<0, 2> nametable_address;          //  - 0=0x2000, 1=0x2400, 2=0x2800, 3=0x2C00
    RegBit<2>    vertical_write;             //  - 0=PPU memory address increments by 1, 1=increments by 32
    RegBit<3>    sprite_pattern_table_addr;  //  - 0=0x0000, 1=0x1000
    RegBit<4>    screen_pattern_table_addr;  //  - 0=0x0000, 1=0x1000
    RegBit<5>    sprite_size;                //  - 0=8x8, 1=0x8x16
    RegBit<6>    ppu_master_slave_mode;      //  - Unused
    RegBit<7>    vblank_enable;              //  - Generate interrupts on VBlank
  } ctrl_reg_1_ = {0};                       //
  union {                                    // PPU Control Register 2, mapped to CPU 0x2001 (RW)
    uint8_t raw;                             //
                                             //  - Unknown
    RegBit<1>    image_mask;                 //  - Show left 8 columns of the screen
    RegBit<2>    sprite_mask;                //  - Show sprites in left 8 columns
    RegBit<3>    screen_enable;              //  - 0=Blank screen, 1=Show picture
    RegBit<4>    sprites_enable;             //  - 0=Hide sprites, 1=Show sprites
    RegBit<5, 3> background_color;           //  - 0=Black, 1=Blue, 2=Green, 4=Red. Do not use other numbers
  } ctrl_reg_2_ = {0};                       //
  union {                                    // PPU Status Register, mapped to CPU 0x2002 (R)
    uint8_t raw;                             //
                                             //  - Unknown
    RegBit<6> hit;                           //  - Sprite refresh has hit sprite #0. Resets when screen refresh starts.
    RegBit<7> vblank;                        //  - PPU is in VBlank state. Resets when VBlank ends or CPU reads 0x2002
  } status_reg_                 = {0};       //
  uint8_t  sprite_mem_addr_     = {0};       // Sprite Memory Address,           mapped to CPU 0x2003 (W)
  uint8_t  vert_scroll_offset_  = {0};       // Vertical Screen Scroll Offset,   mapped to CPU 0x2005 (W)
  uint8_t  horiz_scroll_offset_ = {0};       // Horizontal Screen Scroll Offset, mapped to CPU 0x2005 (W)
  uint16_t ppu_mem_addr_        = {0};       // PPU Memory Address,              mapped to CPU 0x2006 (W)


  // Memory
  uint8_t   spr_ram_[0x100] = {0};        // 256 byte sprite ram
  uint8_t   ram_[0x2000]    = {0};        // 8KiB RAM, at address 0x2000-0x3FFF
  uint8_t*  chr_mem_        = {nullptr};  // Character VRAM/VROM, at address 0x0000-0x1FFF
  bool      chr_mem_is_ram_ = {false};    // Whether chr_mem is VRAM or VROM
  Mirroring mirroring_;                   // Nametable mirroring mode


  // Rendering
  void*                     pixels_;
  uint16_t                  scanline_;  // 0 to 262
  uint16_t                  cycle_;     // 0 to 341
  bool                      frame_is_odd_ = true;
  std::function<void(void)> trigger_vblank_;
  std::function<void(void)> update_screen_;


  // Internal operations
  uint8_t readByte(uint16_t address);
  void    writeByte(uint16_t address, uint8_t data);
};

}  // namespace ppu
