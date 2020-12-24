#include <nesemu/hw/console.h>

#include <nesemu/hw/mapper/mappers.h>
#include <nesemu/hw/rom.h>
#include <nesemu/logger.h>
#include <nesemu/ui/screen.h>

#include <cstdint>


// =*=*=*=*= Console Setup =*=*=*=*=

hw::console::Console::Console(bool allow_unofficial_opcodes) {
  bus_.connectChips(&apu_, &cpu_, &ppu_, &joy_1_, &joy_2_);

  cpu_.connectBus(&bus_);
  cpu_.allowUnofficialOpcodes(allow_unofficial_opcodes);
}

hw::console::Console::~Console() {
  if (mapper_ != nullptr) {
    delete mapper_;
  }
}

void hw::console::Console::loadCart(rom::Rom* rom) {
  // Setup the mapper
  const uint16_t          mapper_num = rom->header.mapper_upper << 8 | rom->header.mapper_lower;
  const mapper::Mirroring mirror     = rom->header.ignore_mirroring
                                       ? mapper::Mirroring::none
                                       : (rom->header.nametable_mirror ? mapper::Mirroring::vertical
                                                                       : mapper::Mirroring::horizontal);
  mapper_ = mapper::mappers[mapper_num](rom->header.prg_rom_size, rom->header.chr_rom_size, mirror);
  logger::log<logger::DEBUG_MAPPER>("Using mapper #%d\n", mapper_num);

  // Load the rom and mapper onto the busses
  bus_.loadCart(mapper_, rom->prg[0], (rom->header.has_battery ? rom->expansion[0] : nullptr));
  ppu_.loadCart(mapper_, rom->chr[0], (rom->header.chr_rom_size == 0));
}

void hw::console::Console::setScreen(ui::Screen* screen) {
  screen_ = screen;
  ppu_.setScreen(screen_);
}


// =*=*=*=*= Console Execution =*=*=*=*=

void hw::console::Console::start() {
  cpu_.reset(true);
  cpu_.executeInstruction();
  cpu_.reset(false);
}

void hw::console::Console::update() {
  cpu_.executeInstruction();
}

void hw::console::Console::handleEvent(const SDL_Event& event) {
  if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_r)
    cpu_.reset(true);
  if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_r)
    cpu_.reset(false);
}
