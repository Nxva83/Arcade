#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>
#include "../../../include/IGameModule.hpp"

class MinesweeperGame : public Arcade::IGameModule {
public:
    MinesweeperGame() : _name("Minesweeper"), _gridW(16), _gridH(16),
                        _totalMines(40), _cursorX(0), _cursorY(0),
                        _score(0), _gameOver(false), _won(false),
                        _firstReveal(true) {}

    void init() override
    {
        std::srand(std::time(nullptr));
        reset();
    }

    void reset() override
    {
        _cursorX = _gridW / 2;
        _cursorY = _gridH / 2;
        _score = 0;
        _gameOver = false;
        _won = false;
        _firstReveal = true;
        _field.assign(_gridH, std::vector<MineCell>(_gridW, {false, false, false, 0}));
    }

    void close() override {}

    const std::string &getName() const override { return _name; }

    void handleInput(Arcade::Key key) override
    {
        if (_gameOver) return;
        switch (key) {
            case Arcade::Key::UP:
                if (_cursorY > 0) _cursorY--;
                break;
            case Arcade::Key::DOWN:
                if (_cursorY < _gridH - 1) _cursorY++;
                break;
            case Arcade::Key::LEFT:
                if (_cursorX > 0) _cursorX--;
                break;
            case Arcade::Key::RIGHT:
                if (_cursorX < _gridW - 1) _cursorX++;
                break;
            case Arcade::Key::SPACE: case Arcade::Key::ENTER: case Arcade::Key::MOUSE_LEFT:
                revealCell(_cursorX, _cursorY);
                break;
            case Arcade::Key::KEY_F: case Arcade::Key::MOUSE_RIGHT:
                toggleFlag(_cursorX, _cursorY);
                break;
            default: break;
        }
    }

    void update() override
    {
        if (_gameOver) return;
        int unrevealed = 0;
        for (int y = 0; y < _gridH; y++)
            for (int x = 0; x < _gridW; x++)
                if (!_field[y][x].revealed && !_field[y][x].hasMine)
                    unrevealed++;
        if (unrevealed == 0) {
            _won = true;
            _gameOver = true;
            _score += 100;
        }
    }

    const Arcade::RenderData &getRenderData() override
    {
        _renderData.gridWidth = _gridW + 20;
        _renderData.gridHeight = _gridH + 2;
        _renderData.grid.assign(_renderData.gridHeight,
            std::vector<Arcade::Cell>(_renderData.gridWidth,
                {' ', Arcade::Color::WHITE, Arcade::Color::BLACK}));
        for (int y = 0; y < _gridH; y++) {
            for (int x = 0; x < _gridW; x++) {
                Arcade::Cell cell;
                bool isCursor = (x == _cursorX && y == _cursorY);
                auto &mc = _field[y][x];
                if (mc.revealed) {
                    if (mc.hasMine) {
                        cell = {'*', Arcade::Color::RED, Arcade::Color::BLACK};
                    } else if (mc.adjacentMines == 0) {
                        cell = {'0', Arcade::Color::WHITE,
                                isCursor ? Arcade::Color::BLUE : Arcade::Color::BLACK};
                    } else {
                        char ch = '0' + mc.adjacentMines;
                        Arcade::Color numColor;
                        switch (mc.adjacentMines) {
                            case 1: numColor = Arcade::Color::BLUE; break;
                            case 2: numColor = Arcade::Color::GREEN; break;
                            case 3: numColor = Arcade::Color::RED; break;
                            case 4: numColor = Arcade::Color::MAGENTA; break;
                            default: numColor = Arcade::Color::YELLOW; break;
                        }
                        cell = {ch, numColor,
                                isCursor ? Arcade::Color::BLUE : Arcade::Color::BLACK};
                    }
                } else if (mc.flagged) {
                    cell = {'F', Arcade::Color::YELLOW,
                            isCursor ? Arcade::Color::BLUE : Arcade::Color::RED};
                } else {
                    cell = {'#', Arcade::Color::WHITE,
                            isCursor ? Arcade::Color::BLUE : Arcade::Color::WHITE};
                }
                _renderData.grid[y + 1][x + 1] = cell;
            }
        }
        _renderData.texts.clear();
        int flagCount = 0;
        for (int y = 0; y < _gridH; y++)
            for (int x = 0; x < _gridW; x++)
                if (_field[y][x].flagged) flagCount++;
        _renderData.texts.push_back(
            {"MINESWEEPER", 1, 0, Arcade::Color::CYAN});
        _renderData.texts.push_back(
            {"Mines: " + std::to_string(_totalMines - flagCount),
             _gridW + 3, 1, Arcade::Color::WHITE});
        _renderData.texts.push_back(
            {"Score: " + std::to_string(_score),
             _gridW + 3, 2, Arcade::Color::WHITE});
        _renderData.texts.push_back(
            {"[SPACE] Reveal", _gridW + 3, 4, Arcade::Color::CYAN});
        _renderData.texts.push_back(
            {"[F] Flag", _gridW + 3, 5, Arcade::Color::CYAN});
        _renderData.texts.push_back(
            {"[Arrows] Move", _gridW + 3, 6, Arcade::Color::CYAN});
        if (_gameOver) {
            if (_won)
                _renderData.texts.push_back(
                    {"YOU WIN!", _gridW + 3, 8, Arcade::Color::GREEN});
            else
                _renderData.texts.push_back(
                    {"GAME OVER!", _gridW + 3, 8, Arcade::Color::RED});
        }
        return _renderData;
    }

    int getScore() const override { return _score; }
    bool isGameOver() const override { return _gameOver; }

private:
    struct MineCell {
        bool hasMine;
        bool revealed;
        bool flagged;
        int adjacentMines;
    };

    std::string _name;
    int _gridW, _gridH;
    int _totalMines;
    int _cursorX, _cursorY;
    int _score;
    bool _gameOver;
    bool _won;
    bool _firstReveal;
    std::vector<std::vector<MineCell>> _field;
    Arcade::RenderData _renderData;

    void placeMines(int safeX, int safeY)
    {
        int placed = 0;
        while (placed < _totalMines) {
            int x = std::rand() % _gridW;
            int y = std::rand() % _gridH;
            if (_field[y][x].hasMine) continue;
            if (std::abs(x - safeX) <= 1 && std::abs(y - safeY) <= 1) continue;
            _field[y][x].hasMine = true;
            placed++;
        }
        for (int y = 0; y < _gridH; y++)
            for (int x = 0; x < _gridW; x++)
                _field[y][x].adjacentMines = countAdjacentMines(x, y);
    }

    int countAdjacentMines(int cx, int cy) const
    {
        int count = 0;
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                int nx = cx + dx, ny = cy + dy;
                if (nx >= 0 && nx < _gridW && ny >= 0 && ny < _gridH)
                    if (_field[ny][nx].hasMine) count++;
            }
        return count;
    }

    void revealCell(int x, int y)
    {
        if (x < 0 || x >= _gridW || y < 0 || y >= _gridH) return;
        if (_field[y][x].revealed || _field[y][x].flagged) return;
        if (_firstReveal) {
            placeMines(x, y);
            _firstReveal = false;
        }
        _field[y][x].revealed = true;
        if (_field[y][x].hasMine) {
            revealAllMines();
            _gameOver = true;
            return;
        }
        _score += 1;
        if (_field[y][x].adjacentMines == 0) {
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++)
                    if (dx != 0 || dy != 0)
                        revealCell(x + dx, y + dy);
        }
    }

    void toggleFlag(int x, int y)
    {
        if (x < 0 || x >= _gridW || y < 0 || y >= _gridH) return;
        if (_field[y][x].revealed) return;
        _field[y][x].flagged = !_field[y][x].flagged;
    }

    void revealAllMines()
    {
        for (int y = 0; y < _gridH; y++)
            for (int x = 0; x < _gridW; x++)
                if (_field[y][x].hasMine)
                    _field[y][x].revealed = true;
    }
};

extern "C" Arcade::IGameModule *entryPointGame()
{
    return new MinesweeperGame();
}

extern "C" int arcade_game_api_version()
{
    return 2;
}
