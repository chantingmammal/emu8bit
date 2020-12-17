#include <nesemu/ui/nametable_viewer.h>

#include <nesemu/hw/ppu.h>
#include <nesemu/logger.h>


void ui::NametableViewer::attachPPU(const hw::ppu::PPU* ppu) {
  ppu_ = ppu;
}

void ui::NametableViewer::update() {
  if (minimized_) {
    return;
  }

  uint32_t pixels[256 * 240] = {0};  // Screen buffer

  // Fill with random colors for now
  static uint32_t c = 0;
  for (size_t i = 0; i < sizeof(pixels) / sizeof(pixels[0]); i++) {
    pixels[i] = c;
  }
  c += (1 << 0) + (2 << 8) + (3 << 16);

  SDL_UpdateTexture(texture_, nullptr, pixels, WINDOW_WIDTH * sizeof(uint32_t));
  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, texture_, NULL, NULL);
  SDL_RenderPresent(renderer_);
}
