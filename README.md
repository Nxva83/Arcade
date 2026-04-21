# Arcade

Arcade project in C++17 with a dynamic modular architecture:
- one core executable: arcade
- dynamically loaded graphics libraries
- dynamically loaded games

The program loads .so modules from lib/, lets you switch graphics libraries and games at runtime, and stores scores in assets/scores.txt.

## Table of Contents

1. Description
2. Features
3. Architecture
4. Prerequisites
5. Build
6. Run
7. Keyboard Controls
8. Included Modules
9. Add a Module
10. Project Structure

## Description

The core (Core) does not contain rendering logic or game logic.
It relies on two interfaces:
- IDisplayModule for rendering
- IGameModule for games

Modules are automatically discovered in lib/.
The menu is a special game (arcade_menu.so) used as a hub:
- player name selection
- game selection
- score display

## Features

- Dynamic module loading with dlopen/dlsym
- Real-time graphics library switching
- Real-time game switching
- Interactive menu
- Score persistence (assets/scores.txt)
- Extended game API (context, requests, player name)

## Architecture


- Core: src/core/Core.cpp
- Dynamic loader: src/DLLoader.hpp
- Interfaces:
	- include/IDisplayModule.hpp
	- include/IGameModule.hpp
	- include/Types.hpp

The core scans lib/ and classifies .so files as:
- graphics modules
- game modules

Recognized display entry points:
- createDisplayModule
- entryPointDisplay
- createDisplay
- entryPoint

Recognized game entry points:
- createGameModule
- entryPointGame
- createGame
- entryPoint

If a module exposes arcade_game_api_version and returns at least 2, the core enables advanced features:
- setContext(GameContext)
- pullRequest()
- getPlayerName()

## Prerequisites

Target platform: Linux (or WSL) with a C++17 toolchain.

Minimum dependencies:
- cmake (>= 3.16)
- g++
- make
- ncurses
- SFML (graphics, window, system)

Optional dependencies:
- OpenGL: gl, glew, glfw3 (via pkg-config)
- SDL2: sdl2, SDL2_ttf (via pkg-config)

Debian/Ubuntu example:

```bash
sudo apt update
sudo apt install -y \
	build-essential cmake pkg-config \
	libncurses-dev \
	libsfml-dev \
	libgl1-mesa-dev libglew-dev libglfw3-dev \
	libsdl2-dev libsdl2-ttf-dev
```

## Build

From the project root:

```bash
cmake -S . -B build
cmake --build build -j
```

Expected output:
- executable: ./arcade
- libraries: ./lib/arcade_*.so

Available CMake options:

```bash
cmake -S . -B build -DARCADE_BUILD_OPENGL=ON -DARCADE_BUILD_SDL2=ON
```

You can disable optional modules:

```bash
cmake -S . -B build -DARCADE_BUILD_OPENGL=OFF -DARCADE_BUILD_SDL2=OFF
```

## Run

The program expects the initial graphics library as argument:

```bash
./arcade ./lib/arcade_ncurses.so
```

Other examples:

```bash
./arcade ./lib/arcade_sfml.so
./arcade ./lib/arcade_sdl2.so
./arcade ./lib/arcade_opengl.so
```

If the argument is not a valid graphics library, the program returns an error code.

## Keyboard Controls

System controls (global):
- F1: next graphics library
- F2: previous graphics library
- F3: next game
- F4: previous game
- F5: restart current game
- F6: go to menu
- F7: quit
- ESC: quit

Common in-game controls:
- Arrow keys: movement
- Enter / Space: main action (depends on game)
- F: flag in Minesweeper

In the menu:
- A-Z: enter player name
- Backspace: erase
- Up/Down arrows: game selection
- Enter: start game

## Included Modules

Graphics:
- arcade_ncurses
- arcade_sfml
- arcade_sdl2 (if dependencies are installed)
- arcade_opengl (if dependencies are installed)

Games:
- arcade_menu
- arcade_snake
- arcade_minesweeper
- arcade_centipede
- arcade_donkey_kong

## Add a Module

### Add a graphics library

1. Implement the IDisplayModule interface.
2. Export a compatible entry point (for example: entryPointDisplay).
3. Build as a shared library and output a file named like arcade_xxx.so in lib/.

### Add a game

1. Implement the IGameModule interface.
2. Export a compatible entry point (for example: entryPointGame).
3. Optional: export arcade_game_api_version() and return 2 for the extended API.
4. Build as a shared library and output a file named like arcade_xxx.so in lib/.

## Project Structure

```text
.
|- CMakeLists.txt
|- include/
|  |- IDisplayModule.hpp
|  |- IGameModule.hpp
|  `- Types.hpp
|- src/
|  |- DLLoader.hpp
|  |- core/
|  |  |- Core.cpp
|  |  |- Core.hpp
|  |  `- main.cpp
|  |- graphics/
|  |  |- ncurses/
|  |  |- sfml/
|  |  |- sdl2/
|  |  `- opengl/
|  `- games/
|     |- menu/
|     |- snake/
|     |- minesweeper/
|     |- centipede/
|     `- donkey_kong/
|- assets/
`- lib/
```

## Notes

- The project relies on .so files and the dlopen API, so a Linux environment is recommended.
- The assets/scores.txt file is created/updated automatically during gameplay.