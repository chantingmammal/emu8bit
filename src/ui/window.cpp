#include <nesemu/ui/window.h>

#include <nesemu/logger.h>

#include <string>


bool ui::Window::init(const std::string&& title, bool shown) {
  title_ = title;

  uint32_t window_flags = (shown ? SDL_WINDOW_SHOWN : SDL_WINDOW_HIDDEN) | SDL_WINDOW_RESIZABLE;

  // Create window
  if (!(window_ = SDL_CreateWindow(title_.c_str(),           // Window title
                                   SDL_WINDOWPOS_UNDEFINED,  // Initial x position
                                   SDL_WINDOWPOS_UNDEFINED,  // Initial y position
                                   TEXTURE_WIDTH,             // Width, in pixels
                                   TEXTURE_HEIGHT,            // Height, in pixels
                                   window_flags))) {         // Flags
    logger::log<logger::ERROR>("Window initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  // TODO: Cannot be vsynced in multiple windows. Only vsync main display
  if (!(renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED))) {
    logger::log<logger::ERROR>("Renderer initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  if (!(texture_ = SDL_CreateTexture(renderer_,
                                     SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     TEXTURE_WIDTH,
                                     TEXTURE_HEIGHT))) {
    logger::log<logger::ERROR>("Texture initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  id_             = SDL_GetWindowID(window_);
  mouse_focus_    = true;
  keyboard_focus_ = true;
  shown_          = shown;
  width_          = TEXTURE_WIDTH;
  height_         = TEXTURE_HEIGHT;

  return 0;
}

void ui::Window::close() {
  SDL_DestroyTexture(texture_);
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);

  texture_        = nullptr;
  renderer_       = nullptr;
  window_         = nullptr;
  mouse_focus_    = false;
  keyboard_focus_ = false;
  shown_          = false;
  width_          = 0;
  height_         = 0;
}

void ui::Window::update() {
  if (!minimized_) {

    static bool warned = false;
    if (!warned) {
      logger::log<logger::WARNING>("Window %s does not override the update() method!\n", title_.c_str());
      warned = true;
    }
    SDL_SetRenderDrawColor(renderer_, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);
  }
}

void ui::Window::handleEvent(SDL_Event& event) {

  // Window event occured
  if (event.type == SDL_WINDOWEVENT && event.window.windowID == id_) {
    switch (event.window.event) {
      // Window appeared
      case SDL_WINDOWEVENT_SHOWN:
        shown_ = true;
        break;

      // Window disappeared
      case SDL_WINDOWEVENT_HIDDEN:
        shown_ = false;
        break;

      // Get new dimensions and repaint on window size change
      case SDL_WINDOWEVENT_SIZE_CHANGED:
        width_  = event.window.data1;
        height_ = event.window.data2;
        SDL_RenderPresent(renderer_);
        break;

      // Repaint on exposure
      case SDL_WINDOWEVENT_EXPOSED:
        SDL_RenderPresent(renderer_);
        break;

      // Mouse entered window
      case SDL_WINDOWEVENT_ENTER:
        mouse_focus_ = true;
        break;

      // Mouse left window
      case SDL_WINDOWEVENT_LEAVE:
        mouse_focus_ = false;
        break;

      // Window has keyboard focus
      case SDL_WINDOWEVENT_FOCUS_GAINED:
        keyboard_focus_ = true;
        break;

      // Window lost keyboard focus
      case SDL_WINDOWEVENT_FOCUS_LOST:
        keyboard_focus_ = false;
        break;

      // Window minimized
      case SDL_WINDOWEVENT_MINIMIZED:
        minimized_ = true;
        break;

      // Window maxized
      case SDL_WINDOWEVENT_MAXIMIZED:
        minimized_ = false;
        break;

      // Window restored
      case SDL_WINDOWEVENT_RESTORED:
        minimized_ = false;
        break;

      // Hide on close
      case SDL_WINDOWEVENT_CLOSE:
        SDL_HideWindow(window_);
        if (on_close_) {
          on_close_();
        }
        break;
    }
  }
}


void ui::Window::hide() {
  if (shown_) {
    SDL_HideWindow(window_);
  }
}

void ui::Window::focus() {
  // Restore window if needed
  if (!shown_) {
    SDL_ShowWindow(window_);
  }

  // Move window forward
  SDL_RaiseWindow(window_);
}
