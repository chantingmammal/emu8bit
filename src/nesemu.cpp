#include <nesemu/console.h>
#include <nesemu/logger.h>
#include <nesemu/rom.h>
#include <nesemu/steady_timer.h>
#include <nesemu/window.h>

#include <getopt.h>
#include <iostream>
#include <string>

#include <SDL2/SDL_events.h>


void printUsage() {
  std::cout << "Usage: nesemu --file ROM.nes\n";
}


int main(int argc, char* argv[]) {
  int         opt = 0;
  std::string filename;
  int         scale = 1;

  static struct option long_options[] = {{"file", required_argument, nullptr, 'f'},
                                         {"scale", required_argument, nullptr, 's'},
                                         {"debug", required_argument, nullptr, 'd'},
                                         {"cpu", required_argument, nullptr, 'c'},
                                         {"ppu", required_argument, nullptr, 'p'},
                                         {"apu", required_argument, nullptr, 'a'},
                                         {"bus", required_argument, nullptr, 'b'},
                                         {"mapper", required_argument, nullptr, 'm'},
                                         {"help", no_argument, nullptr, 'h'},
                                         {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "f:s:dcpabmh", long_options, nullptr)) != -1) {
    switch (opt) {
      case 'f':  // -f or --file
        filename = std::string(optarg);
        break;
      case 's':  // -s or --scale
        scale = std::atoi(optarg);
        break;
      case 'd':  // -d or --debug
        logger::level = logger::Level::DEBUG_ALL;
        break;
      case 'c':  // -c or --cpu
        logger::level = static_cast<logger::Level>(logger::level | logger::Level::DEBUG_CPU);
        break;
      case 'p':  // -p or --ppu
        logger::level = static_cast<logger::Level>(logger::level | logger::Level::DEBUG_PPU);
        break;
      case 'a':  // -a or --apu
        logger::level = static_cast<logger::Level>(logger::level | logger::Level::DEBUG_ALL);
        break;
      case 'b':  // -b or --bus
        logger::level = static_cast<logger::Level>(logger::level | logger::Level::DEBUG_BUS);
        break;
      case 'm':  // -m or --mapper
        logger::level = static_cast<logger::Level>(logger::level | logger::Level::DEBUG_MAPPER);
        break;
      case 'h':  // -h or --help
      case '?':  // Unrecognized option
      default:
        printUsage();
        return 1;
    }
  }

  if (filename.empty()) {
    printUsage();
    return 1;
  }

  std::cout << std::hex;

  rom::Rom rom;
  if (rom::parseFromFile(filename, &rom))
    return 1;

  window::Window window;
  if (window.init(scale))
    return 1;

  console::Console console;
  console.setWindow(&window);
  console.loadCart(&rom);
  console.start();

  SteadyTimer<1, 30> sdl_timer;
  sdl_timer.start();

  bool      running = true;
  SDL_Event event;
  while (running) {

    // Process SDL events at 30Hz
    if (sdl_timer.ready()) {
      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
          running = false;
        } else {
          console.handleEvent(event);
        }
      }
    }

    console.update();
  }

  return 0;
}
