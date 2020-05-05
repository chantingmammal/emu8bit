#include <nesemu/window.h>

#include <iostream>


window::Window::~Window() {
  SDL_DestroyTexture(texture_);
  SDL_DestroyRenderer(renderer_);
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

  renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_PRESENTVSYNC);
  texture_  = SDL_CreateTexture(renderer_,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               WINDOW_WIDTH,
                               WINDOW_HEIGHT);

  return 0;
}


void window::Window::updateScreen(const uint32_t* pixels) {
  SDL_UpdateTexture(texture_, nullptr, pixels, WINDOW_WIDTH * sizeof(uint32_t));
  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, texture_, NULL, NULL);
  SDL_RenderPresent(renderer_);
}
