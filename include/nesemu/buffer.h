#pragma once

template <class T, unsigned Capacity = 10>
struct Buffer {
  T            frame_duration_[Capacity] = {0};
  unsigned head_                     = {0};
  unsigned size_                     = {0};

  void append(T d) {
    frame_duration_[head_] = d;
    if (size_ < Capacity) {
      size_++;
    }
    head_ = (head_ + 1) % Capacity;
  }

  T avg() {
    T total = 0;
    for (unsigned i = 0; i < Capacity; i++) {
      total += frame_duration_[i];
    }
    return total / size_;
  }
};