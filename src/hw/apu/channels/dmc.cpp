#include <nesemu/hw/apu/channels.h>

#include <nesemu/hw/apu/apu_clock.h>

#include <cstdint>


void hw::apu::channel::DMC::enable(bool enabled) {
  Channel::enable(enabled);
  irq_ = false;
  if (!enabled) {
    dma_remaining_ = 0;
    // TODO: Clear loop flag?
  } else {
    // Kick off sample
    if (dma_remaining_ == 0) {
      dma_address_   = sample_address_;
      dma_remaining_ = sample_length_;
      DMAFetch();
    }
  }
}


void hw::apu::channel::DMC::writeReg(uint8_t reg, uint8_t data) {
  switch (reg & 0x03) {
    case 0x00:
      timer_.setPeriod(PERIODS[data & 0x0F]);
      loop_       = data & 0x40;
      IRQ_enable_ = data & 0x80;
      if (!IRQ_enable_) {
        irq_ = false;
      }
      break;
    case 0x01:
      // Direct load of output
      // TODO: Check conflict here. If timer outputs a clock at the same time as this write, the write may not take
      // effect
      output_ = data & 0x7F;
      break;
    case 0x02:
      sample_address_ = 0xC000 | (data << 6);
      break;
    case 0x03:
      sample_length_ = (data << 4) | 0x0001;
      break;
  }
}


void hw::apu::channel::DMC::clockCPU() {
  if (!timer_.clock()) {
    return;
  }

  if (!silence_) {
    // Update output based on bit 0 of the sample buffer. Ensure output does not clip.
    if (bit_buffer_ & 0x01) {
      output_ += (output_ <= 125) ? 2 : 0;
    } else {
      output_ -= (output_ >= 125) ? 2 : 0;
    }
  }

  bit_buffer_ >>= 1;
  bits_remaining_ -= 1;
  if (bits_remaining_ == 0) {
    bits_remaining_ = 8;
    if (has_sample_) {
      silence_    = false;
      bit_buffer_ = sample_buffer_;
      DMAFetch();
    } else {
      silence_ = true;
    }
  }
};


void hw::apu::channel::DMC::DMAFetch() {
  // TODO: Read sample from memory. Requires CPU stalls
  //  sample_buffer_ = READ(dma_address_);

  // Increment DMA address
  dma_address_ = (dma_address_ + 1) | 0x8000;

  // Decrement DMA bytes remaining
  if (dma_remaining_ > 0) {
    dma_remaining_--;
    if (dma_remaining_ == 0 && loop_) {
      dma_address_   = sample_address_;
      dma_remaining_ = sample_length_;
    } else if (dma_remaining_ == 0 && IRQ_enable_) {
      irq_ = true;
    }
  }
}
