#include <nesemu/hw/joystick.h>

#include <SDL2/SDL_keyboard.h>


void hw::joystick::Joystick::write(uint8_t data) {

  if (prev_strobe_ && !(data & 0x01)) {
    strobe_pos_ = 0;

    // TODO: Controller 2 mapping
    const uint8_t* state = SDL_GetKeyboardState(nullptr);
    state_.a             = (port_ == 1) ? state[SDL_SCANCODE_Z] : false;
    state_.b             = (port_ == 1) ? state[SDL_SCANCODE_X] : false;
    state_.select        = (port_ == 1) ? state[SDL_SCANCODE_SPACE] : false;
    state_.start         = (port_ == 1) ? state[SDL_SCANCODE_RETURN] : false;
    state_.up            = (port_ == 1) ? state[SDL_SCANCODE_UP] : false;
    state_.down          = (port_ == 1) ? state[SDL_SCANCODE_DOWN] : false;
    state_.left          = (port_ == 1) ? state[SDL_SCANCODE_LEFT] : false;
    state_.right         = (port_ == 1) ? state[SDL_SCANCODE_RIGHT] : false;
  }

  prev_strobe_ = (data & 0x01);
}

uint8_t hw::joystick::Joystick::read() {
  register_.standard = (strobe_pos_ < 8) ? (state_.raw >> strobe_pos_) : 1;
  strobe_pos_++;
  return register_.raw;
}