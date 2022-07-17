#pragma once

#include <nesemu/ui/window.h>
#include <nesemu/utils/buffer.h>

#include <chrono>
#include <cstdint>

#include <SDL2/SDL.h>


namespace ui {

class Screen : public Window {
public:
  Screen() : Window(256, 240) {}

  // Note: Do not use default update(), since this window is updated in the PPU loop rather than SDL loop
  void update() override {};
  void update(const uint32_t* pixels_);

  void handleEvent(SDL_Event& event) override;


private:
  unsigned frame_ = {0};

  utils::Buffer<double, 10>             fps_buffer_;
  std::chrono::steady_clock::time_point prev_frame_;

  bool lock_aspect_ratio_ = true;
};

}  // namespace ui
