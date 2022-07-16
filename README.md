# nesemu

A (hopefully) cycle-accurate NES emulator.

This personal project is more of a learning experience than an optimized emulator. Targets cycle-accurate emulation of the NES's CPU (a MOS6502 derivative), APU, and PPU.

## Building

Builds with CMake. Requires OpenGL and SDL.

## Usage

`nesemu --file file.rom`

## Controls

NES    | Keyboard
-------|--------
D-Pad  | Arrow keys
A      | Z
B      | X
Start  | Enter
Select | Space
Reset  | R
Unlimit speed | Tab
Debug: PPU nametable viewer | 1
Debug: PPU sprite viewer | 2
