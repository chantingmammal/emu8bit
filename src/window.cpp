#include <nesemu/window.h>

#include <iostream>


window::Window::~Window() {
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

int window::Window::init() {

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "Video initialization failed: " << SDL_GetError() << std::endl;
    return 1;
  }

  window_ = SDL_CreateWindow("NES Emu",                // Window title
                             SDL_WINDOWPOS_UNDEFINED,  // Initial x position
                             SDL_WINDOWPOS_UNDEFINED,  // Initial y position
                             WINDOW_WIDTH,             // Width, in pixels
                             WINDOW_HEIGHT,            // Height, in pixels
                             SDL_WINDOW_SHOWN);        // Flags
  screen_ = SDL_GetWindowSurface(window_);

  return 0;
}

void* window::Window::getScreenPixelPtr() {
  return screen_->pixels;
}

std::function<void(void)> window::Window::getUpdateScreenPtr() {
  return std::bind(SDL_UpdateWindowSurface, window_);
}
