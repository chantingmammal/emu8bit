#include <nesemu/ui/pattern_table_viewer.h>

#include <nesemu/hw/ppu.h>
#include <nesemu/logger.h>
#include <nesemu/temp_mapping.h>


void ui::PatternTableViewer::update() {
  if (!visible_) {
    return;
  }

  // Screen buffer
  static uint32_t* pixels = new uint32_t[TEXTURE_WIDTH * TEXTURE_HEIGHT];

  // Iterate over every tile in the first pattern table
  for (uint8_t table = 0; table < 2; table++) {
    for (uint8_t row = 0; row < 16; row++) {
      for (uint8_t col = 0; col < 16; col++) {
        const uint16_t pattern_addr = (row * 16 + col) * 8  // Base tile address
                                      | table << 12;        // Pattern table

        // Tile row
        for (uint8_t i = 0; i < 8; i++) {
          const uint8_t ptrn_a = ppu_->readByte((pattern_addr + i));
          const uint8_t ptrn_b = ppu_->readByte((pattern_addr + i) | 8);

          // Tile col
          for (uint8_t j = 0; j < 8; j++) {
            const uint8_t  pixel        = ((ptrn_a >> (7 - j)) & 0x01) | (((ptrn_b >> (7 - j)) << 1) & 0x02);
            const uint16_t palette_addr = pixel | (palette_ << 2);

            pixels[((table * 16 + row) * 8 + i) * TEXTURE_WIDTH + (col * 8 + j)] = decodeColor(
                ppu_->readByte(0x3F00 | palette_addr));
          }
        }
      }
    }
  }

  // Draw the pattern table to the screen, with locked aspect ratio TEXTURE_HEIGHT/TEXTURE_WIDTH
  SDL_UpdateTexture(texture_, nullptr, pixels, TEXTURE_WIDTH * sizeof(uint32_t));
  SDL_RenderClear(renderer_);
  SDL_Rect dest;
  if (width_ * TEXTURE_HEIGHT < height_ * TEXTURE_WIDTH) {
    const int h = (width_ * TEXTURE_HEIGHT) / TEXTURE_WIDTH;
    dest        = SDL_Rect {0, (height_ - h) / 2, width_, h};
  } else {
    const int w = (height_ * TEXTURE_WIDTH) / TEXTURE_HEIGHT;
    dest        = SDL_Rect {(width_ - w) / 2, 0, w, height_};
  }
  SDL_RenderCopy(renderer_, texture_, NULL, &dest);
  SDL_RenderPresent(renderer_);
}

void ui::PatternTableViewer::focus() {
  ui::Window::focus();
  if (visible_) {
    palette_ = (palette_ + 1) % 8;
  }
}