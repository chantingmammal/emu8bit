#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

#include <SDL2/SDL.h>


namespace ui {

class Window {
public:
  Window(unsigned width, unsigned height) : TEXTURE_WIDTH(width), TEXTURE_HEIGHT(height) {}
  virtual ~Window() = default;

  // Setup & cleanup window
  virtual bool init(const std::string&& string, bool shown = true);
  virtual void close();

  // Render window
  virtual void update();

  // Handles window events
  virtual void handleEvent(SDL_Event& event);
  virtual void onClose(const std::function<void(void)>& f) { on_close_ = f; };

  void hide();
  void focus();
  bool isHidden() const { return !visible_; };

  // Window dimensions
  // int getWidth();
  // int getHeight();

  // Window state
  // bool hasMouseFocus();
  // bool hasKeyboardFocus();
  // bool isMinimized();

protected:
  const unsigned TEXTURE_WIDTH;
  const unsigned TEXTURE_HEIGHT;

  // SDL data
  SDL_Window*   window_   = {nullptr};
  SDL_Renderer* renderer_ = {nullptr};
  SDL_Texture*  texture_  = {nullptr};

  // ID
  std::string title_;
  uint32_t    id_ = {0};

  // Dimensions
  int width_  = {0};
  int height_ = {0};

  // State
  bool mouse_focus_    = {false};
  bool keyboard_focus_ = {false};
  bool full_screen_    = {false};
  bool visible_        = {false};

  std::function<void(void)> on_close_;
};

}  // namespace ui
