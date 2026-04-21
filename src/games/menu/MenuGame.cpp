#include "IGameModule.hpp"

#include <algorithm>

namespace Arcade {

class MenuGame final : public IGameModule {
public:
    MenuGame() : _name("Menu") {}

    void init() override
    {
        _request = {};
        _selectedGame = 0;
        if (_playerName.empty())
            _playerName = "";
        rebuildRenderData();
    }

    void reset() override
    {
        _request = {};
        _selectedGame = 0;
        rebuildRenderData();
    }

    void close() override {}

    const std::string &getName() const override { return _name; }

    void setContext(const GameContext &ctx) override
    {
        _ctx = ctx;
        if (!_ctx.playerName.empty())
            _playerName = _ctx.playerName;
    }

    const std::string &getPlayerName() const override { return _playerName; }

    void handleInput(Key key) override
    {
        switch (key) {
            case Key::UP:
                if (_selectedGame > 0)
                    _selectedGame--;
                break;
            case Key::DOWN:
                if (!_ctx.gameLibPaths.empty() && _selectedGame + 1 < (int)_ctx.gameLibPaths.size())
                    _selectedGame++;
                break;
            case Key::BACKSPACE:
                if (!_playerName.empty())
                    _playerName.pop_back();
                break;
            case Key::ENTER:
                if (!_ctx.gameLibPaths.empty() && _selectedGame >= 0 && _selectedGame < (int)_ctx.gameLibPaths.size()) {
                    _request.type = GameRequestType::START_GAME;
                    _request.gameLibPath = _ctx.gameLibPaths[(std::size_t)_selectedGame];
                }
                break;
            case Key::QUIT: case Key::ESCAPE:
                _request.type = GameRequestType::QUIT;
                break;
            default:
                if (key >= Key::KEY_A && key <= Key::KEY_Z && _playerName.size() < 12) {
                    char c = 'A' + (static_cast<int>(key) - static_cast<int>(Key::KEY_A));
                    _playerName += c;
                }
                break;
        }
        rebuildRenderData();
    }

    void update() override
    {
        rebuildRenderData();
    }

    GameRequest pullRequest() override
    {
        GameRequest out = _request;
        _request = {};
        return out;
    }

    const RenderData &getRenderData() override
    {
        rebuildRenderData();
        return _renderData;
    }

    int getScore() const override { return 0; }

    bool isGameOver() const override { return false; }

private:
    std::string _name;
    GameContext _ctx;
    GameRequest _request;

    std::string _playerName;
    int _selectedGame = 0;

    RenderData _renderData;

    static bool startsWith(const std::string &s, const char *pfx)
    {
        return s.rfind(pfx, 0) == 0;
    }

    static std::string prettyLabel(const std::string &path)
    {
        std::string s = path;
        if (startsWith(s, "./lib/"))
            s = s.substr(6);
        const std::size_t p = s.find_last_of("/\\");
        if (p != std::string::npos)
            s = s.substr(p + 1);
        if (s.size() > 3 && s.substr(s.size() - 3) == ".so")
            s = s.substr(0, s.size() - 3);
        return s;
    }

    void rebuildRenderData()
    {
        _renderData.gridWidth = 60;
        _renderData.gridHeight = 30;
        _renderData.grid.assign(_renderData.gridHeight,
            std::vector<Cell>(_renderData.gridWidth, {' ', Color::WHITE, Color::BLACK}));

        for (int x = 0; x < _renderData.gridWidth; x++) {
            _renderData.grid[0][x] = {'=', Color::CYAN, Color::BLACK};
            _renderData.grid[_renderData.gridHeight - 1][x] = {'=', Color::CYAN, Color::BLACK};
        }
        for (int y = 0; y < _renderData.gridHeight; y++) {
            _renderData.grid[y][0] = {'|', Color::CYAN, Color::BLACK};
            _renderData.grid[y][_renderData.gridWidth - 1] = {'|', Color::CYAN, Color::BLACK};
        }

        _renderData.texts.clear();
        _renderData.texts.push_back({"=== ARCADE ===", 22, 1, Color::CYAN});
        _renderData.texts.push_back({"Player: " + _playerName + "_", 2, 3, Color::YELLOW});

        _renderData.texts.push_back({"--- Games ---", 2, 5, Color::GREEN});
        for (std::size_t i = 0; i < _ctx.gameLibPaths.size(); i++) {
            const bool selected = ((int)i == _selectedGame);
            std::string prefix = selected ? "> " : "  ";
            Color c = selected ? Color::YELLOW : Color::WHITE;
            _renderData.texts.push_back({prefix + _ctx.gameLibPaths[i], 2, 6 + (int)i, c});
        }

        int gfxY = 6 + (int)_ctx.gameLibPaths.size() + 1;
        _renderData.texts.push_back({"--- Graphics ---", 2, gfxY, Color::GREEN});
        for (std::size_t i = 0; i < _ctx.graphicsLibPaths.size(); i++) {
            std::string prefix = (i == _ctx.activeGraphicsIndex) ? "* " : "  ";
            _renderData.texts.push_back({prefix + _ctx.graphicsLibPaths[i], 2, gfxY + 1 + (int)i, Color::WHITE});
        }

        int scoreY = gfxY + 1 + (int)_ctx.graphicsLibPaths.size() + 1;
        _renderData.texts.push_back({"--- Scores ---", 2, scoreY, Color::GREEN});
        int shown = 0;
        for (const auto &s : _ctx.scores) {
            if (shown >= 5)
                break;
            _renderData.texts.push_back({s.playerName + " | " + s.gameName + " | " + std::to_string(s.score),
                2, scoreY + 1 + shown, Color::WHITE});
            shown++;
        }
        _renderData.texts.push_back({"[ENTER] Play  [F1/F2] Gfx  [F6] Menu  [F7/ESC] Quit",
            2, _renderData.gridHeight - 2, Color::CYAN});
        const int colX = 38;
        const int baseY = 6;
        for (std::size_t i = 0; i < _ctx.gameLibPaths.size() && (int)i < 18; i++) {
            _renderData.texts.push_back({prettyLabel(_ctx.gameLibPaths[i]), colX, baseY + (int)i, Color::WHITE});
        }
    }
};

}

extern "C" Arcade::IGameModule *entryPointGame()
{
    return new Arcade::MenuGame();
}

extern "C" int arcade_game_api_version()
{
    return 2;
}
