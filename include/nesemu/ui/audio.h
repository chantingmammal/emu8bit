#pragma once

#include <algorithm>  // std::min
#include <cstdint>    // uint8_t, uint16_t

#include <SDL2/SDL.h>


namespace ui {

class Audio {
public:
  virtual bool init();
  virtual void close();
  virtual void update(uint8_t* stream, size_t len);
  virtual void setVolume(float volume) { volume_ = std::max(std::min(volume, 1.f), 0.f); };

private:
  // We need to downsample from ~1.79MHz to a standard freq like 48kHz. Since 1.79MHz is too fast, we downsample in two
  // steps. Additionally, since the downsampler only supports integral sampling rates, we first upsample by 11 to
  // achieve an integral rate, then downsample by 500, then downsample the remainder to the output freq
  static constexpr int             UPSAMPLE        = 11;
  static constexpr int             FREQ_A          = 500;
  static constexpr int             FREQ_B          = (39375000 * UPSAMPLE) / (22 * FREQ_A);
  static constexpr int             OUTPUT_C        = 48000;      // Output freq
  static constexpr SDL_AudioFormat OUTPUT_FORMAT   = AUDIO_U8;   // Unsigned 8-bit samples
  static constexpr uint8_t         OUTPUT_CHANNELS = 1;          // Mono
  static constexpr uint16_t        OUTPUT_SAMPLES  = (1 << 12);  // Buffer size, in samples

  float volume_ = 0.5;

  SDL_AudioDeviceID device_;
  SDL_AudioSpec     audio_spec_;
  SDL_AudioStream*  downsampler_a_;
  SDL_AudioStream*  downsampler_b_;
};

}  // namespace ui
