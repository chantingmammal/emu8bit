#include <nesemu/hw/console.h>
#include <nesemu/hw/rom.h>
#include <nesemu/logger.h>
#include <nesemu/ui/nametable_viewer.h>
#include <nesemu/ui/pattern_table_viewer.h>
#include <nesemu/ui/screen.h>
#include <nesemu/ui/speaker.h>
#include <nesemu/ui/sprite_viewer.h>
#include <nesemu/ui/window.h>
#include <nesemu/utils/steady_timer.h>

#include <cstdio>
#include <fstream>
#include <getopt.h>
#include <map>
#include <string>

#include <SDL2/SDL_events.h>


std::map<std::string, ui::Window*> windows;
ui::Speaker                        speaker;

void printUsage() {
  printf("Usage: nesemu [options]... file.nes\n");
  printf("  -h --help               print this usage and exit\n");
  printf("  -s --save=file.sav      specify the savefile to use. Default {ROMCRC32}.sav\n");
  printf("  -o --official           allow unofficial opcodes\n");
  printf("  -q --quiet              disable all logging\n");
  printf("  -v --verbose[=abceimpw] specify the log levels. If no argument is specified,\n");
  printf("                          all messages are displayed. Every level implies all\n");
  printf("                          log levels (eg. INFO implies WARNING and ERROR)\n");
  printf("                            a = APU DEBUG messages\n");
  printf("                            b = bus DEBUG messages\n");
  printf("                            c = CPU DEBUG messages\n");
  printf("                            e = ERROR messages\n");
  printf("                            i = INFO messages\n");
  printf("                            m = mapper DEBUG messages\n");
  printf("                            p = PPU DEBUG messages\n");
  printf("                            w = WARNING messages\n");
  printf("  \n");
  printf("  If neither -q nor -v are specified, the default log level of\n");
  printf("   INFO|WARNING|ERROR is used.\n");
}

int  init();
void exit();
void save(std::string& filename, hw::rom::Rom& rom);

int main(int argc, char* argv[]) {
  int         opt = 0;
  std::string filename;
  std::string save_filename;
  bool        allow_unofficial = true;

  static struct option long_options[] = {{"save", required_argument, nullptr, 's'},
                                         {"official", no_argument, nullptr, 'o'},
                                         {"quiet", no_argument, nullptr, 'q'},
                                         {"verbose", optional_argument, nullptr, 'v'},
                                         {"help", no_argument, nullptr, 'h'},
                                         {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "f:s:oqv::h", long_options, nullptr)) != -1) {
    switch (opt) {
      case 's':  // -s or --save
        save_filename = std::string(optarg);
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

  // Parse rom filename
  if (optind >= argc) {
    printUsage();
    return 1;
  }
  filename = std::string(argv[optind]);

  hw::rom::Rom rom;
  if (hw::rom::parseFromFile(filename, &rom)) {
    return 1;
  }

  if (rom.header.has_battery) {
    if (save_filename.empty()) {
      logger::log<logger::WARNING>("No save file specified, using '%08X.sav'\n", rom.crc);
      char dfl_filename[13];
      sprintf(dfl_filename, "%08X.sav", rom.crc);
      save_filename = std::string(dfl_filename);
    }

    // If savefile doesn't exist or is corrupted, overwrite it. Otherwise, load it to RAM
    std::fstream savefile(save_filename, std::ios::in | std::ios::binary | std::ios::ate);
    if (!savefile || savefile.tellg() != 0x2000) {
      logger::log<logger::INFO>("Creating new save file '%s'... ", save_filename.c_str());
      savefile.close();
      savefile.open(save_filename, std::ios::out | std::ios::binary | std::ios::trunc);
      savefile.write(reinterpret_cast<char*>(rom.expansion), 0x2000);
      logger::log<logger::INFO>("Done\n");
    } else {
      logger::log<logger::INFO>("Loading save data from file '%s'... ", save_filename.c_str());
      savefile.seekg(0, std::ios::beg);
      savefile.read(reinterpret_cast<char*>(rom.expansion), 0x2000);
      logger::log<logger::INFO>("Done\n");
    }
  } else if (!save_filename.empty()) {
    logger::log<logger::WARNING>("Save file '%s' specified, but '%s' does not support battery backup\n",
                                 save_filename.c_str(),
                                 filename.c_str());
  }

  // Initialize SDL
  if (init()) {
    exit();
    return 1;
  }

  // Create the windows
  windows["screen"] = new ui::Screen();
  if (windows["screen"]->init("NES Emu")) {
    exit();
    return 1;
  }
  windows["nt"] = new ui::NametableViewer();
  if (windows["nt"]->init("Nametable Viewer", false)) {
    exit();
    return 1;
  }
  windows["pt"] = new ui::PatternTableViewer();
  if (windows["pt"]->init("Pattern Table Viewer", false)) {
    exit();
    return 1;
  }
  windows["oam"] = new ui::SpriteViewer();
  if (windows["oam"]->init("Sprite Viewer", false)) {
    exit();
    return 1;
  }

  // Create the audio output device
  speaker.init();

  // Create the emulated hardware
  hw::console::Console console(allow_unofficial);
  console.loadCart(&rom);

  // Connect the emulated HW to the UI
  console.setScreen(static_cast<ui::Screen*>(windows["screen"]));
  console.setSpeaker(&speaker);
  static_cast<ui::NametableViewer*>(windows["nt"])->attachPPU(console.getPPU());
  static_cast<ui::PatternTableViewer*>(windows["pt"])->attachPPU(console.getPPU());
  static_cast<ui::SpriteViewer*>(windows["oam"])->attachPPU(console.getPPU());

  // Start the hardware
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

        // Emulator controls
        if (event.type == SDL_KEYDOWN) {
          switch (event.key.keysym.sym) {

            // Reset
            case SDLK_r:
              console.reset(true);
              break;

            // Unlock speed limit
            case SDLK_TAB:
              console.limitSpeed(false);
              break;

            // Show nametable viewer
            case SDLK_1:
              windows["nt"]->focus();
              break;

            // Show sprite viewer
            case SDLK_2:
              windows["oam"]->focus();
              break;

            // Show pattern table viewer
            case SDLK_3:
              windows["pt"]->focus();
              break;

            // Volume down
            case SDLK_LEFTBRACKET:
              speaker.addVolume(-0.1);
              break;

            // Volume up
            case SDLK_RIGHTBRACKET:
              speaker.addVolume(0.1);
              break;
          }
        } else if (event.type == SDL_KEYUP) {
          switch (event.key.keysym.sym) {

            // Release reset
            case SDLK_r:
              console.reset(false);
              break;


            // Relock speed limit
            case SDLK_TAB:
              console.limitSpeed(true);
              break;
          }
        }
      }

      // Render all visible windows
      for (auto&& window : windows) {
        window.second->update();
      }
    }
  }

  save(save_filename, rom);
  exit();
  return 0;
}

/// Initialize SDL subsystems
int init() {
  if (SDL_Init(SDL_INIT_VIDEO)) {
    logger::log<logger::ERROR>("Video initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  if (SDL_Init(SDL_INIT_AUDIO)) {
    logger::log<logger::ERROR>("Audio initialization failed: %s\n", SDL_GetError());
    return 1;
  }

  return 0;
}

/// Save to file
void save(std::string& file, hw::rom::Rom& rom) {
  if (rom.header.has_battery) {
    logger::log<logger::INFO>("Saving to file '%s'... ", file.c_str());
    std::ofstream savefile(file, std::ios::out | std::ios::binary | std::ios::trunc);
    savefile.write(reinterpret_cast<char*>(rom.expansion), 0x2000);
    logger::log<logger::INFO>("Done\n");
  }
}

/// Cleanup windows and shutdown SDL subsystems
void exit() {
  logger::log<logger::INFO>("Shutting down... ");

  // Cleanup all windows
  for (auto&& window : windows) {
    window.second->close();
    delete window.second;
  }

  speaker.close();

  // Quit SDL subsystems
  SDL_Quit();

  logger::log<logger::INFO>("Done\n");
}
