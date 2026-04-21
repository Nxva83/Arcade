#ifndef ARCADE_TYPES_HPP_
#define ARCADE_TYPES_HPP_

#include <cstddef>
#include <string>
#include <vector>

namespace Arcade {

    enum class Key {
        UP, DOWN, LEFT, RIGHT,
        ENTER, BACKSPACE, SPACE, ESCAPE,
        KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G,
        KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N,
        KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U,
        KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
        NEXT_LIB, PREV_LIB,
        NEXT_GAME, PREV_GAME,
        RESTART, MENU, QUIT,
        MOUSE_LEFT, MOUSE_RIGHT,
        NONE
    };

    enum class Color {
        BLACK, RED, GREEN, YELLOW,
        BLUE, MAGENTA, CYAN, WHITE
    };

    struct Cell {
        char character;
        Color fgColor;
        Color bgColor;
    };

    struct Text {
        std::string content;
        int x;
        int y;
        Color color;
    };

    struct RenderData {
        int gridWidth;
        int gridHeight;
        std::vector<std::vector<Cell>> grid;
        std::vector<Text> texts;
    };

    struct ScoreEntry {
        std::string playerName;
        std::string gameName;
        int score;
    };

    struct GameContext {
        std::vector<std::string> gameLibPaths;
        std::vector<std::string> graphicsLibPaths;
        std::size_t activeGraphicsIndex = 0;
        std::vector<ScoreEntry> scores;
        std::string playerName;
    };

    enum class GameRequestType {
        NONE,
        START_GAME,
        MENU,
        QUIT
    };

    struct GameRequest {
        GameRequestType type = GameRequestType::NONE;
        std::string gameLibPath;
    };

}

#endif
