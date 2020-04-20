#pragma once

#include <functional>

#include <SDL2/SDL.h>


namespace window {

constexpr int WINDOW_WIDTH  = 256;
constexpr int WINDOW_HEIGHT = 240;


class Window {
public:
  ~Window();
  int                       init();
  void*                     getScreenPixelPtr();
  std::function<void(void)> getUpdateScreenPtr();

private:
  SDL_Window*  window_ = {nullptr};
  SDL_Surface* screen_ = {nullptr};
};

}  // namespace window
