#include <nesemu/joystick.h>

#include <SDL2/SDL_keyboard.h>


joystick::Joystick::Joystick(uint8_t port) {
  // Disconnect joystick 2
  if (port == 2) {
    register_.connected = 1;
  }
}

void joystick::Joystick::write(uint8_t data) {
  static bool prev_strobe = false;

  if (prev_strobe && !(data & 0x01)) {
    strobe_pos_ = 0;

    // TODO: Handle stick 1 vs stick 2
    const uint8_t* state = SDL_GetKeyboardState(nullptr);
    state_.a             = state[SDL_SCANCODE_Z];
    state_.b             = state[SDL_SCANCODE_X];
    state_.select        = state[SDL_SCANCODE_SPACE];
    state_.start         = state[SDL_SCANCODE_RETURN];
    state_.up            = state[SDL_SCANCODE_UP];
    state_.down          = state[SDL_SCANCODE_DOWN];
    state_.left          = state[SDL_SCANCODE_LEFT];
    state_.right         = state[SDL_SCANCODE_RIGHT];
  }

  prev_strobe = (data & 0x01);
}

uint8_t joystick::Joystick::read() {
  register_.data = (state_.raw >> strobe_pos_);
  strobe_pos_++;
  return register_.raw;
}