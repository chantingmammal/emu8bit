#pragma once

#include <nesemu/ui/window.h>


// Forward declarations
namespace hw::ppu {
class PPU;
}


namespace ui {

class PatternTableViewer : public Window {
public:
  // Two 16x16 tables of 8x8 tiles, stacked vertically
  PatternTableViewer() : Window(128, 256) {}

  void attachPPU(const hw::ppu::PPU* ppu) { ppu_ = ppu; };
  void update() override;
  void focus() override;

private:
  const hw::ppu::PPU* ppu_ = {nullptr};
  uint8_t palette_ = {0};
};

}  // namespace ui
