#include <nesemu/ui/screen.h>

#include <algorithm>  // std::min
#include <cstdio>     // snprintf


void ui::Screen::update(const uint32_t* pixels) {
  if (minimized_) {
    return;
  }

  std::chrono::duration<double> duration = std::chrono::steady_clock::now() - prev_frame_;
  prev_frame_                            = std::chrono::steady_clock::now();
  fps_buffer_.append(duration.count());

  frame_++;
  if ((frame_ % 10) == 0) {
    size_t size   = snprintf(nullptr, 0, "%s | %.2f FPS", title_.c_str(), 1.0 / fps_buffer_.avg()) + 1;
    char*  buffer = new char[size];
    snprintf(buffer, size, "%s | %.2f FPS", title_.c_str(), 1.0 / fps_buffer_.avg());
    SDL_SetWindowTitle(window_, buffer);
    delete buffer;
  }

  SDL_UpdateTexture(texture_, nullptr, pixels, WINDOW_WIDTH * sizeof(uint32_t));
  SDL_RenderClear(renderer_);

  if (lock_aspect_ratio_) {
    int d = std::min(width_, height_);
    SDL_Rect dest = {(width_ - d)/2, (height_ - d)/2, d, d};
    SDL_RenderCopy(renderer_, texture_, NULL, &dest);
  } else {
    SDL_RenderCopy(renderer_, texture_, NULL, NULL);
  }
  SDL_RenderPresent(renderer_);
}

void ui::Screen::handleEvent(SDL_Event& event) {
  if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_l) {
    lock_aspect_ratio_ = !lock_aspect_ratio_;
  }
  Window::handleEvent(event);
}
