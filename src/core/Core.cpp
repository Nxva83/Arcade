#include "Core.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <chrono>
#include <thread>
#include <algorithm>
#include <filesystem>
#include <vector>

namespace Arcade {

namespace {

const std::vector<std::string> &getGameEntryPointNames()
{
    static const std::vector<std::string> names = {
        "createGameModule",
        "entryPointGame",
        "createGame",
        "entryPoint",
    };
    return names;
}

const std::vector<std::string> &getDisplayEntryPointNames()
{
    static const std::vector<std::string> names = {
        "createDisplayModule",
        "entryPointDisplay",
        "createDisplay",
        "entryPoint",
    };
    return names;
}

template <typename T>
std::string resolveEntryPointOrThrow(DLLoader<T> &loader,
    const std::vector<std::string> &names,
    const std::string &libPath,
    const std::string &kind)
{
    for (const auto &name : names) {
        if (loader.hasSymbol(name))
            return name;
    }

    std::string msg = "No " + kind + " entry point found in '" + libPath + "' (tried: ";
    for (std::size_t i = 0; i < names.size(); i++) {
        msg += names[i];
        if (i + 1 < names.size())
            msg += ", ";
    }
    msg += ")";
    throw std::runtime_error(msg);
}

}

Core::Core(const std::string &initialGraphicsLib)
        : _state(State::GAME),
            _currentGraphicsIdx(0), _currentGameIdx(0),
      _initialGraphicsLib(initialGraphicsLib),
    _currentGameSupportsExtendedApi(false),
      _displayLoader(nullptr), _gameLoader(nullptr),
      _display(nullptr), _game(nullptr)
{
}

Core::~Core()
{
    unloadGame();
    unloadGraphics();
}

void Core::scanLibraries()
{
    DIR *dir = opendir("./lib/");
    if (!dir)
        throw std::runtime_error("Cannot open ./lib/ directory");
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name.size() < 4 || name.substr(name.size() - 3) != ".so")
            continue;
        if (name.rfind("arcade_", 0) != 0)
            continue;
        std::string path = "./lib/" + name;
        try {
            DLLoader<IDisplayModule> test(path);
            (void)resolveEntryPointOrThrow(test, getDisplayEntryPointNames(), path, "display");
            _graphicsLibPaths.push_back(path);
            continue;
        } catch (...) {
        }
        try {
            DLLoader<IGameModule> test(path);
            (void)resolveEntryPointOrThrow(test, getGameEntryPointNames(), path, "game");
            _gameLibPaths.push_back(path);
        } catch (...) {
        }
    }
    closedir(dir);
    std::sort(_graphicsLibPaths.begin(), _graphicsLibPaths.end());
    std::sort(_gameLibPaths.begin(), _gameLibPaths.end());

    _menuGamePath.clear();
    for (const auto &p : _gameLibPaths) {
        if (p.find("arcade_menu.so") != std::string::npos) {
            _menuGamePath = p;
            break;
        }
    }

    _playableGameLibPaths.clear();
    for (const auto &p : _gameLibPaths) {
        if (!_menuGamePath.empty() && p == _menuGamePath)
            continue;
        _playableGameLibPaths.push_back(p);
    }
}

void Core::loadGraphics(std::size_t index)
{
    unloadGraphics();
    _currentGraphicsIdx = index;
    _displayLoader = new DLLoader<IDisplayModule>(_graphicsLibPaths[index]);
    const std::string entry = resolveEntryPointOrThrow(*_displayLoader,
        getDisplayEntryPointNames(), _graphicsLibPaths[index], "display");
    _display = _displayLoader->getInstance(entry);
    _display->init();
}

void Core::loadGame(std::size_t index)
{
    unloadGame();
    _currentGameIdx = index;
    _currentGamePath = _gameLibPaths[index];
    _gameLoader = new DLLoader<IGameModule>(_gameLibPaths[index]);
    _currentGameSupportsExtendedApi = false;
    if (_gameLoader->hasSymbol("arcade_game_api_version")) {
        using version_fn_t = int (*)();
        version_fn_t versionFn = _gameLoader->getSymbol<version_fn_t>("arcade_game_api_version");
        if (versionFn && versionFn() >= 2)
            _currentGameSupportsExtendedApi = true;
    }
    const std::string entry = resolveEntryPointOrThrow(*_gameLoader,
        getGameEntryPointNames(), _gameLibPaths[index], "game");
    _game = _gameLoader->getInstance(entry);
    _game->init();
}

void Core::loadGameByPath(const std::string &path)
{
    for (std::size_t i = 0; i < _gameLibPaths.size(); i++) {
        if (_gameLibPaths[i] == path) {
            loadGame(i);
            return;
        }
    }
    throw std::runtime_error("Cannot load game: '" + path + "'");
}

void Core::loadMenuGame()
{
    if (_menuGamePath.empty())
        throw std::runtime_error("Menu game not found (expected ./lib/arcade_menu.so)");
    if (_game && !_currentGamePath.empty() && _currentGamePath == _menuGamePath)
        return;
    loadGameByPath(_menuGamePath);
}

void Core::unloadGraphics()
{
    if (_display) { _display->close(); delete _display; _display = nullptr; }
    if (_displayLoader) { delete _displayLoader; _displayLoader = nullptr; }
}

void Core::unloadGame()
{
    if (_game) { _game->close(); delete _game; _game = nullptr; }
    if (_gameLoader) { delete _gameLoader; _gameLoader = nullptr; }
    _currentGamePath.clear();
    _currentGameSupportsExtendedApi = false;
}

void Core::loadScores()
{
    _scores.clear();
    std::ifstream file("assets/scores.txt");
    if (!file.is_open()) return;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string name, game, scoreStr;
        if (std::getline(iss, name, ':') &&
            std::getline(iss, game, ':') &&
            std::getline(iss, scoreStr)) {
            try {
                _scores.push_back({name, game, std::stoi(scoreStr)});
            } catch (...) {}
        }
    }
}

void Core::saveScore(const std::string &playerName,
                     const std::string &gameName, int score)
{
    if (playerName.empty()) return;
    _scores.push_back({playerName, gameName, score});
    std::ofstream file("assets/scores.txt", std::ios::app);
    if (file.is_open())
        file << playerName << ":" << gameName << ":" << score << "\n";
}

void Core::updateGameContext()
{
    _gameContext.gameLibPaths = _playableGameLibPaths;
    _gameContext.graphicsLibPaths = _graphicsLibPaths;
    _gameContext.activeGraphicsIndex = _currentGraphicsIdx;
    _gameContext.scores = _scores;
    _gameContext.playerName = _playerName;
}

void Core::handleSystemKeys(Key key)
{
    switch (key) {
        case Key::NEXT_LIB:
            if (!_graphicsLibPaths.empty())
                loadGraphics((_currentGraphicsIdx + 1) % _graphicsLibPaths.size());
            break;
        case Key::PREV_LIB:
            if (!_graphicsLibPaths.empty())
                loadGraphics((_currentGraphicsIdx + _graphicsLibPaths.size() - 1)
                    % _graphicsLibPaths.size());
            break;
        case Key::NEXT_GAME:
            if (!_playableGameLibPaths.empty()) {
                std::size_t next = 0;
                auto it = std::find(_playableGameLibPaths.begin(), _playableGameLibPaths.end(), _currentGamePath);
                if (it != _playableGameLibPaths.end())
                    next = ((std::size_t)std::distance(_playableGameLibPaths.begin(), it) + 1) % _playableGameLibPaths.size();
                loadGameByPath(_playableGameLibPaths[next]);
            }
            break;
        case Key::PREV_GAME:
            if (!_playableGameLibPaths.empty()) {
                std::size_t prev = 0;
                auto it = std::find(_playableGameLibPaths.begin(), _playableGameLibPaths.end(), _currentGamePath);
                if (it != _playableGameLibPaths.end()) {
                    std::size_t cur = (std::size_t)std::distance(_playableGameLibPaths.begin(), it);
                    prev = (cur + _playableGameLibPaths.size() - 1) % _playableGameLibPaths.size();
                }
                loadGameByPath(_playableGameLibPaths[prev]);
            }
            break;
        case Key::RESTART:
            if (_game) _game->reset();
            break;
        case Key::MENU:
            loadMenuGame();
            break;
        case Key::QUIT: case Key::ESCAPE:
            _state = State::QUIT;
            break;
        default: break;
    }
}

void Core::gameLoop()
{
    using clock = std::chrono::steady_clock;

    auto getTickDuration = [&]() {
        if (_currentGamePath.find("arcade_minesweeper.so") != std::string::npos)
            return std::chrono::milliseconds(120);
        if (_currentGamePath.find("arcade_snake.so") != std::string::npos)
            return std::chrono::milliseconds(115);
        if (_currentGamePath.find("arcade_centipede.so") != std::string::npos)
            return std::chrono::milliseconds(35);
        if (_currentGamePath.find("arcade_menu.so") != std::string::npos)
            return std::chrono::milliseconds(120);
        return std::chrono::milliseconds(70);
    };

    auto lastTick = clock::now();

    while (_state == State::GAME) {
        Key key;
        while ((key = _display->pollEvent()) != Key::NONE) {
            if (key == Key::NEXT_LIB || key == Key::PREV_LIB ||
                key == Key::NEXT_GAME || key == Key::PREV_GAME ||
                key == Key::RESTART || key == Key::MENU ||
                key == Key::QUIT || key == Key::ESCAPE) {
                handleSystemKeys(key);
                if (_state != State::GAME) return;
                continue;
            }
            if (_game) _game->handleInput(key);
        }

        updateGameContext();
        if (_game && _currentGameSupportsExtendedApi)
            _game->setContext(_gameContext);

        auto now = clock::now();
        const auto tickDuration = getTickDuration();
        while (_game && now - lastTick >= tickDuration) {
            _game->update();
            lastTick += tickDuration;
        }

        if (_game && _currentGameSupportsExtendedApi) {
            const std::string &maybeName = _game->getPlayerName();
            if (!maybeName.empty())
                _playerName = maybeName;
            GameRequest req = _game->pullRequest();
            if (req.type == GameRequestType::QUIT) {
                _state = State::QUIT;
                return;
            }
            if (req.type == GameRequestType::MENU) {
                loadMenuGame();
            }
            if (req.type == GameRequestType::START_GAME && !req.gameLibPath.empty()) {
                loadGameByPath(req.gameLibPath);
            }
        }

        if (_game) {
            _display->clear();
            _display->render(_game->getRenderData());
            _display->display();
            if (_game->isGameOver()) {
                saveScore(_playerName, _game->getName(), _game->getScore());
                std::this_thread::sleep_for(std::chrono::seconds(2));
                loadScores();
                loadMenuGame();
                return;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void Core::run()
{
    scanLibraries();

    auto normalizePath = [](const std::string &p) -> std::string {
        namespace fs = std::filesystem;
        try {
            fs::path path(p);
            if (fs::exists(path))
                return fs::weakly_canonical(path).string();
        } catch (...) {
        }
        return p;
    };

    const std::string requested = normalizePath(_initialGraphicsLib);
    bool found = false;
    for (std::size_t i = 0; i < _graphicsLibPaths.size(); i++) {
        if (normalizePath(_graphicsLibPaths[i]) == requested) {
            _currentGraphicsIdx = i;
            found = true;
            break;
        }
    }
    if (!found)
        throw std::runtime_error("'" + _initialGraphicsLib +
            "' not a graphical library");
    loadGraphics(_currentGraphicsIdx);
    loadScores();
    _state = State::GAME;
    loadMenuGame();
    while (_state != State::QUIT)
        gameLoop();
}

}
