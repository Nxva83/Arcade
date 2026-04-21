#include <deque>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>
#include "../../../include/IGameModule.hpp"

enum class Direction {
    NONE,
    UP,
    DOWN,
    LEFT,
    RIGHT
};

class donkey_kongGame : public Arcade::IGameModule {
public:
    donkey_kongGame() : _name("donkey_kong"), _gridW(50), _gridH(25),
                        _score(0), _gameOver(false), vy(0), wantJump(false),
                        jumpImpulse(3), terminalV(3)  {}
  void init() override {
    std::srand(std::time(nullptr));
    reset();
  }

  void reset() override {
        _direction = Direction::NONE;
        _nextDirection = Direction::NONE;
        _score = 0;
        _gameOver = false;
        _won = false;
            vy = 0;
        wantJump = false;
        jumpImpulse = 3;
        terminalV = 3; 
        _barrels.clear();
        _barrelSpawnCooldown = 0;
        _barrelMoveTick = 0;
        _fallGravityTick = 0;

        clearMap();
        addBorders();

        std::vector<int> platY = {4,  10,  16};
        const int lowPlatY = _gridH - 3;
        platY.push_back(lowPlatY);

        addZigzagPlatforms(platY);
        addZigzagLadders(platY);
        _ladderPositions.clear();

        const int playerStartX = 6;
        placeEntities(playerStartX, lowPlatY);
  }

  void close() override {}

  const std::string &getName() const override { return _name; }

    void handleInput(Arcade::Key key) override {
    switch (key) {
        case Arcade::Key::SPACE:
            wantJump = true;
            break;
        case Arcade::Key::LEFT:
            if (_direction != Direction::RIGHT)
                keysPressed.push_back(Direction::LEFT);
            break;
        case Arcade::Key::RIGHT:
            if (_direction != Direction::LEFT)
                keysPressed.push_back(Direction::RIGHT);
            break;
        case Arcade::Key::UP:
            keysPressed.push_back(Direction::UP);
            break;
        case Arcade::Key::DOWN:
            keysPressed.push_back(Direction::DOWN);
            break;
        default: break;
    }
  }

    void update() override 
    {
        if (_gameOver) return;

        while (!keysPressed.empty()) {
            Direction d = keysPressed.front();
            keysPressed.pop_front();

            auto head = _player;
            switch (d) {
                case Direction::NONE: break;
                case Direction::UP:
                    tryClimb(-1);
                    continue;
                case Direction::DOWN:
                    tryClimb(1);
                    continue;
                case Direction::LEFT: head.first--; break;
                case Direction::RIGHT: head.first++; break;
            }

            if (head.first > 0 && head.first < _gridW - 1) {
                char c = _map[head.second][head.first];
                if (c == ' ' || c == 'P' || c == 'H') {
                    movePlayerTo(head.first, _player.second);
                }
            }
        }

        applicateGravity();

        if (isPlayerHitByBarrel()) {
            _gameOver = true;
            _won = false;
            return;
        }

        if (_player == _princess) {
            _gameOver = true;
            _won = true;
            return;
        }

        updateBarrels();

        if (isPlayerHitByBarrel()) {
            _gameOver = true;
            _won = false;
        }
    }
  const Arcade::RenderData &getRenderData() override {
    _renderData.gridWidth = _gridW;
    _renderData.gridHeight = _gridH;
    _renderData.grid.assign(_gridH,
        std::vector<Arcade::Cell>(_gridW,
            {' ', Arcade::Color::WHITE, Arcade::Color::BLACK}));
    colorMap(_renderData, _gridW, _map, _gridH);

    for (const auto &barrel : _barrels) {
        if (!barrel.active)
            continue;
        if (barrel.y > 0 && barrel.y < _gridH - 1 && barrel.x > 0 && barrel.x < _gridW - 1)
            _renderData.grid[barrel.y][barrel.x] = {'O', Arcade::Color::RED, Arcade::Color::BLACK};
    }

    _renderData.texts.clear();
    _renderData.texts.push_back({"Score: " + std::to_string(_score), _gridW + 1, 1, Arcade::Color::WHITE});
    if (_gameOver) {
        if (_won)
            _renderData.texts.push_back({"YOU WIN", _gridW + 1, 3, Arcade::Color::GREEN});
        else
            _renderData.texts.push_back({"GAME OVER", _gridW + 1, 3, Arcade::Color::RED});
    }
    return _renderData;
}
    int getScore() const override { return _score; }
    bool isGameOver() const override { return _gameOver; }
private:
    void clearMap() {
        _map.assign(_gridH, std::string(_gridW, ' '));
    }
    void addBorders() {

        for (int y = 0; y < _gridH; y++) {
            _map[y][0] = '#';
            _map[y][_gridW - 1] = '#';
        }

        for (int y = 1; y < _gridH - 1; y++) {
            _map[y][1] = '#';
            _map[y][_gridW - 2] = '#';
        }
    }
    void addZigzagPlatforms(const std::vector<int> &platY) {
        for (int i = 0; i < (int)platY.size(); i++) {
            int py = platY[i];
            for (int x = 2; x < _gridW - 2; x++) {
                _map[py][x] = '=';
            }
        }
    }
    void addZigzagLadders(const std::vector<int> &platY) {
        for (int i = 0; i < (int)platY.size(); i++) {
            int py = platY[i];
            int lx = (i % 2 == 0) ? 2 : (_gridW - 3);
            _ladderPositions.push_back({lx, py});
            int startY = std::max(1, py - 7);
            for (int y = startY; y <= py; y++) {
                if (y > 0 && y < _gridH - 1 && lx > 0 && lx < _gridW - 1)
                    _map[y][lx] = 'H';
            }
            int platformBelow = py;
            if (platformBelow > 0 && platformBelow < _gridH - 1 && lx > 0 && lx < _gridW - 1)
                _map[platformBelow][lx] = '=';
        }
    }
    void placeEntities(int playerStartX, int lowPlatY) {
        _player = {playerStartX, lowPlatY - 1};
        _princess = {3, 1};
        _underPlayer = _map[_player.second][_player.first];
        _map[_player.second][_player.first] = 'M';
        _map[_princess.second][_princess.first] = 'P';
    }

    void colorMap(Arcade::RenderData &renderData, int gridW, const std::vector<std::string> &map, int gridH)
    {
        if (map.size() == (size_t)gridH) {
            for (int y = 0; y < gridH; y++) {
                for (int x = 0; x < gridW; x++) {
                    char c = map[y][x];
                    switch (c) {
                        case '#':
                            renderData.grid[y][x] = {'#', Arcade::Color::WHITE, Arcade::Color::WHITE};
                            break;
                        case '=':
                            renderData.grid[y][x] = {'=', Arcade::Color::YELLOW, Arcade::Color::BLACK};
                            break;
                        case 'H':
                            renderData.grid[y][x] = {'|', Arcade::Color::MAGENTA, Arcade::Color::BLACK};
                            break;
                        case 'M':
                            renderData.grid[y][x] = {'M', Arcade::Color::GREEN, Arcade::Color::BLACK};
                            break;
                        case 'P':
                            renderData.grid[y][x] = {'P', Arcade::Color::MAGENTA, Arcade::Color::BLACK};
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }

    void pass_ender_ground() {
        
    }

    bool isSolidTile(char tile) const {
        return tile == '=' || tile == '#';
    }

    bool isWalkableTile(char tile) const {
        return tile == ' ' || tile == 'P' || tile == 'H';
    }

    void movePlayerTo(int newX, int newY) {
        _map[_player.second][_player.first] = _underPlayer;
        _underPlayer = _map[newY][newX];
        _player.first = newX;
        _player.second = newY;
        _map[_player.second][_player.first] = 'M';
    }

    bool tryClimb(int dy) {
        int newY = _player.second + dy;
        if (newY <= 0 || newY >= _gridH - 1)
            return false;

        bool onLadder = (_underPlayer == 'H');
        bool ladderAhead = (_map[newY][_player.first] == 'H');
        if (!onLadder && !ladderAhead)
            return false;

        char target = _map[newY][_player.first];
        if (target == '=' || target == '#')
            return false;

        movePlayerTo(_player.first, newY);
        vy = 0;
        return true;
    }

    char getBaseTileAt(int x, int y) const {
        if (x < 0 || x >= _gridW || y < 0 || y >= _gridH)
            return '#';
        if (_player.first == x && _player.second == y)
            return _underPlayer;
        return _map[y][x];
    }

    bool isPlayerHitByBarrel() const {
        if (_underPlayer == 'H')
            return false;
        for (const auto &barrel : _barrels) {
            if (barrel.active && barrel.x == _player.first && barrel.y == _player.second)
                return true;
        }
        return false;
    }

    void spawnBarrel() {
        const int spawnX = 2;
        const int spawnY = 2;
        if (!isWalkableTile(getBaseTileAt(spawnX, spawnY)))
            return;
        int dir = (_ladderPositions.empty() || _ladderPositions[0].first > spawnX) ? 1 : -1;
        _barrels.push_back({spawnX, spawnY, dir, true});
    }

    void updateBarrels() {
        if (_barrelSpawnCooldown <= 0) {
            spawnBarrel();
            _barrelSpawnCooldown = 40;
        } else {
            _barrelSpawnCooldown--;
        }

        _barrelMoveTick++;
        if (_barrelMoveTick < 3)
            return;
        _barrelMoveTick = 0;

        for (auto &barrel : _barrels) {
            if (!barrel.active)
                continue;

            int belowY = barrel.y + 1;
            char belowTile = getBaseTileAt(barrel.x, belowY);

            if (belowY < _gridH - 1 && !isSolidTile(belowTile)) {
                barrel.y++;
            } else {
                int targetX = barrel.x + barrel.dir;
                for (const auto &ladder : _ladderPositions) {
                    if (ladder.second == barrel.y) {
                        targetX = ladder.first;
                        barrel.dir = (targetX > barrel.x) ? 1 : -1;
                        break;
                    }
                }

                int nextX = barrel.x + barrel.dir;
                char nextTile = getBaseTileAt(nextX, barrel.y);

                if (nextX <= 0 || nextX >= _gridW - 1 || isSolidTile(nextTile)) {
                    barrel.dir *= -1;
                    nextX = barrel.x + barrel.dir;
                    nextTile = getBaseTileAt(nextX, barrel.y);
                }

                if (nextX > 0 && nextX < _gridW - 1 && !isSolidTile(nextTile))
                    barrel.x = nextX;
            }

            if (barrel.y >= _gridH - 1)
                barrel.active = false;
        }

        _barrels.erase(
            std::remove_if(_barrels.begin(), _barrels.end(), [](const Barrel &b) { return !b.active; }),
            _barrels.end());
    }

    void applicateGravity(void) {
        int belowY = _player.second + 1;
        bool onGround = false;
        if (belowY >= 0 && belowY < _gridH)
            onGround = isSolidTile(_map[belowY][_player.first]);

        bool onLadder = (_underPlayer == 'H');

        bool jumpStarted = false;
        if (wantJump && onGround && vy == 0) {
            vy = -jumpImpulse;
            jumpStarted = true;
            _fallGravityTick = 0;
        }
        wantJump = false;

        if (onLadder && !jumpStarted) {
            vy = 0;
            _fallGravityTick = 0;
            return;
        }

        if (!jumpStarted) {
            if (!onGround || vy < 0) {
                if (vy < 0) {
                    if (vy < terminalV)
                        vy++;
                } else {
                    _fallGravityTick++;
                    if (_fallGravityTick >= 2) {
                        if (vy < terminalV)
                            vy++;
                        _fallGravityTick = 0;
                    }
                }
            } else if (vy > 0) {
                vy = 0;
            }
        }

        if (onGround)
            _fallGravityTick = 0;

        if (vy == 0)
            return;

        int step = (vy > 0) ? 1 : -1;
        int remaining = std::abs(vy);
        while (remaining-- > 0) {
            int nextY = _player.second + step;
            if (nextY <= 0) {
                nextY = 1;
                vy = 0;
            }
            if (nextY >= _gridH - 1) {
                nextY = _gridH - 2;
                vy = 0;
            }

            if (!isWalkableTile(_map[nextY][_player.first])) {
                vy = 0;
                break;
            }

            movePlayerTo(_player.first, nextY);

            if (step > 0) {
                int underY = _player.second + 1;
                if (underY >= 0 && underY < _gridH && isSolidTile(_map[underY][_player.first])) {
                    vy = 0;
                    break;
                }
            }
        }
    }

    
    std::string _name;
    struct Barrel {
        int x;
        int y;
        int dir;
        bool active;
    };
    Direction _direction;
    Direction _nextDirection;
    int _gridW;
    int _gridH;
    int _score;
    bool _gameOver;
    bool _won = false;
    std::deque<std::pair<int, int>> _snake;
    Arcade::RenderData _renderData;
    std::vector<std::string> _map;
    std::pair<int,int> _player;
    std::pair<int,int> _princess;
    std::deque<Direction> keysPressed;
    char _underPlayer = ' ';
    //jump variables
    int vy;
    bool wantJump;
    int jumpImpulse = 3;
    int terminalV = 3;
    int _fallGravityTick = 0;
    std::vector<Barrel> _barrels;
    int _barrelSpawnCooldown = 0;
    int _barrelMoveTick = 0;
    std::vector<std::pair<int, int>> _ladderPositions; // Ladder x,y positions for barrels to follow
};

extern "C" Arcade::IGameModule *entryPointGame() { return new donkey_kongGame(); }

extern "C" int arcade_game_api_version()
{
    return 2;
}