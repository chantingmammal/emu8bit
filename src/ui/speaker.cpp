#include <nesemu/logger.h>
#include <nesemu/ui/speaker.h>

#include <cstddef>
#include <cstdint>

// TODO: Make circular buffer class
constexpr size_t AUDIO_BUFFER_LEN = (1 << 13);  // (2^13)/48000 ~= 0.175sec
uint8_t          audio_buffer[AUDIO_BUFFER_LEN];
size_t           audio_buffer_head = {0};
size_t           audio_buffer_tail = {0};
size_t           audio_buffer_size = {0};

extern "C" void audio_callback(void* userdata, uint8_t* stream, int len) {
  (void) userdata;
  if (audio_buffer_size == 0) {
    // No salvaging this :/
    return;
  }

  size_t copied     = std::min<size_t>(len, audio_buffer_size);
  size_t copy_a_len = std::min<size_t>(copied, AUDIO_BUFFER_LEN - audio_buffer_head);
  SDL_memcpy(stream, audio_buffer + audio_buffer_head, copy_a_len);
  SDL_memcpy(stream + copy_a_len, audio_buffer, copied - copy_a_len);
  audio_buffer_head = (audio_buffer_head + copied) % AUDIO_BUFFER_LEN;
  audio_buffer_size -= copied;

  // Hacky audio smoothing. Replay the last chunk of samples back and forth to fill up the audio buffer. This sounds
  // pretty bad, but it's better than the crackles and pops you get if filled with silence.
  while (copied < len) {
    int to_copy = std::min(copied, (len - copied));
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
  desired_spec.callback = audio_callback;

  // Attach audio device
  if (!(device_ = SDL_OpenAudioDevice(NULL, 0, &desired_spec, &audio_spec_, SDL_AUDIO_ALLOW_ANY_CHANGE))) {
    logger::log<logger::ERROR>("Couldn't open audio device: %s\n", SDL_GetError());
    return 1;
  }

  // Create downsamplers
  if (!(downsampler_a_ = SDL_NewAudioStream(AUDIO_U8, 1, FREQ_A, AUDIO_U8, 1, 1))) {
    logger::log<logger::ERROR>("Couldn't open create audio stream: %s\n", SDL_GetError());
    return 1;
  }
  if (!(downsampler_b_ = SDL_NewAudioStream(AUDIO_U8, 1, FREQ_B, AUDIO_U8, 1, audio_spec_.freq))) {
    logger::log<logger::ERROR>("Couldn't open create audio stream: %s\n", SDL_GetError());
    return 1;
  }

  // Start playback
  SDL_PauseAudioDevice(device_, 0);

  return 0;
}

void ui::Speaker::close() {
  SDL_CloseAudioDevice(device_);
  SDL_FreeAudioStream(downsampler_a_);
  SDL_FreeAudioStream(downsampler_b_);
}

void ui::Speaker::update(uint8_t* stream, size_t len) {
  // Upscale by 11 and feed the first downsampler
  // Note that feeding the downsampler is slow, so feed it 11 at a time
  // We could consider further slowing this down
  for (size_t i = 0; i < len; i++) {
    static uint8_t upsample_buffer[UPSAMPLE];
    SDL_memset(upsample_buffer, stream[i] * volume_, UPSAMPLE);
    if (0 != SDL_AudioStreamPut(downsampler_a_, upsample_buffer, UPSAMPLE)) {
      logger::log<logger::ERROR>("Failed to put samples in first downsampler: %s\n", SDL_GetError());
    }
  }

  // When the first downsampler has output, feed the second downsampler
  constexpr size_t BUFFER_LEN = 1024;
  uint8_t          buffer[BUFFER_LEN];
  size_t           available = 0;
  while ((available = std::min<size_t>(SDL_AudioStreamAvailable(downsampler_a_), BUFFER_LEN))
         && SDL_AudioStreamGet(downsampler_a_, buffer, available)) {
    if (0 != SDL_AudioStreamPut(downsampler_b_, buffer, available)) {
      logger::log<logger::ERROR>("Failed to put samples in second downsampler: %s\n", SDL_GetError());
    }
  }

  // When the second downsampler has output, feed the output device
  while ((available = std::min<size_t>(SDL_AudioStreamAvailable(downsampler_b_), BUFFER_LEN))
         && SDL_AudioStreamGet(downsampler_b_, buffer, available)) {
    SDL_LockAudioDevice(device_);
    // Overwrite oldest samples if needed
    if (audio_buffer_size == AUDIO_BUFFER_LEN) {
      audio_buffer_head = (audio_buffer_head + available) % AUDIO_BUFFER_LEN;
    }

    size_t copy_a_len = std::min(available, AUDIO_BUFFER_LEN - audio_buffer_tail);
    memcpy(audio_buffer + audio_buffer_tail, buffer, copy_a_len);
    memcpy(audio_buffer, buffer + copy_a_len, available - copy_a_len);
    audio_buffer_tail = (audio_buffer_tail + available) % AUDIO_BUFFER_LEN;
    audio_buffer_size += available;
    SDL_UnlockAudioDevice(device_);
  }
}
