#include <nesemu/window.h>

#include <iostream>


window::Window::~Window() {
  SDL_FreeSurface(screen_);
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

int window::Window::init(int scale) {

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "Video initialization failed: " << SDL_GetError() << std::endl;
    return 1;
  }

  window_ = SDL_CreateWindow("NES Emu",                // Window title
                             SDL_WINDOWPOS_UNDEFINED,  // Initial x position
                             SDL_WINDOWPOS_UNDEFINED,  // Initial y position
                             WINDOW_WIDTH * scale,     // Width, in pixels
                             WINDOW_HEIGHT * scale,    // Height, in pixels
                             SDL_WINDOW_SHOWN);        // Flags

  window_surf_ = SDL_GetWindowSurface(window_);
  screen_      = SDL_CreateRGBSurface(0, 256, 240, 32, 0, 0, 0, 0);

  return 0;
}

void* window::Window::getScreenPixelPtr() {
  return screen_->pixels;
}

std::function<void(void)> window::Window::getUpdateScreenPtr() {
  return std::bind(&window::Window::updateScreen, this);
}


void window::Window::updateScreen() {
  SDL_BlitScaled(screen_, nullptr, window_surf_, nullptr);
  SDL_UpdateWindowSurface(window_);
}
