#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <cmath>
#include <deque>
#include <string>
#include "../../../include/IDisplayModule.hpp"

static const float CELL_SIZE = 20.f;

class SFMLDisplay : public Arcade::IDisplayModule {
public:
    SFMLDisplay() : _name("SFML") {}

    void init() override
    {
        _window.create(sf::VideoMode(1200, 600), "Arcade - SFML");
        _window.setFramerateLimit(60);
        if (!_font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"))
            _font.loadFromFile("/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf");
    }

    void close() override
    {
        if (_window.isOpen())
            _window.close();
    }

    const std::string &getName() const override { return _name; }

    void clear() override
    {
        _window.clear(sf::Color::Black);
    }

    void render(const Arcade::RenderData &data) override
    {
        _lastData = data;
        _hasData = true;
        sf::RectangleShape rect(sf::Vector2f(CELL_SIZE, CELL_SIZE));
        sf::Text charText;
        charText.setFont(_font);
        charText.setCharacterSize(14);

        for (int y = 0; y < data.gridHeight; y++) {
            for (int x = 0; x < data.gridWidth; x++) {
                const auto &cell = data.grid[y][x];
                rect.setPosition(x * CELL_SIZE, y * CELL_SIZE);
                rect.setFillColor(sfColor(cell.bgColor));
                _window.draw(rect);
                if (cell.character != ' ') {
                    charText.setString(std::string(1, cell.character));
                    charText.setFillColor(sfColor(cell.fgColor));
                    charText.setPosition(x * CELL_SIZE + 4, y * CELL_SIZE);
                    _window.draw(charText);
                }
            }
        }
        for (const auto &text : data.texts) {
            sf::Text t;
            t.setFont(_font);
            t.setCharacterSize(14);
            t.setString(text.content);
            t.setFillColor(sfColor(text.color));
            t.setPosition(text.x * CELL_SIZE, text.y * CELL_SIZE);
            _window.draw(t);
        }
    }

    void display() override
    {
        _window.display();
    }

    Arcade::Key pollEvent() override
    {
        if (!_pendingEvents.empty()) {
            Arcade::Key key = _pendingEvents.front();
            _pendingEvents.pop_front();
            return key;
        }
        sf::Event event;
        if (!_window.pollEvent(event))
            return Arcade::Key::NONE;
        if (event.type == sf::Event::Closed)
            return Arcade::Key::QUIT;
        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left)
                queueMouseClick(event.mouseButton.x, event.mouseButton.y, Arcade::Key::MOUSE_LEFT);
            if (event.mouseButton.button == sf::Mouse::Right)
                queueMouseClick(event.mouseButton.x, event.mouseButton.y, Arcade::Key::MOUSE_RIGHT);
            if (!_pendingEvents.empty()) {
                Arcade::Key key = _pendingEvents.front();
                _pendingEvents.pop_front();
                return key;
            }
            return Arcade::Key::NONE;
        }
        if (event.type != sf::Event::KeyPressed)
            return Arcade::Key::NONE;
        switch (event.key.code) {
            case sf::Keyboard::Up: return Arcade::Key::UP;
            case sf::Keyboard::Down: return Arcade::Key::DOWN;
            case sf::Keyboard::Left: return Arcade::Key::LEFT;
            case sf::Keyboard::Right: return Arcade::Key::RIGHT;
            case sf::Keyboard::Return: return Arcade::Key::ENTER;
            case sf::Keyboard::BackSpace: return Arcade::Key::BACKSPACE;
            case sf::Keyboard::Space: return Arcade::Key::SPACE;
            case sf::Keyboard::Escape: return Arcade::Key::ESCAPE;
            case sf::Keyboard::F1: return Arcade::Key::NEXT_LIB;
            case sf::Keyboard::F2: return Arcade::Key::PREV_LIB;
            case sf::Keyboard::F3: return Arcade::Key::NEXT_GAME;
            case sf::Keyboard::F4: return Arcade::Key::PREV_GAME;
            case sf::Keyboard::F5: return Arcade::Key::RESTART;
            case sf::Keyboard::F6: return Arcade::Key::MENU;
            case sf::Keyboard::F7: return Arcade::Key::QUIT;
            default: break;
        }
        if (event.key.code >= sf::Keyboard::A &&
            event.key.code <= sf::Keyboard::Z)
            return static_cast<Arcade::Key>(
                static_cast<int>(Arcade::Key::KEY_A) +
                (event.key.code - sf::Keyboard::A));
        return Arcade::Key::NONE;
    }

private:
    std::string _name;
    sf::RenderWindow _window;
    sf::Font _font;
    std::deque<Arcade::Key> _pendingEvents;
    Arcade::RenderData _lastData;
    bool _hasData = false;

    sf::Color sfColor(Arcade::Color c) const
    {
        switch (c) {
            case Arcade::Color::BLACK:   return sf::Color::Black;
            case Arcade::Color::RED:     return sf::Color(255, 50, 50);
            case Arcade::Color::GREEN:   return sf::Color(50, 255, 50);
            case Arcade::Color::YELLOW:  return sf::Color(255, 255, 50);
            case Arcade::Color::BLUE:    return sf::Color(50, 50, 255);
            case Arcade::Color::MAGENTA: return sf::Color(255, 50, 255);
            case Arcade::Color::CYAN:    return sf::Color(50, 255, 255);
            case Arcade::Color::WHITE:   return sf::Color::White;
        }
        return sf::Color::White;
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
    return new SFMLDisplay();
}
