#pragma once

#include <algorithm>  // std::min
#include <cstddef>    // size_t
#include <cstdint>    // uint8_t, uint16_t

#include <SDL2/SDL.h>


namespace ui {

class Speaker;
void audio_callback(Speaker* speaker, uint8_t* stream, size_t len);

class Speaker {
public:
  bool init();
  void close();
  void update(uint8_t* stream, size_t len);
  void setVolume(float volume) { volume_ = std::max(std::min(volume, 1.f), 0.f); };
  void addVolume(float delta) { volume_ = std::max(std::min(volume_ + delta, 1.f), 0.f); };
  void pause(bool pause);

private:
  friend void audio_callback(Speaker* speaker, uint8_t* stream, size_t len);

  // We need to downsample from ~1.79MHz to a standard freq like 44.1kHz. Since the downsampler only supports integral
  // sampling rates, we first upsample by 11 to achieve an integral rate, then downsample to the output freq
  static constexpr int             UPSAMPLE        = 11;
  static constexpr int             MULTIPLIER      = 100;
  static constexpr int             FREQ_A          = (39375000 * UPSAMPLE) / (22 * MULTIPLIER);
  static constexpr int             OUTPUT_C        = 44100;      // Output freq
  static constexpr SDL_AudioFormat OUTPUT_FORMAT   = AUDIO_U8;   // Unsigned 8-bit samples
  static constexpr uint8_t         OUTPUT_CHANNELS = 1;          // Mono
  static constexpr uint16_t        OUTPUT_SAMPLES  = (1 << 11);  // Buffer size, in samples
                                                                 //   Seem to always get 2x less than requested

  // We ensure the target audio buffer len is at least as long as the output device sample buffer, so that the audio
  // device never runs dry.
  //
  // The resulting latency is the processing time of the downsampler plus the internal buffer size of the
  // downsampler. This is approx 0.07sec, or 1/15th of a sec.
  //
  //  - The downsampler processing time has not yet been measured
  //  - The upsampler outputs every 100 samples = 100*(22/39375000) ~= 0.000056sec
  //  - The downsampler starts outputting after 41560 input samples = 41560*(22/39375000) ~= 0.023sec
  //  - The TARGET_AUDIO_BUFFER_LEN = (2^11)/44100 ~= 0.045sec
  //  - The OUTPUT_SAMPLES = (2^10)/44100 ~= 0.023sec, irrelevant since TARGET_AUDIO_BUFFER_LEN >= OUTPUT_SAMPLES
  static constexpr size_t TARGET_AUDIO_BUFFER_LEN = (1 << 11);
  static_assert(TARGET_AUDIO_BUFFER_LEN >= OUTPUT_SAMPLES);

  float volume_ = 0.5;

  SDL_AudioDeviceID device_;
  SDL_AudioSpec     audio_spec_;
  SDL_AudioStream*  downsampler_;
};

}  // namespace ui
