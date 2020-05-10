#include <nesemu/rom.h>

#include <nesemu/logger.h>
#include <nesemu/utils.h>

#include <fstream>


int rom::parseFromFile(std::string filename, Rom* rom) {
  std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);

  if (!file.is_open()) {
    logger::log<logger::ERROR>("Unable to open file %s\n", filename);
    return 1;
  }

  std::streampos size = file.tellg();
  file.seekg(0, std::ios::beg);

  file.read(reinterpret_cast<char*>(&rom->header), 16);

  unsigned expected_size = 16 + (rom->header.has_trainer ? 512 : 0) + (rom->header.prg_rom_size * 16 * 1024)
                           + (rom->header.chr_rom_size * 8 * 1024);

  if (!std::equal(rom->header.name, std::end(rom->header.name), header_name)) {
    logger::log<logger::ERROR>("Unable to parse file %s: Invalid header beginning with '%s'\n",
                               filename,
                               utils::uint8_to_hex_string(rom->header.name, 4));
    return 1;
  }

  if (expected_size != size) {
    logger::log<logger::ERROR>("Unable to parse file %s: Expected $%0X bytes, but file is $%0X bytes\n",
                               filename,
                               expected_size,
                               size);
    return 1;
  }

  if (rom->header.has_battery) {
    rom->expansion = new uint8_t[1][0x2000];

    if (rom->header.has_trainer) {
      rom->trainer = rom->expansion[0] + 0x1000;
      file.read(reinterpret_cast<char*>(&rom->trainer), 512);
    } else {
      rom->trainer = nullptr;
    }
  } else {
    rom->expansion = nullptr;
    rom->trainer   = nullptr;
  }

  rom->prg = new uint8_t[rom->header.prg_rom_size][16 * 1024];
  file.read(reinterpret_cast<char*>(rom->prg), rom->header.prg_rom_size * 16 * 1024);

  if (rom->header.chr_rom_size == 0) {
    rom->chr = new uint8_t[1][8 * 1024];  // Cartridge uses CHR RAM
  } else {
    rom->chr = new uint8_t[rom->header.chr_rom_size][8 * 1024];
    file.read(reinterpret_cast<char*>(rom->chr), rom->header.chr_rom_size * 8 * 1024);
  }

  return 0;
}
