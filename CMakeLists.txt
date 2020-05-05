cmake_minimum_required(VERSION 3.0.2)
project(nesemu)

# Set C++ version
set(CMAKE_CXX_STANDARD 11)
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
  src/apu.cpp
  src/console.cpp
  src/cpu.cpp
  src/joystick.cpp
  src/nesemu.cpp
  src/ppu.cpp
  src/rom.cpp
  src/window.cpp
)
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:-g>")
target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:RELEASE>:-O3>")


#############
## Install ##
#############

# Install targets
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
