#include "IGameModule.hpp"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>

namespace Arcade {

static const int GRID_W = 20;
static const int GRID_H = 30;
static const int PLAYER_ZONE_Y = static_cast<int>(GRID_H * 0.8f);
static const int NB_OBSTACLES = 20;
static const int OBSTACLE_HP = 5;
static const int WIN_COUNT = 20;

struct Segment { int x, y; };

struct Centipede {
    std::vector<Segment> body;
    int dirX;
};

struct Shot {
    float x, y;
    bool  active;
};

struct Obstacle {
    int x, y, hp;
};

class CentipedeGame : public IGameModule {
public:
    CentipedeGame()
        : _name("Centipede"), _playerX(0), _playerY(0),
          _shot{0.f, 0.f, false},
          _score(0), _gameOver(false), _won(false),
          _tickCount(0), _centipedesKilled(0)
    {}
    ~CentipedeGame() override = default;

    void init() override
    {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        reset();
    }

    void reset() override
    {
        _score = 0;
        _gameOver = false;
        _won = false;
        _tickCount = 0;
        _centipedesKilled = 0;
        _playerX = GRID_W / 2;
        _playerY = PLAYER_ZONE_Y + 2;
        _shot = {0.f, 0.f, false};
        _obstacles.clear();
        for (int i = 0; i < NB_OBSTACLES; i++) {
            Obstacle o;
            o.x  = std::rand() % GRID_W;
            o.y  = 1 + std::rand() % (PLAYER_ZONE_Y - 2);
            o.hp = OBSTACLE_HP;
            _obstacles.push_back(o);
        }
        _centipedes.clear();
        spawnCentipede();
        buildRenderData();
    }

    void close() override {}

    const std::string &getName()  const override { return _name; }
    int getScore()  const override { return _score; }
    bool isGameOver() const override { return _gameOver; }

    void handleInput(Key key) override
    {
        if (_gameOver) return;
        switch (key) {
            case Key::LEFT:
                if (_playerX > 0) _playerX--;
                break;
            case Key::RIGHT:
                if (_playerX < GRID_W - 1) _playerX++;
                break;
            case Key::UP:
                if (_playerY > PLAYER_ZONE_Y) _playerY--;
                break;
            case Key::DOWN:
                if (_playerY < GRID_H - 1) _playerY++;
                break;
            case Key::SPACE:
                if (!_shot.active) {
                    _shot = {static_cast<float>(_playerX),
                             static_cast<float>(_playerY - 1), true};
                }
                break;
            default: break;
        }
    }

    void update() override
    {
        if (_gameOver) return;

        _tickCount++;
        if (_shot.active) {
            _shot.y -= 1.5f;
            if (_shot.y < 0) {
                _shot.active = false;
            } else {
                checkShotObstacle();
                checkShotCentipede();
            }
        }

        if (_tickCount >= 5) {
            _tickCount = 0;
            for (auto &c : _centipedes)
                moveCentipede(c);
            for (const auto &c : _centipedes) {
                if (c.body.empty()) continue;
                const Segment &h = c.body.front();
                if (h.x == _playerX && h.y == _playerY)
                    _gameOver = true;
            }
            if (_centipedes.empty() && !_gameOver) {
                if (_centipedesKilled >= WIN_COUNT) {
                    _won = _gameOver = true;
                } else {
                    spawnCentipede();
                }
            }
        }
        buildRenderData();
    }
    const RenderData &getRenderData() override { return _renderData; }
private:

    void spawnCentipede()
    {
        Centipede c;
        c.dirX = 1;
        for (int i = 0; i < 10; i++)
            c.body.push_back({i, 0});
        _centipedes.push_back(c);
    }

    void moveCentipede(Centipede &c)
    {
        if (c.body.empty()) return;
        Segment &head  = c.body.front();
        int nextX = head.x + c.dirX;

        bool blocked = (nextX < 0 || nextX >= GRID_W
                        || hasObstacle(nextX, head.y));
        if (blocked) {
            c.dirX = -c.dirX;
            for (int i = (int)c.body.size() - 1; i > 0; i--)
                c.body[i] = c.body[i - 1];
            c.body[0].y += 1;
        } else {
            for (int i = (int)c.body.size() - 1; i > 0; i--)
                c.body[i] = c.body[i - 1];
            c.body[0].x = nextX;
        }
        if (c.body.front().y >= GRID_H)
            _gameOver = true;
    }

    bool hasObstacle(int x, int y) const
    {
        for (const auto &o : _obstacles)
            if (o.x == x && o.y == y) return true;
        return false;
    }

    void checkShotObstacle()
    {
        int sx = static_cast<int>(_shot.x);
        int sy = static_cast<int>(_shot.y);
        for (auto it = _obstacles.begin(); it != _obstacles.end(); ++it) {
            if (it->x == sx && it->y == sy) {
                it->hp--;
                _shot.active = false;
                if (it->hp <= 0) _obstacles.erase(it);
                return;
            }
        }
    }

    void checkShotCentipede()
    {
        int sx = static_cast<int>(_shot.x);
        int sy = static_cast<int>(_shot.y);
        for (std::size_t centIdx = 0; centIdx < _centipedes.size(); ++centIdx) {
            auto &cent = _centipedes[centIdx];
            for (int i = 0; i < (int)cent.body.size(); i++) {
                if (cent.body[i].x == sx && cent.body[i].y == sy) {
                    _shot.active = false;
                    _score += 10;
                    _obstacles.push_back({sx, sy, OBSTACLE_HP});

                    Centipede tail;
                    bool hasTail = false;
                    if (i + 1 < (int)cent.body.size()) {
                        tail.dirX = -cent.dirX;
                        for (int j = i + 1; j < (int)cent.body.size(); j++)
                            tail.body.push_back(cent.body[j]);
                        hasTail = !tail.body.empty();
                    }

                    cent.body.resize(i);
                    if (cent.body.empty()) {
                        _centipedesKilled++;
                        _score += 100;
                        _centipedes.erase(_centipedes.begin() + (std::ptrdiff_t)centIdx);
                    }

                    if (hasTail) {
                        _centipedes.push_back(tail);
                    }
                    return;
                }
            }
        }
    }

    void buildRenderData()
    {
        _renderData.gridWidth  = GRID_W;
        _renderData.gridHeight = GRID_H;
        _renderData.grid.assign(GRID_H,
            std::vector<Cell>(GRID_W, {' ', Color::WHITE, Color::BLACK}));
        for (int x = 0; x < GRID_W; x++)
            _renderData.grid[PLAYER_ZONE_Y][x] =
                {'-', Color::CYAN, Color::BLACK};
        for (const auto &o : _obstacles) {
            Color c = (o.hp == OBSTACLE_HP) ? Color::WHITE :
                      (o.hp >= 3)           ? Color::YELLOW : Color::RED;
            _renderData.grid[o.y][o.x] = {'#', c, Color::BLACK};
        }
        for (const auto &cent : _centipedes) {
            for (int i = 0; i < (int)cent.body.size(); i++) {
                const Segment &s = cent.body[i];
                if (s.y < 0 || s.y >= GRID_H || s.x < 0 || s.x >= GRID_W)
                    continue;
                _renderData.grid[s.y][s.x] = (i == 0)
                    ? Cell{'@', Color::CYAN,  Color::BLACK}
                    : Cell{'o', Color::BLUE,  Color::BLACK};
            }
        }
        if (_shot.active) {
            int sy = static_cast<int>(_shot.y);
            int sx = static_cast<int>(_shot.x);
            if (sy >= 0 && sy < GRID_H && sx >= 0 && sx < GRID_W)
                _renderData.grid[sy][sx] = {'|', Color::YELLOW, Color::BLACK};
        }
        _renderData.grid[_playerY][_playerX] = {'^', Color::GREEN, Color::BLACK};
        _renderData.texts.clear();
        _renderData.texts.push_back(
            {"Score: " + std::to_string(_score), GRID_W + 1, 1, Color::WHITE});
        _renderData.texts.push_back(
            {"Kills: " + std::to_string(_centipedesKilled) +
             "/" + std::to_string(WIN_COUNT), GRID_W + 1, 2, Color::CYAN});
        _renderData.texts.push_back(
            {"[ARROWS] Move  [SPACE] Fire", GRID_W + 1, 4, Color::WHITE});
        if (_gameOver)
            _renderData.texts.push_back(
                {_won ? "YOU WIN!" : "GAME OVER",
                 GRID_W + 1, 6, _won ? Color::GREEN : Color::RED});
    }

    std::string _name;
    int _playerX, _playerY;
    Shot _shot;
    std::vector<Centipede> _centipedes;
    std::vector<Obstacle>  _obstacles;
    int _score;
    bool _gameOver, _won;
    int _tickCount;
    int _centipedesKilled;
    RenderData _renderData;
};
}

extern "C" Arcade::IGameModule *entryPointGame()
{
    return new Arcade::CentipedeGame();
}

extern "C" int arcade_game_api_version()
{
    return 2;
}