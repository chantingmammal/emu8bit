#pragma once

#include <algorithm>  // std::min
#include <cstdint>    // uint8_t, uint16_t

#include <SDL2/SDL.h>


namespace ui {

class Speaker {
public:
  bool init();
  void close();
  void update(uint8_t* stream, size_t len);
  void setVolume(float volume) { volume_ = std::max(std::min(volume, 1.f), 0.f); };
  void addVolume(float delta) { volume_ = std::max(std::min(volume_ + delta, 1.f), 0.f); };
  void pause(bool pause);

private:
  // We need to downsample from ~1.79MHz to a standard freq like 44.1kHz. Since the downsampler only supports integral
  // sampling rates, we first upsample by 11 to achieve an integral rate, then downsample to the output freq
  static constexpr int             UPSAMPLE        = 11;
  static constexpr int             MULTIPLIER      = 100;
  static constexpr int             FREQ_A          = (39375000 * UPSAMPLE) / (22 * MULTIPLIER);
  static constexpr int             OUTPUT_C        = 44100;      // Output freq
  static constexpr SDL_AudioFormat OUTPUT_FORMAT   = AUDIO_U8;   // Unsigned 8-bit samples
  static constexpr uint8_t         OUTPUT_CHANNELS = 1;          // Mono
  static constexpr uint16_t        OUTPUT_SAMPLES  = (1 << 12);  // Buffer size, in samples

  float volume_ = 0.5;

  SDL_AudioDeviceID device_;
  SDL_AudioSpec     audio_spec_;
  SDL_AudioStream*  downsampler_;
};

}  // namespace ui
