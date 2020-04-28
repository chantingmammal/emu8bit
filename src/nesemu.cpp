#include <nesemu/console.h>
#include <nesemu/rom.h>
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
                                         {"help", no_argument, nullptr, 'h'},
                                         {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "f:s:h", long_options, nullptr)) != -1) {
    switch (opt) {
      case 'f':  // -f or --file
        filename = std::string(optarg);
        break;
      case 's':  // -s or --scale
        scale = std::atoi(optarg);
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
  console.setScreenPixelPtr(window.getScreenPixelPtr());
  console.setUpdateScreenPtr(window.getUpdateScreenPtr());
  console.loadCart(&rom);
  console.start();

  bool      running = true;
  SDL_Event event;
  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      } else {
        console.handleEvent(event);
      }
    }

    console.update();
  }

  return 0;
}
