#include <nesemu/window.h>

#include <nesemu/logger.h>

#include <cstdio>  // snprintf


window::Window::~Window() {
  SDL_DestroyTexture(texture_);
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

int window::Window::init(int scale) {

  if (SDL_Init(SDL_INIT_VIDEO)) {
    logger::log<logger::ERROR>("Video initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  if (!(window_ = SDL_CreateWindow("NES Emu",                // Window title
                                   SDL_WINDOWPOS_UNDEFINED,  // Initial x position
                                   SDL_WINDOWPOS_UNDEFINED,  // Initial y position
                                   WINDOW_WIDTH * scale,     // Width, in pixels
                                   WINDOW_HEIGHT * scale,    // Height, in pixels
                                   SDL_WINDOW_SHOWN))) {     // Flags
    logger::log<logger::ERROR>("Window initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  if (!(renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_PRESENTVSYNC))) {
    logger::log<logger::ERROR>("Renderer initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  if (!(texture_ = SDL_CreateTexture(renderer_,
                                     SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     WINDOW_WIDTH,
                                     WINDOW_HEIGHT))) {
    logger::log<logger::ERROR>("Texture initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  return 0;
}


void window::Window::updateScreen(const uint32_t* pixels) {
  std::chrono::duration<double> duration = std::chrono::steady_clock::now() - prev_frame_;
  prev_frame_                            = std::chrono::steady_clock::now();
  fps_buffer_.append(duration.count());

  frame_++;
  if ((frame_ % 10) == 0) {
    size_t size   = snprintf(nullptr, 0, "NES Emu | %.2f FPS", 1.0 / fps_buffer_.avg()) + 1;
    char*  buffer = new char[size];
    snprintf(buffer, size, "NES Emu | %.2f FPS", 1.0 / fps_buffer_.avg());
    SDL_SetWindowTitle(window_, buffer);
    delete buffer;
  }

  SDL_UpdateTexture(texture_, nullptr, pixels, WINDOW_WIDTH * sizeof(uint32_t));
  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, texture_, NULL, NULL);
  SDL_RenderPresent(renderer_);
}
