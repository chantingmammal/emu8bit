#pragma once

#include <cstdint>
#include <iomanip>
#include <iostream>


inline void dump(uint8_t* data, size_t size) {
  static bool done = false;
  if (done)
    return;
  done = true;

  size_t pos = 0;
  while (pos < size) {
    std::cout << std::hex << std::setfill('0') << std::setw(4) << pos << "\t";

    for (uint8_t i = 0; i < 16 && pos < size; i++) {
      std::cout << std::setw(2) << unsigned(data[pos++]) << " ";
    }

    std::cout << std::endl;
  }
}


inline void dump_nt(uint8_t* data) {
  static bool done = false;
  if (done)
    return;
  done = true;

  size_t pos = 0;

  for (uint8_t row = 0; row < 30; row++) {
    for (uint8_t col = 0; col < 32; col++) {
      std::cout << std::hex << std::setfill('0') << std::setw(2) << unsigned(data[pos++]) << "\t";
    }
    std::cout << std::endl;
  }
}

inline void dump_patterns(uint8_t* data) {
  static bool done = false;
  if (done)
    return;
  done = true;

  std::cout << std::hex << std::setw(0);
  for (unsigned tile = 0; tile < 256; tile++) {
    std::cout << "\nTile #" << tile << std::endl;
    for (unsigned row = 0; row < 8; row++) {
      const uint8_t low  = data[tile * 16 + row];
      const uint8_t high = data[tile * 16 + row + 8];
      for (int col = 7; col >= 0; col--) {

        const uint8_t a = ((low >> col) & 1) | (((high >> col) << 1) & 2);
        switch (a) {
          case 0:
            std::cout << ".";
            break;
          case 1:

            std::cout << "|";
            break;
          case 2:

            std::cout << "2";
            break;
          case 3:

            std::cout << "3";
            break;
        }
      }
      std::cout << std::endl;
    }
  }
}

inline void print_bg(uint8_t* nt, uint8_t* pt) {
  static bool done = false;
  if (done)
    return;
  done = true;

  for (unsigned coarse_y = 0; coarse_y < 30; coarse_y++) {
    for (unsigned fine_y = 0; fine_y < 8; fine_y++) {
      for (unsigned coarse_x = 0; coarse_x < 32; coarse_x++) {
        const unsigned tile = (coarse_y * 32) + coarse_x;
        const uint8_t  p    = nt[tile];
        const uint8_t  low  = pt[p * 16 + fine_y];
        const uint8_t  high = pt[p * 16 + fine_y + 8];
        for (int fine_x = 7; fine_x >= 0; fine_x--) {
          std::cout << (((low >> fine_x) & 1) | (((high >> fine_x) << 1) & 2));
        }
        std::cout << " ";
      }
      std::cout << "\n";
    }
    std::cout << "\n";
  }
}
