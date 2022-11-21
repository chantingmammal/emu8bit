# nesemu

[![Build Status](https://github.com/matthew-reynolds/nesemu/actions/workflows/cmake.yml/badge.svg?branch=master)](https://github.com/matthew-reynolds/nesemu/actions/workflows/cmake.yml?query=branch%3Amaster)

A simple, (hopefully) cycle-accurate NES emulator.

## Building

Builds with CMake. Requires SDL.

```
sudo apt install libsdl2-dev
git clone https://github.com/matthew-reynolds/nesemu.git
mkdir nesemu/build
cd nesemu/build
cmake ..
make
```

## Usage

```
Usage: nesemu [options]... file.nes
  -h --help               print this usage and exit
  -s --save=file.sav      specify the savefile to use. Default {ROMCRC32}.sav
  -o --official           allow unofficial opcodes
  -q --quiet              disable all logging
  -v --verbose[=abceimpw] specify the log levels. If no argument is specified,
                          all messages are displayed. Every level implies all
                          log levels (eg. INFO implies WARNING and ERROR)
                            a = APU DEBUG messages
                            b = bus DEBUG messages
                            c = CPU DEBUG messages
                            e = ERROR messages
                            i = INFO messages
                            m = mapper DEBUG messages
                            p = PPU DEBUG messages
                            w = WARNING messages

  If neither -q nor -v are specified, the default log level of
   INFO|WARNING|ERROR is used.
```

## Controls

NES    | Keyboard
-------|--------
D-Pad  | Arrow keys
A      | Z
B      | X
Start  | Enter
Select | Space
Reset  | R
Volume down | Left bracket [
Volume up | Right bracket ]
Unlimit speed | Tab
Debug: PPU nametable viewer | 1
Debug: PPU sprite viewer | 2
