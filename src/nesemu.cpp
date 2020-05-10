#include <nesemu/console.h>
#include <nesemu/logger.h>
#include <nesemu/rom.h>
#include <nesemu/steady_timer.h>
#include <nesemu/window.h>

#include <cstdio>
#include <getopt.h>
#include <string>

#include <SDL2/SDL_events.h>


void printUsage() {
  printf("Usage: nesemu --file ROM.nes\n");
}


int main(int argc, char* argv[]) {
  int         opt = 0;
  std::string filename;
  int         scale = 1;

  static struct option long_options[] = {{"file", required_argument, nullptr, 'f'},
                                         {"scale", required_argument, nullptr, 's'},
                                         {"quiet", no_argument, nullptr, 'q'},
                                         {"verbose", optional_argument, nullptr, 'v'},
                                         {"help", no_argument, nullptr, 'h'},
                                         {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "f:s:qv::h", long_options, nullptr)) != -1) {
    switch (opt) {
      case 'f':  // -f or --file
        filename = std::string(optarg);
        break;
      case 's':  // -s or --scale
        scale = std::atoi(optarg);
        break;
      case 'q':  // -q or --quiet
        logger::level = logger::NONE;
        break;
      case 'v':  // -v or --verbose
        if (!optarg) {
          logger::level = static_cast<logger::Level>(logger::level | logger::ERROR | logger::WARNING | logger::INFO
                                                     | logger::DEBUG_CPU | logger::DEBUG_PPU | logger::DEBUG_APU
                                                     | logger::DEBUG_BUS | logger::DEBUG_MAPPER);
        } else {
          const auto INFO = logger::INFO | logger::WARNING | logger::ERROR;
          for (unsigned i = 0; optarg[i] != '\0'; i++) {
            switch (optarg[i]) {
              case 'c':
                logger::level = static_cast<logger::Level>(logger::level | logger::DEBUG_CPU | INFO);
                break;
              case 'p':
                logger::level = static_cast<logger::Level>(logger::level | logger::DEBUG_PPU | INFO);
                break;
              case 'a':
                logger::level = static_cast<logger::Level>(logger::level | logger::DEBUG_APU | INFO);
                break;
              case 'b':
                logger::level = static_cast<logger::Level>(logger::level | logger::DEBUG_BUS | INFO);
                break;
              case 'm':
                logger::level = static_cast<logger::Level>(logger::level | logger::DEBUG_MAPPER | INFO);
                break;
              case 'i':
                logger::level = static_cast<logger::Level>(logger::level | INFO);
                break;
              case 'w':
                logger::level = static_cast<logger::Level>(logger::level | logger::WARNING | logger::ERROR);
                break;
              case 'e':
                logger::level = static_cast<logger::Level>(logger::level | logger::ERROR);
                break;
              default:
                printf("Unknown log level '%c'\n", optarg[0]);
                printUsage();
                return 1;
            }
          }
        }
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
