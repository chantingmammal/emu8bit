#pragma once

#include <nesemu/ui/window.h>


// Forward declarations
namespace hw::ppu {
class PPU;
}


namespace ui {

class SpriteViewer : public Window {
public:
  SpriteViewer() : Window(64, 64) {}

  void attachPPU(const hw::ppu::PPU* ppu) { ppu_ = ppu; };
  void update() override;

private:
  const hw::ppu::PPU* ppu_ = {nullptr};
};

}  // namespace ui
