#pragma once

#include <nesemu/buffer.h>

#include <chrono>
#include <cstdint>

#include <SDL2/SDL.h>


namespace window {

constexpr int WINDOW_WIDTH  = 256;
constexpr int WINDOW_HEIGHT = 240;


class Window {
public:
  ~Window();
  int  init(int scale);
  void updateScreen(const uint32_t* pixels_);

private:
  unsigned frame_ = {0};

  Buffer<double, 10>                    fps_buffer_;
  std::chrono::steady_clock::time_point prev_frame_;

  SDL_Window*   window_   = {nullptr};
  SDL_Renderer* renderer_ = {nullptr};
  SDL_Texture*  texture_  = {nullptr};
};

}  // namespace window
