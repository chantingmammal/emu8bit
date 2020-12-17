#include <nesemu/hw/console.h>
#include <nesemu/hw/rom.h>
#include <nesemu/logger.h>
#include <nesemu/ui/screen.h>
#include <nesemu/ui/window.h>
#include <nesemu/utils/steady_timer.h>

#include <cstdio>
#include <getopt.h>
#include <map>
#include <string>

#include <SDL2/SDL_events.h>


std::map<std::string, ui::Window*> windows;

void printUsage() {
  printf("Usage: nesemu --file ROM.nes\n");
}

int  init();
void exit();

int main(int argc, char* argv[]) {
  int         opt = 0;
  std::string filename;
  bool        allow_unofficial = true;

  static struct option long_options[] = {{"file", required_argument, nullptr, 'f'},
                                         {"official", no_argument, nullptr, 'o'},
                                         {"quiet", no_argument, nullptr, 'q'},
                                         {"verbose", optional_argument, nullptr, 'v'},
                                         {"help", no_argument, nullptr, 'h'},
                                         {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "f:s:oqv::h", long_options, nullptr)) != -1) {
    switch (opt) {
      case 'f':  // -f or --file
        filename = std::string(optarg);
        break;
      case 'o':  // -o or --official
        allow_unofficial = false;
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

  hw::rom::Rom rom;
  if (hw::rom::parseFromFile(filename, &rom)) {
    return 1;
  }

  // Initialize SDL
  if (init()) {
    exit();
    return 1;
  }

  // Setup the windows
  windows["screen"] = new ui::Screen();
  if (windows["screen"]->init("NES Emu")) {
    exit();
    return 1;
  }

  hw::console::Console console(allow_unofficial);
  console.setScreen(static_cast<ui::Screen*>(windows["screen"]));
  console.loadCart(&rom);
  console.start();

  utils::SteadyTimer<1, 30> sdl_timer;
  sdl_timer.start();

  // When the main window is closed, exit the program
  bool running = true;
  windows["screen"]->onClose([&]() -> void { running = false; });

  SDL_Event event;
  while (running) {

    console.update();

    // Process SDL events at 30Hz
    if (sdl_timer.ready()) {
      while (SDL_PollEvent(&event)) {
        // Exit on SDL_QUIT
        if (event.type == SDL_QUIT) {
          running = false;
        }

        // Handle window events (show/hide, focus, resize, etc)
        for (auto&& window : windows) {
          window.second->handleEvent(event);
        }

        // Handle console events (Controller input)
        console.handleEvent(event);

      }

      // Render all visible windows
      for (auto&& window : windows) {
        window.second->update();
      }
    }
  }

  exit();
  return 0;
}

/// Initialize SDL subsystems
int init() {
  if (SDL_Init(SDL_INIT_VIDEO)) {
    logger::log<logger::ERROR>("Video initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  return 0;
}

/// Cleanup windows and shutdown SDL subsystems
void exit() {

  // Cleanup all windows
  for (auto&& window : windows) {
    window.second->close();
    delete window.second;
  }

  // Quit SDL subsystems
  SDL_Quit();
}
