#include <nesemu/hw/joystick.h>

#include <SDL2/SDL_keyboard.h>

// TODO: Change controller 2 mapping
constexpr int MAPPING_A[2]     = {SDL_SCANCODE_Z, SDL_SCANCODE_Z};
constexpr int MAPPING_B[2]     = {SDL_SCANCODE_X, SDL_SCANCODE_X};
constexpr int MAPPING_SEL[2]   = {SDL_SCANCODE_SPACE, SDL_SCANCODE_SPACE};
constexpr int MAPPING_START[2] = {SDL_SCANCODE_RETURN, SDL_SCANCODE_RETURN};
constexpr int MAPPING_UP[2]    = {SDL_SCANCODE_UP, SDL_SCANCODE_UP};
constexpr int MAPPING_DOWN[2]  = {SDL_SCANCODE_DOWN, SDL_SCANCODE_DOWN};
constexpr int MAPPING_LEFT[2]  = {SDL_SCANCODE_LEFT, SDL_SCANCODE_LEFT};
constexpr int MAPPING_RIGHT[2] = {SDL_SCANCODE_RIGHT, SDL_SCANCODE_RIGHT};


void hw::joystick::Joystick::write(uint8_t data) {
  static const uint8_t* state = SDL_GetKeyboardState(nullptr);

  if (prev_strobe_ && !(data & 0x01)) {
    strobe_pos_ = 0;

    state_.a      = state[MAPPING_A[port_ - 1]];
    state_.b      = state[MAPPING_B[port_ - 1]];
    state_.select = state[MAPPING_SEL[port_ - 1]];
    state_.start  = state[MAPPING_START[port_ - 1]];
    state_.up     = state[MAPPING_UP[port_ - 1]];
    state_.down   = state[MAPPING_DOWN[port_ - 1]];
    state_.left   = state[MAPPING_LEFT[port_ - 1]];
    state_.right  = state[MAPPING_RIGHT[port_ - 1]];
  }

  prev_strobe_ = (data & 0x01);
}

uint8_t hw::joystick::Joystick::read() {
  register_.standard = (strobe_pos_ < 8) ? (state_.raw >> strobe_pos_) : 1;
  strobe_pos_++;
  return register_.raw;
}
