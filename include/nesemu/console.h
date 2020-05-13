#pragma once

#include <nesemu/cpu.h>
#include <nesemu/joystick.h>
#include <nesemu/ppu.h>
#include <nesemu/system_bus.h>

#include <SDL2/SDL_events.h>


// Forward declarations
namespace mapper {
class Mapper;
}

namespace rom {
class Rom;
}

namespace window {
class Window;
}


namespace console {

class Console {
public:
  explicit Console(bool allow_unofficial_opcodes);
  ~Console();

  // Setup
  void loadCart(rom::Rom* rom);
  void setWindow(window::Window* window);

  // Execution
  void start();
  void update();
  void handleEvent(const SDL_Event& event);

private:
  window::Window* window_ = {nullptr};

  // HW Components
  system_bus::SystemBus bus_;
  cpu::CPU              cpu_;
  ppu::PPU              ppu_;
  joystick::Joystick    joy_1_  = {1};
  joystick::Joystick    joy_2_  = {2};
  mapper::Mapper*       mapper_ = {nullptr};
};

}  // namespace console
