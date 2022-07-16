#include <nesemu/ui/sprite_viewer.h>

#include <nesemu/hw/ppu.h>
#include <nesemu/logger.h>
#include <nesemu/temp_mapping.h>


void ui::SpriteViewer::update() {
  if (minimized_) {
    return;
  }

  // Screen buffer
  uint32_t pixels[TEXTURE_WIDTH * TEXTURE_HEIGHT] = {0};

  // Iterate over every sprite in the OAM
  for (uint8_t row = 0; row < 8; row++) {
    for (uint8_t col = 0; col < 8; col++) {

      const uint8_t               sprite_id = row * 8 + col;
      const hw::ppu::PPU::Sprite& sprite    = ppu_->primary_oam_.sprite[sprite_id];

      const uint16_t pattern_addr = sprite.small_tile_index << 4                          // Base tile address
                                    | ppu_->ctrl_reg_1_.sprite_pattern_table_addr << 12;  // Pattern table

      // Tile row
      for (uint8_t i = 0; i < 8; i++) {
        const uint8_t ptrn_a = ppu_->readByte((pattern_addr + i));
        const uint8_t ptrn_b = ppu_->readByte((pattern_addr + i) | 8);

        // Tile col
        for (uint8_t j = 0; j < 8; j++) {
          const uint8_t  pixel        = ((ptrn_a >> (7 - j)) & 0x01) | (((ptrn_b >> (7 - j)) << 1) & 0x02);
          const uint16_t palette_addr = pixel | (sprite.attributes.palette << 2) | 0x10;

          pixels[(row * 8 + i) * TEXTURE_WIDTH + (col * 8 + j)] = decodeColor(ppu_->readByte(0x3F00 | palette_addr));
        }
      }
    }
  }

  // Draw the nametable to the screen, with locked aspect ratio TEXTURE_HEIGHT/TEXTURE_WIDTH
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
