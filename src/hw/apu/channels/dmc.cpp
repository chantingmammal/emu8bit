#include <nesemu/hw/apu/channels.h>

#include <nesemu/hw/apu/apu_clock.h>

#include <cstdint>


void hw::apu::channel::DMC::writeReg(uint8_t reg, uint8_t data) {
  (void) reg;
  (void) data;
  // TODO
}


void hw::apu::channel::DMC::clockFrame(APUClock clock_type) {
  switch (clock_type) {
    case APUClock::QUARTER_FRAME:
      break;
    case APUClock::HALF_FRAME:
      length_counter_.clock();
      break;
      // No default
  }
}
