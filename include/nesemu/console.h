#pragma once

#include <nesemu/cpu.h>
#include <nesemu/joystick.h>
#include <nesemu/ppu.h>
#include <nesemu/rom.h>

#include <SDL2/SDL_events.h>


namespace console {

class Console {
public:
  Console();

  // Setup
  void loadCart(rom::Rom* rom);
  void setScreenPixelPtr(void* pixels);
  void setUpdateScreenPtr(std::function<void(void)> func);

  // Execution
  void start();
  void clockTick();
  void update();
  void handleEvent(const SDL_Event& event);

private:
  cpu::CPU           cpu_;
  ppu::PPU           ppu_;
  joystick::Joystick joy_1_;
  joystick::Joystick joy_2_;
};

}  // namespace console
