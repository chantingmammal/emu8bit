#pragma once

#include <nesemu/cpu.h>
#include <nesemu/joystick.h>
#include <nesemu/ppu.h>
#include <nesemu/rom.h>
#include <nesemu/window.h>

#include <SDL2/SDL_events.h>


namespace console {

class Console {
public:
  Console();

  // Setup
  void loadCart(rom::Rom* rom);
  void setWindow(window::Window* window);

  // Execution
  void start();
  void update();
  void handleEvent(const SDL_Event& event);

private:
  window::Window* window_ = {nullptr};

  cpu::CPU           cpu_;
  ppu::PPU           ppu_;
  joystick::Joystick joy_1_ = {1};
  joystick::Joystick joy_2_ = {2};
};

}  // namespace console
