#include <nesemu/ui/nametable_viewer.h>

#include <nesemu/hw/ppu.h>
#include <nesemu/logger.h>
#include <nesemu/temp_mapping.h>


void ui::NametableViewer::update() {
  if (minimized_) {
    return;
  }

  // Screen buffer
  uint32_t pixels[TEXTURE_WIDTH * TEXTURE_HEIGHT] = {0};

  // Iterate over every 8x8 tile in the 2x2 nametable array
  for (uint8_t row = 0; row < 60; row++) {
    for (uint8_t col = 0; col < 64; col++) {

      // Tile index within the current nametable
      const uint8_t nt = (col < 32 ? 0 : 1) | (row < 30 ? 0 : 2);
      const uint8_t r  = row % 30;
      const uint8_t c  = col % 32;

      // Fine the index into the pattern table from the nametable
      const uint16_t offset       = c | r << 5 | nt << 10;
      const uint16_t pattern_addr = (ppu_->readByte(0x2000 | offset) << 4)                // Base tile address
                                    | ppu_->ctrl_reg_1_.screen_pattern_table_addr << 12;  // Pattern table

      // Index into attribute table (4x4 tile palette)
      const uint16_t at_entry    = (c / 4) | ((r / 4) << 3) | nt << 10;
      const uint8_t  at_subentry = (c & 2) | ((r & 2) << 1);

      // Get palette from attribute table
      const uint8_t full_palette = ppu_->readByte(0x23C0 | at_entry);
      const bool    palette_a    = (full_palette >> at_subentry) & 0x01;
      const bool    palette_b    = (full_palette >> (at_subentry | 1)) & 0x01;
      const uint8_t sub_palette  = palette_a | (palette_b << 1);

      // Tile row
      for (uint8_t i = 0; i < 8; i++) {
        const uint8_t ptrn_a = ppu_->readByte((pattern_addr + i));
        const uint8_t ptrn_b = ppu_->readByte((pattern_addr + i) | 8);

        // Tile col
        for (uint8_t j = 0; j < 8; j++) {
          const uint8_t pixel = ((ptrn_a >> (7 - j)) & 0x01) | (((ptrn_b >> (7 - j)) << 1) & 0x02);

          uint8_t palette_addr;
          if (pixel == 0) {
            palette_addr = 0;
          } else {
            palette_addr = pixel | (sub_palette << 2);
          }

          pixels[(row * 8 + i) * TEXTURE_WIDTH + (col * 8 + j)] = decodeColor(ppu_->readByte(0x3F00 | palette_addr));
        }
      }
    }
  }

  // Using the t_ variable, draw the current viewport into the nametable array
  // TODO: Using t_ is imperfect, since t_ sometimes jumps around a bit
  const uint32_t box_color = 0x00FF0000;  // Red
  const unsigned x = (ppu_->t_.coarse_x_scroll * 8 + ppu_->fine_x_scroll_) + (ppu_->t_.nametable_select & 0x01) * 256;
  const unsigned y = (ppu_->t_.coarse_y_scroll * 8 + ppu_->t_.fine_y_scroll)
                     + (ppu_->t_.nametable_select & 0x02) * 240 / 2;

  for (unsigned col = x; col < x + 255; col++) {
    pixels[y * TEXTURE_WIDTH + (col % TEXTURE_WIDTH)] = box_color;
  }
  for (unsigned row = y; row < y + 239; row++) {
    pixels[(row % TEXTURE_HEIGHT) * TEXTURE_WIDTH + x]                         = box_color;
    pixels[(row % TEXTURE_HEIGHT) * TEXTURE_WIDTH + (x + 255) % TEXTURE_WIDTH] = box_color;
  }
  for (unsigned col = x; col < x + 255; col++) {
    pixels[((y + 239) % TEXTURE_HEIGHT) * TEXTURE_WIDTH + (col % TEXTURE_WIDTH)] = box_color;
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
