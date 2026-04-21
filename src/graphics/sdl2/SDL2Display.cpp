#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cmath>
#include <deque>
#include <string>
#include <unordered_map>
#include "../../../include/IDisplayModule.hpp"

static const int CELL_SIZE = 20;

class SDL2Display : public Arcade::IDisplayModule {
public:
    SDL2Display() : _name("SDL2"), _window(nullptr),
                    _renderer(nullptr), _font(nullptr) {}

    void init() override
    {
        SDL_Init(SDL_INIT_VIDEO);
        TTF_Init();
        _window = SDL_CreateWindow("Arcade - SDL2",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            1200, 600, SDL_WINDOW_SHOWN);
        _renderer = SDL_CreateRenderer(_window, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        _font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 14);
        if (!_font)
            _font = TTF_OpenFont("/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf", 14);
    }

    void close() override
    {
        for (auto &p : _textCache) SDL_DestroyTexture(p.second);
        _textCache.clear();
        if (_font) TTF_CloseFont(_font);
        if (_renderer) SDL_DestroyRenderer(_renderer);
        if (_window) SDL_DestroyWindow(_window);
        TTF_Quit();
        SDL_Quit();
        _font = nullptr;
        _renderer = nullptr;
        _window = nullptr;
    }

    const std::string &getName() const override { return _name; }

    void clear() override
    {
        SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
        SDL_RenderClear(_renderer);
    }

    void render(const Arcade::RenderData &data) override
    {
        _lastData = data;
        _hasData = true;
        for (int y = 0; y < data.gridHeight; y++) {
            for (int x = 0; x < data.gridWidth; x++) {
                const auto &cell = data.grid[y][x];
                SDL_Rect rect = {x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
                auto [br, bg, bb] = sdlColor(cell.bgColor);
                SDL_SetRenderDrawColor(_renderer, br, bg, bb, 255);
                SDL_RenderFillRect(_renderer, &rect);
                if (cell.character != ' ' && _font) {
                    auto [fr, fg, fb] = sdlColor(cell.fgColor);
                    SDL_Color color = {fr, fg, fb, 255};
                    char str[2] = {cell.character, '\0'};
                    renderText(str, rect.x + 4, rect.y + 2, color);
                }
            }
        }
        for (const auto &text : data.texts) {
            if (_font) {
                auto [r, g, b] = sdlColor(text.color);
                SDL_Color color = {r, g, b, 255};
                renderText(text.content.c_str(),
                    text.x * CELL_SIZE, text.y * CELL_SIZE, color);
            }
        }
    }

    void display() override
    {
        SDL_RenderPresent(_renderer);
        for (auto &p : _textCache) SDL_DestroyTexture(p.second);
        _textCache.clear();
    }

    Arcade::Key pollEvent() override
    {
        if (!_pendingEvents.empty()) {
            Arcade::Key key = _pendingEvents.front();
            _pendingEvents.pop_front();
            return key;
        }
        SDL_Event e;
        if (!SDL_PollEvent(&e))
            return Arcade::Key::NONE;
        if (e.type == SDL_QUIT)
            return Arcade::Key::QUIT;
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (e.button.button == SDL_BUTTON_LEFT)
                queueMouseClick(e.button.x, e.button.y, Arcade::Key::MOUSE_LEFT);
            if (e.button.button == SDL_BUTTON_RIGHT)
                queueMouseClick(e.button.x, e.button.y, Arcade::Key::MOUSE_RIGHT);
            if (!_pendingEvents.empty()) {
                Arcade::Key key = _pendingEvents.front();
                _pendingEvents.pop_front();
                return key;
            }
            return Arcade::Key::NONE;
        }
        if (e.type != SDL_KEYDOWN)
            return Arcade::Key::NONE;
        switch (e.key.keysym.sym) {
            case SDLK_UP: return Arcade::Key::UP;
            case SDLK_DOWN: return Arcade::Key::DOWN;
            case SDLK_LEFT: return Arcade::Key::LEFT;
            case SDLK_RIGHT: return Arcade::Key::RIGHT;
            case SDLK_RETURN: return Arcade::Key::ENTER;
            case SDLK_BACKSPACE: return Arcade::Key::BACKSPACE;
            case SDLK_SPACE: return Arcade::Key::SPACE;
            case SDLK_ESCAPE: return Arcade::Key::ESCAPE;
            case SDLK_F1: return Arcade::Key::NEXT_LIB;
            case SDLK_F2: return Arcade::Key::PREV_LIB;
            case SDLK_F3: return Arcade::Key::NEXT_GAME;
            case SDLK_F4: return Arcade::Key::PREV_GAME;
            case SDLK_F5: return Arcade::Key::RESTART;
            case SDLK_F6: return Arcade::Key::MENU;
            case SDLK_F7: return Arcade::Key::QUIT;
            default: break;
        }
        if (e.key.keysym.sym >= SDLK_a && e.key.keysym.sym <= SDLK_z)
            return static_cast<Arcade::Key>(
                static_cast<int>(Arcade::Key::KEY_A) +
                (e.key.keysym.sym - SDLK_a));
        return Arcade::Key::NONE;
    }

private:
    std::string _name;
    SDL_Window *_window;
    SDL_Renderer *_renderer;
    TTF_Font *_font;
    std::deque<Arcade::Key> _pendingEvents;
    Arcade::RenderData _lastData;
    bool _hasData = false;
    std::unordered_map<std::string, SDL_Texture *> _textCache;

    struct RGB { Uint8 r, g, b; };

    RGB sdlColor(Arcade::Color c) const
    {
        switch (c) {
            case Arcade::Color::BLACK:   return {0, 0, 0};
            case Arcade::Color::RED:     return {255, 50, 50};
            case Arcade::Color::GREEN:   return {50, 255, 50};
            case Arcade::Color::YELLOW:  return {255, 255, 50};
            case Arcade::Color::BLUE:    return {50, 50, 255};
            case Arcade::Color::MAGENTA: return {255, 50, 255};
            case Arcade::Color::CYAN:    return {50, 255, 255};
            case Arcade::Color::WHITE:   return {255, 255, 255};
        }
        return {255, 255, 255};
    }

    void renderText(const char *text, int x, int y, SDL_Color color)
    {
        SDL_Surface *surface = TTF_RenderText_Solid(_font, text, color);
        if (!surface) return;
        SDL_Texture *texture = SDL_CreateTextureFromSurface(_renderer, surface);
        SDL_Rect dst = {x, y, surface->w, surface->h};
        SDL_FreeSurface(surface);
        SDL_RenderCopy(_renderer, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
    }

    void queueMouseClick(int mouseX, int mouseY, Arcade::Key action)
    {
        if (!_hasData) {
            _pendingEvents.push_back(action);
            return;
        }

        const int gx = mouseX / CELL_SIZE;
        const int gy = mouseY / CELL_SIZE;
        if (gy < 0 || gy >= _lastData.gridHeight || gx < 0 || gx >= _lastData.gridWidth) {
            _pendingEvents.push_back(action);
            return;
        }

        const auto &cell = _lastData.grid[gy][gx];
        if (cell.character == ' ') {
            _pendingEvents.push_back(action);
            return;
        }

        int cursorX = -1;
        int cursorY = -1;
        for (int y = 0; y < _lastData.gridHeight; y++) {
            for (int x = 0; x < _lastData.gridWidth; x++) {
                if (_lastData.grid[y][x].bgColor == Arcade::Color::BLUE) {
                    cursorX = x;
                    cursorY = y;
                    break;
                }
            }
            if (cursorX >= 0)
                break;
        }

        if (cursorX < 0 || cursorY < 0) {
            _pendingEvents.push_back(action);
            return;
        }

        int dx = gx - cursorX;
        int dy = gy - cursorY;
        while (dx > 0) { _pendingEvents.push_back(Arcade::Key::RIGHT); dx--; }
        while (dx < 0) { _pendingEvents.push_back(Arcade::Key::LEFT); dx++; }
        while (dy > 0) { _pendingEvents.push_back(Arcade::Key::DOWN); dy--; }
        while (dy < 0) { _pendingEvents.push_back(Arcade::Key::UP); dy++; }
        _pendingEvents.push_back(action);
    }
};

extern "C" Arcade::IDisplayModule *entryPointDisplay()
{
    return new SDL2Display();
}
