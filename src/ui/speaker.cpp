#include <nesemu/logger.h>
#include <nesemu/ui/speaker.h>

#include <cmath>
#include <cstddef>
#include <cstdint>

extern "C" void cb_trampoline(void* userdata, uint8_t* stream, int len) {
  ui::Speaker* speaker = static_cast<ui::Speaker*>(userdata);
  ui::audio_callback(speaker, stream, len);
}

void ui::audio_callback(ui::Speaker* speaker, uint8_t* stream, size_t len) {
  const size_t available = SDL_AudioStreamAvailable(speaker->downsampler_);

  static bool startup_complete = false;
  if (!startup_complete) {
    if (available < ui::Speaker::TARGET_AUDIO_BUFFER_LEN) {
      memset(stream, 0, len);
      return;
    } else {
      startup_complete = true;
    }
  }


  // No salvaging this, nothing to stretch :/
  if (available == 0) {
    memset(stream, 0, len);
    return;
  }

  // Fill the device audio buffer
  size_t copied = std::min<size_t>(len, available);
  SDL_AudioStreamGet(speaker->downsampler_, stream, copied);

  // Drop samples as required from the input buffer to maintain the target buffer length
  // This compensates for extra frames added when the buffer runs dry or the system runs over 100% speed (uncapped)
  if (available > ui::Speaker::TARGET_AUDIO_BUFFER_LEN) {
    const size_t to_drop     = available - ui::Speaker::TARGET_AUDIO_BUFFER_LEN;
    const size_t drop_period = std::max<size_t>(std::floor(static_cast<double>(len) / to_drop), 1);

    // TODO: Support drop period less than 1 (ie drop multiple samples per iteration)
    for (size_t i = 1; i <= to_drop && i * drop_period < len; i++) {
      SDL_memmove(stream + (i * drop_period), stream + (i * drop_period) + 1, len - (i * drop_period + 1));
      SDL_AudioStreamGet(speaker->downsampler_, &stream[len - 1], 1);
    }
  }

  // Hacky audio stretching. Replay the last chunk of samples back and forth to fill up the audio buffer. This sounds
  // pretty bad, but it's better than the crackles and pops you get if filled with silence.
  const size_t repeat_buffer = copied;
  while (copied < len) {
    int to_copy = std::min(repeat_buffer, (len - copied));
    for (int i = 0; i < to_copy; i++) {
      stream[copied + i] = stream[copied - 1 - i];
    }
    copied += to_copy;
  }
}

bool ui::Speaker::init() {

  SDL_AudioSpec desired_spec;
  desired_spec.freq     = OUTPUT_C;
  desired_spec.format   = OUTPUT_FORMAT;
  desired_spec.channels = OUTPUT_CHANNELS;
  desired_spec.samples  = OUTPUT_SAMPLES;
  desired_spec.callback = cb_trampoline;
  desired_spec.userdata = this;

  // Attach audio device
  // Do not allow format or channels to change
  if (!(device_ = SDL_OpenAudioDevice(NULL,
                                      0,
                                      &desired_spec,
                                      &audio_spec_,
                                      SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_SAMPLES_CHANGE))) {
    logger::log<logger::ERROR>("Couldn't open audio device: %s\n", SDL_GetError());
    return 1;
  }

  // Create downsampler
  if (!(downsampler_ = SDL_NewAudioStream(AUDIO_U8,
                                          1,
                                          FREQ_A,
                                          audio_spec_.format,
                                          audio_spec_.channels,
                                          audio_spec_.freq / MULTIPLIER))) {
    logger::log<logger::ERROR>("Couldn't open create audio stream: %s\n", SDL_GetError());
    return 1;
  }

  // Start playback
  SDL_PauseAudioDevice(device_, 0);

  return 0;
}

void ui::Speaker::close() {
  SDL_CloseAudioDevice(device_);
  SDL_FreeAudioStream(downsampler_);
}

void ui::Speaker::update(uint8_t* stream, size_t len) {
  constexpr size_t UPSAMPLE_B = 100;
  static uint8_t   upsample_buffer[UPSAMPLE * UPSAMPLE_B];
  static uint8_t   buffer_size = 0;

  // Upscale by 11 and feed the downsampler
  // Note that feeding the downsampler is slow, so feed it 11*10 at a time
  for (size_t i = 0; i < len; i++) {
    SDL_memset(&upsample_buffer[UPSAMPLE * buffer_size++], stream[i] * volume_, UPSAMPLE);

    if (buffer_size == UPSAMPLE_B) {
      SDL_LockAudioDevice(device_);
      if (0 != SDL_AudioStreamPut(downsampler_, upsample_buffer, UPSAMPLE * UPSAMPLE_B)) {
        logger::log<logger::ERROR>("Failed to put samples in first downsampler: %s\n", SDL_GetError());
      }
      SDL_UnlockAudioDevice(device_);
      buffer_size = 0;
    }
  }


  // TODO: 90Hz first-order high-pass
  // TODO: 440Hz first-order high-pass
  // TODO: 14kHz first-order low-pass
}

void ui::Speaker::pause(bool pause) {
  SDL_PauseAudioDevice(device_, pause);
}
