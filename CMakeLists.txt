cmake_minimum_required(VERSION 3.16)
project(TankStorm C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# raylib récupéré et compilé automatiquement
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(tankstorm
    src/main.c
    src/terrain.c
    src/tank.c
    src/rocket.c
    src/bot.c
    src/crate.c
    src/fx.c
    src/map.c
    src/editor.c
)
target_include_directories(tankstorm PRIVATE src)
target_link_libraries(tankstorm PRIVATE raylib)

if(MSVC)
    target_compile_definitions(tankstorm PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(tankstorm PRIVATE /utf-8)   # chaînes accentuées
endif()
