#include <nesemu/hw/console.h>

#include <nesemu/hw/mapper/mappers.h>
#include <nesemu/hw/rom.h>
#include <nesemu/logger.h>
#include <nesemu/ui/screen.h>
#include <nesemu/ui/speaker.h>

#include <cstdint>


// =*=*=*=*= Console Setup =*=*=*=*=

hw::console::Console::Console(bool allow_unofficial_opcodes) {
  bus_.connectChips(&clock_, &apu_, &cpu_, &ppu_, &joy_1_, &joy_2_);

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
  const uint8_t           mapper_num = rom->header.mapper_upper << 4 | rom->header.mapper_lower;
  const mapper::Mirroring mirror     = rom->header.ignore_mirroring
                                           ? mapper::Mirroring::none
                                           : (rom->header.nametable_mirror ? mapper::Mirroring::vertical
                                                                           : mapper::Mirroring::horizontal);
  logger::log<logger::DEBUG_MAPPER>("Using mapper #%d\n", mapper_num);
  mapper_ = mapper::getMapper(mapper_num)(rom->header.prg_rom_size, rom->header.chr_rom_size, mirror);

  // Load the rom and mapper onto the busses
  bus_.loadCart(mapper_, rom->prg[0], (rom->header.has_battery ? rom->expansion[0] : nullptr));
  ppu_.loadCart(mapper_, rom->chr[0], (rom->header.chr_rom_size == 0));
}

void hw::console::Console::setScreen(ui::Screen* screen) {
  screen_ = screen;
  ppu_.setScreen(screen_);
}

void hw::console::Console::setSpeaker(ui::Speaker* speaker) {
  speaker_ = speaker;
  apu_.setSpeaker(speaker_);
}


// =*=*=*=*= Console Execution =*=*=*=*=

void hw::console::Console::start() {
  clock_.start();
  cpu_.reset(true);
  cpu_.executeInstruction();
  cpu_.reset(false);
}

void hw::console::Console::update() {
  cpu_.reset(reset_);
  // TODO: Reset APU & PPU regs
  cpu_.executeInstruction();
}

void hw::console::Console::reset(bool reset) {
  reset_ = reset;
  // speaker_->pause(reset_);
  if (!reset_) {
    clock_.start();
  }
}
