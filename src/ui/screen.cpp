#include <nesemu/ui/screen.h>

#include <cstdio>  // snprintf


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

  SDL_UpdateTexture(texture_, nullptr, pixels, TEXTURE_WIDTH * sizeof(uint32_t));
  SDL_RenderClear(renderer_);

  // Lock aspect ratio to TEXTURE_HEIGHT/TEXTURE_WIDTH
  if (lock_aspect_ratio_) {
    SDL_Rect dest;
    if (width_ * TEXTURE_HEIGHT < height_ * TEXTURE_WIDTH) {
      const int h = (width_ * TEXTURE_HEIGHT) / TEXTURE_WIDTH;
      dest        = SDL_Rect {0, (height_ - h) / 2, width_, h};
    } else {
      const int w = (height_ * TEXTURE_WIDTH) / TEXTURE_HEIGHT;
      dest        = SDL_Rect {(width_ - w) / 2, 0, w, height_};
    }
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
