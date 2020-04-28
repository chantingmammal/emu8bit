#pragma once

#include <functional>

#include <SDL2/SDL.h>


namespace window {

constexpr int WINDOW_WIDTH  = 256;
constexpr int WINDOW_HEIGHT = 240;


class Window {
public:
  ~Window();
  int                       init(int scale);
  void*                     getScreenPixelPtr();
  std::function<void(void)> getUpdateScreenPtr();

private:
  inline void  updateScreen();
  SDL_Window*  window_      = {nullptr};
  SDL_Surface* window_surf_ = {nullptr};
  SDL_Surface* screen_      = {nullptr};
};

}  // namespace window
