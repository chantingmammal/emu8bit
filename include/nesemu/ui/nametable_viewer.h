#pragma once

#include <nesemu/ui/window.h>


// Forward declarations
namespace hw::ppu {
class PPU;
}


namespace ui {

class NametableViewer : public Window {
public:
  void attachPPU(const hw::ppu::PPU* ppu);
  void update() override;

private:
  const hw::ppu::PPU* ppu_ = {nullptr};
};

}  // namespace ui
