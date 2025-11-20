cmake_minimum_required(VERSION 3.10)
project(TankStorm)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SFML 2.5 COMPONENTS graphics window system audio network REQUIRED)

include_directories(header)

set(SOURCES
    src/main.cpp
    src/menu.cpp
    src/play.cpp
    src/tank.cpp
    src/button.cpp
    src/tool.cpp
    src/terrain.cpp
    src/missile.cpp
    src/config.cpp
    src/input_manager.cpp
    src/keyboard_player.cpp
    src/settings.cpp
)

add_executable(TankStorm ${SOURCES})

target_link_libraries(TankStorm sfml-graphics sfml-window sfml-system sfml-audio sfml-network)

# Copy resources to build directory
add_custom_command(TARGET TankStorm POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/res $<TARGET_FILE_DIR:TankStorm>/res
    COMMENT "Copying resources to build directory"
)
