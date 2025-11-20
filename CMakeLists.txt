cmake_minimum_required(VERSION 3.10)
project(TankStorm)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SFML 2.5 COMPONENTS graphics window system audio network REQUIRED)

include_directories(header)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(TankStorm ${SOURCES})

target_link_libraries(TankStorm sfml-graphics sfml-window sfml-system sfml-audio sfml-network)

# Copy resources to build directory
add_custom_command(TARGET TankStorm POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/res $<TARGET_FILE_DIR:TankStorm>/res
    COMMENT "Copying resources to build directory"
)
