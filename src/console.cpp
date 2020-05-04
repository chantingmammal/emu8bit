#include <nesemu/console.h>

#include <cstdint>


// =*=*=*=*= Console Setup =*=*=*=*=

console::Console::Console() {
  cpu_.connectChips(nullptr, &ppu_, &joy_1_, &joy_2_);
  ppu_.connectChips(&cpu_);
}

void console::Console::loadCart(rom::Rom* rom) {
  cpu_.loadCart(rom->prg[0], rom->header.prg_rom_size, (rom->header.has_battery ? rom->expansion[0] : nullptr));

  ppu::Mirroring mirror = ppu::Mirroring::horizontal;
  if (rom->header.ignore_mirroring)
    mirror = ppu::Mirroring::none;
  else if (rom->header.nametable_mirror)
    mirror = ppu::Mirroring::vertical;
  ppu_.loadCart(rom->chr[0], (rom->header.chr_rom_size == 0), mirror);
}

void console::Console::setWindow(window::Window* window) {
  window_ = window;
  ppu_.setWindow(window_);
}


// =*=*=*=*= Console Execution =*=*=*=*=

void console::Console::start() {
  cpu_.reset(true);
  cpu_.executeInstruction();
  cpu_.reset(false);
}

void console::Console::update() {
  cpu_.executeInstruction();
}

void console::Console::handleEvent(const SDL_Event& event) {
  if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_r)
    cpu_.reset(true);
  if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_r)
    cpu_.reset(false);
}
