# TankStorm

A C++/SFML reimplementation of Rocket Bot Royale featuring destructible terrain and physics-based tank combat.

## Project Goals

This project aims to recreate Rocket Bot Royale with:
- **Client**: C++ with SFML for graphics and input
- **Server**: Rust (planned) for multiplayer networking
- **Performance**: Optimized for smooth gameplay with destructible environments

## Features

- 🎮 Tank movement and rotation
- 🚀 Missile shooting with physics simulation
- 💥 Destructible 2D terrain
- 🌍 Gravity and falling mechanics
- 🎯 Pixel-perfect collision detection

## Build Instructions

### Prerequisites

- CMake 3.10+
- C++17 compatible compiler (GCC, Clang, MSVC)
- SFML 2.5+

### Linux

```bash
# Install SFML
sudo apt-get install libsfml-dev

# Build
mkdir build && cd build
cmake ..
make

# Run
./TankStorm
```

### Command Line Arguments

- `--size [width] [height]`: Set window size (default: 640x480)
- `--fps [fps]`: Set frame rate limit (default: 60)
- `--screen [id]`: Set screen ID for multi-monitor setups

Example:
```bash
./TankStorm --size 1280 720 --fps 60
```

## Controls

- **Arrow Left/Right**: Move tank
- **Mouse**: Aim cannon
- **Space**: Shoot missile
- **Escape**: Exit to menu/quit

## Project Structure

```
RBR/
├── header/          # Header files
├── src/             # Source files
├── res/             # Resources (fonts, images)
├── build/           # Build directory (generated)
└── CMakeLists.txt   # CMake configuration
```

## Development Status

- [x] C++/SFML migration complete
- [x] Basic tank movement and rendering
- [ ] Destructible terrain system
- [ ] Missile physics
- [ ] Multiplayer networking (Rust server)

## License

See [LICENSE](LICENSE) file for details.
