cmake_minimum_required(VERSION 3.16)
project(arcade LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -fno-gnu-unique)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(arcade
    src/core/main.cpp
    src/core/Core.cpp
)
target_link_libraries(arcade PRIVATE dl)

add_library(arcade_ncurses SHARED
    src/graphics/ncurses/NcursesDisplay.cpp
)
set_target_properties(arcade_ncurses PROPERTIES
    OUTPUT_NAME arcade_ncurses
    PREFIX ""
)
target_link_libraries(arcade_ncurses PRIVATE ncurses)

option(ARCADE_BUILD_OPENGL "Build OpenGL display module" ON)

if(ARCADE_BUILD_OPENGL)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(OPENGL_DEPS QUIET IMPORTED_TARGET gl glew glfw3)
    endif()

    if(OPENGL_DEPS_FOUND)
        add_library(arcade_opengl SHARED
            src/graphics/opengl/OpenGlDisplay.cpp
        )
        set_target_properties(arcade_opengl PROPERTIES
            OUTPUT_NAME arcade_opengl
            PREFIX ""
        )
        target_link_libraries(arcade_opengl PRIVATE PkgConfig::OPENGL_DEPS)
    else()
        message(WARNING "OpenGL module disabled: missing dependencies (pkg-config modules: gl, glew, glfw3). Install libgl1-mesa-dev libglew-dev libglfw3-dev to enable.")
    endif()
endif()

option(ARCADE_BUILD_SDL2 "Build SDL2 display module" ON)

if(ARCADE_BUILD_SDL2)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(SDL2_DEPS QUIET IMPORTED_TARGET sdl2 SDL2_ttf)
    endif()

    if(SDL2_DEPS_FOUND)
        add_library(arcade_sdl2 SHARED
            src/graphics/sdl2/SDL2Display.cpp
        )
        set_target_properties(arcade_sdl2 PROPERTIES
            OUTPUT_NAME arcade_sdl2
            PREFIX ""
        )
        target_link_libraries(arcade_sdl2 PRIVATE PkgConfig::SDL2_DEPS)
    else()
        message(WARNING "SDL2 module disabled: missing dependencies (pkg-config modules: sdl2, SDL2_ttf). Install libsdl2-dev and libsdl2-ttf-dev to enable.")
    endif()
endif()

add_library(arcade_sfml SHARED
    src/graphics/sfml/SFMLDisplay.cpp
)
set_target_properties(arcade_sfml PROPERTIES
    OUTPUT_NAME arcade_sfml
    PREFIX ""
)

target_link_libraries(arcade_sfml PRIVATE sfml-graphics sfml-window sfml-system)

add_library(arcade_snake SHARED
    src/games/snake/SnakeGame.cpp
)
set_target_properties(arcade_snake PROPERTIES
    OUTPUT_NAME arcade_snake
    PREFIX ""
)

add_library(arcade_menu SHARED
    src/games/menu/MenuGame.cpp
)
set_target_properties(arcade_menu PROPERTIES
    OUTPUT_NAME arcade_menu
    PREFIX ""
)

add_library(arcade_minesweeper SHARED
    src/games/minesweeper/MinesweeperGame.cpp
)


set_target_properties(arcade_minesweeper PROPERTIES
    OUTPUT_NAME arcade_minesweeper
    PREFIX ""
)

add_library(arcade_centipede SHARED
    src/games/centipede/Centipede.cpp
)

set_target_properties(arcade_centipede PROPERTIES
    OUTPUT_NAME arcade_centipede
    PREFIX ""
)

add_library(arcade_donkey_kong SHARED
    src/games/donkey_kong/donkey_kongGame.cpp
)

set_target_properties(arcade_donkey_kong PROPERTIES
    OUTPUT_NAME arcade_donkey_kong
    PREFIX ""
)
