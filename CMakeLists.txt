cmake_minimum_required(VERSION 3.0.2)
project(nesemu)

# Set C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

option(DEBUG "Enable DEBUG mode" OFF)
if (DEBUG)
  add_definitions(-DDEBUG=1)
endif()


# Load SDL2
find_package(SDL2 REQUIRED)


###########
## Build ##
###########

# Specify header include paths
include_directories(include ${SDL2_INCLUDE_DIRS})

add_executable(${PROJECT_NAME}
  src/hw/apu/apu.cpp
  src/hw/apu/channels/dmc.cpp
  src/hw/apu/channels/noise.cpp
  src/hw/apu/channels/square.cpp
  src/hw/apu/channels/triangle.cpp
  src/hw/apu/units.cpp
  src/hw/console.cpp
  src/hw/cpu.cpp
  src/hw/joystick.cpp
  src/hw/ppu.cpp
  src/hw/rom.cpp
  src/hw/system_bus.cpp
  src/logger.cpp
  src/ui/nametable_viewer.cpp
  src/ui/pattern_table_viewer.cpp
  src/ui/screen.cpp
  src/ui/speaker.cpp
  src/ui/sprite_viewer.cpp
  src/ui/window.cpp
  src/nesemu.cpp
)
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror=switch)
target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:-g>")
target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:RELEASE>:-O3>")


#############
## Install ##
#############

# Install targets
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
