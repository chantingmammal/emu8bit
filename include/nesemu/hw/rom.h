#pragma once

#include <nesemu/utils/reg_bit.h>

#include <cstdint>
#include <string>


// http://fms.komkon.org/EMUL8/NES.html#LABM
namespace hw::rom {

enum class Mapper : uint16_t {
  standard,
};

struct Header {
  uint8_t name[4];                      // Should contain "NES^Z", or {0x4E 0x45 0x53 0x1A}
  uint8_t prg_rom_size;                 // Number of 16KiB program ROM banks
  uint8_t chr_rom_size;                 // Number of 8KiB character VROM banks (If 0, game uses 1 VRAM)
  union {                               //
    uint8_t          byte_6_raw;        //
    utils::RegBit<0> nametable_mirror;  // Nametable mirroring. 0=horiz (CIRAM A10=PPU A11), 1=vert (CIRAM A10=PPU A10)
    utils::RegBit<1> has_battery;       // Cartridge contains battery-backed RAM at 0x6000-0x7FFF
    utils::RegBit<2> has_trainer;       // Catridge contains a 512-byte trainer at 0x7000-0x71FF
    utils::RegBit<3> ignore_mirroring;  // Use four-screen VRAM layout
    utils::RegBit<4, 4> mapper_lower;   // Lower nibble of ROM mapper type
  };                                    //
  union {                               //
    uint8_t             byte_7_raw;     //
    utils::RegBit<0>    vs_unisystem;   // Cartidge is VS. System (arcade console)
    utils::RegBit<1, 3> reserved_a;     // Reserved
    utils::RegBit<4, 4> mapper_upper;   // Upper nibble of ROM mapper type
  };                                    //
  uint8_t prg_ram_size;                 // Number of 8KiB RAM banks. If 0, assume 1 // TODO
  union {                               //
    uint8_t             byte_9_raw;     //
    utils::RegBit<0>    region;         // Region. 0=NTSC, 1=PAL
    utils::RegBit<1, 7> reserved_b;     // Reserved
  };
  uint8_t padding[5];
};

struct Rom {
  Header   header;
  uint32_t crc;
  uint8_t (*expansion)[0x2000];
  uint8_t* trainer;
  uint8_t (*prg)[16 * 1024];
  uint8_t (*chr)[8 * 1024];
};

constexpr uint8_t header_name[4] = {0x4E, 0x45, 0x53, 0x1A};

int parseFromFile(std::string filename, Rom* rom);

}  // namespace hw::rom
