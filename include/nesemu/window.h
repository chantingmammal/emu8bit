#pragma once

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
  SDL_Window*   window_   = {nullptr};
  SDL_Renderer* renderer_ = {nullptr};
  SDL_Texture*  texture_  = {nullptr};
};

}  // namespace window
