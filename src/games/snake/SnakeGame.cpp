#include <deque>
#include <cstdlib>
#include <ctime>
#include <string>
#include "../../../include/IGameModule.hpp"

class SnakeGame : public Arcade::IGameModule {
public:
    SnakeGame() : _name("Snake"), _gridW(20), _gridH(20),
                  _score(0), _gameOver(false) {}

    void init() override
    {
        std::srand(std::time(nullptr));
        reset();
    }

    void reset() override
    {
        _snake.clear();
        _direction = Direction::RIGHT;
        _nextDirection = Direction::RIGHT;
        _score = 0;
        _gameOver = false;
        int midX = _gridW / 2;
        int midY = _gridH / 2;
        for (int i = 0; i < 4; i++)
            _snake.push_back({midX - i, midY});
        spawnFood();
    }

    void close() override {}

    const std::string &getName() const override { return _name; }

    void handleInput(Arcade::Key key) override
    {
        switch (key) {
            case Arcade::Key::UP:
                if (_direction != Direction::DOWN)
                    _nextDirection = Direction::UP;
                break;
            case Arcade::Key::DOWN:
                if (_direction != Direction::UP)
                    _nextDirection = Direction::DOWN;
                break;
            case Arcade::Key::LEFT:
                if (_direction != Direction::RIGHT)
                    _nextDirection = Direction::LEFT;
                break;
            case Arcade::Key::RIGHT:
                if (_direction != Direction::LEFT)
                    _nextDirection = Direction::RIGHT;
                break;
            default: break;
        }
    }

    void update() override
    {
        if (_gameOver) return;
        _direction = _nextDirection;
        auto head = _snake.front();
        switch (_direction) {
            case Direction::UP: head.second--; break;
            case Direction::DOWN: head.second++; break;
            case Direction::LEFT: head.first--; break;
            case Direction::RIGHT: head.first++; break;
        }
        if (head.first <= 0 || head.first >= _gridW - 1 ||
            head.second <= 0 || head.second >= _gridH - 1) {
            _gameOver = true;
            return;
        }
        for (const auto &seg : _snake) {
            if (seg.first == head.first && seg.second == head.second) {
                _gameOver = true;
                return;
            }
        }
        _snake.push_front(head);
        if (head.first == _food.first && head.second == _food.second) {
            _score += 10;
            spawnFood();
        } else {
            _snake.pop_back();
        }
    }

    const Arcade::RenderData &getRenderData() override
    {
        _renderData.gridWidth = _gridW;
        _renderData.gridHeight = _gridH;
        _renderData.grid.assign(_gridH,
            std::vector<Arcade::Cell>(_gridW,
                {' ', Arcade::Color::WHITE, Arcade::Color::BLACK}));
        for (int x = 0; x < _gridW; x++) {
            _renderData.grid[0][x] = {'#', Arcade::Color::WHITE, Arcade::Color::WHITE};
            _renderData.grid[_gridH - 1][x] = {'#', Arcade::Color::WHITE, Arcade::Color::WHITE};
        }
        for (int y = 0; y < _gridH; y++) {
            _renderData.grid[y][0] = {'#', Arcade::Color::WHITE, Arcade::Color::WHITE};
            _renderData.grid[y][_gridW - 1] = {'#', Arcade::Color::WHITE, Arcade::Color::WHITE};
        }
        _renderData.grid[_food.second][_food.first] =
            {'*', Arcade::Color::RED, Arcade::Color::BLACK};
        for (size_t i = 0; i < _snake.size(); i++) {
            auto &s = _snake[i];
            if (i == 0)
                _renderData.grid[s.second][s.first] =
                    {'O', Arcade::Color::GREEN, Arcade::Color::BLACK};
            else
                _renderData.grid[s.second][s.first] =
                    {'o', Arcade::Color::GREEN, Arcade::Color::BLACK};
        }
        _renderData.texts.clear();
        _renderData.texts.push_back(
            {"Score: " + std::to_string(_score),
             _gridW + 1, 1, Arcade::Color::WHITE});
        if (_gameOver)
            _renderData.texts.push_back(
                {"GAME OVER", _gridW + 1, 3, Arcade::Color::RED});
        return _renderData;
    }

    int getScore() const override { return _score; }
    bool isGameOver() const override { return _gameOver; }

private:
    enum class Direction { UP, DOWN, LEFT, RIGHT };
    std::string _name;
    int _gridW, _gridH;
    int _score;
    bool _gameOver;
    Direction _direction;
    Direction _nextDirection;
    std::deque<std::pair<int, int>> _snake;
    std::pair<int, int> _food;
    Arcade::RenderData _renderData;

    void spawnFood()
    {
        while (true) {
            int x = 1 + std::rand() % (_gridW - 2);
            int y = 1 + std::rand() % (_gridH - 2);
            bool onSnake = false;
            for (const auto &s : _snake)
                if (s.first == x && s.second == y) { onSnake = true; break; }
            if (!onSnake) {
                _food = {x, y};
                return;
            }
        }
    }
};

extern "C" Arcade::IGameModule *entryPointGame()
{
    return new SnakeGame();
}

extern "C" int arcade_game_api_version()
{
    return 2;
}
