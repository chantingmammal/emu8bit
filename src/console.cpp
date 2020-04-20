#include <nesemu/console.h>

#include <cstdint>
#include <functional>


// =*=*=*=* Console Setup =*=*=*=*

console::Console::Console() {
  using namespace std::placeholders;

  cpu_.setPPUReadRegisterPtr(std::bind(&ppu::PPU::readRegister, &ppu_, _1));
  cpu_.setPPUWriteRegisterPtr(std::bind(&ppu::PPU::writeRegister, &ppu_, _1, _2));
  cpu_.setPPUSpriteDMAPtr(std::bind(&ppu::PPU::spriteDMAWrite, &ppu_, _1));
  cpu_.setClockTickPtr(std::bind(&console::Console::clockTick, this));

  ppu_.setTriggerVBlankPtr(std::bind(&cpu::CPU::NMI, &cpu_));
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

void console::Console::setScreenPixelPtr(void* pixels) {
  ppu_.setPixelPtr(pixels);
}

void console::Console::setUpdateScreenPtr(std::function<void(void)> func) {
  ppu_.setUpdateScreenPtr(func);
}

// =*=*=*=* Console Execution =*=*=*=*

void console::Console::start() {
  cpu_.reset(true);
  cpu_.executeInstruction();
  cpu_.reset(false);
}

void console::Console::clockTick() {
  ppu_.tick();
  ppu_.tick();
  ppu_.tick();

  // cpu_.tick();
}

void console::Console::update() {
  cpu_.executeInstruction();
}

void console::Console::handleEvent(const SDL_Event& event) {
  // TODO: Joysticks
}
