#include <ncurses.h>
#include <deque>
#include <string>
#include "../../../include/IDisplayModule.hpp"

class NcursesDisplay : public Arcade::IDisplayModule {
public:
    NcursesDisplay() : _name("nCurses") {}

    void init() override
    {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        nodelay(stdscr, TRUE);
        curs_set(0);
        start_color();
        for (int fg = 0; fg < 8; fg++)
            for (int bg = 0; bg < 8; bg++)
                init_pair(fg * 8 + bg + 1, fg, bg);
        mousemask(BUTTON1_CLICKED | BUTTON3_CLICKED, nullptr);
    }

    void close() override
    {
        endwin();
    }

    const std::string &getName() const override { return _name; }

    void clear() override
    {
        ::clear();
    }

    void render(const Arcade::RenderData &data) override
    {
        _lastData = data;
        _hasData = true;
        for (int y = 0; y < data.gridHeight && y < LINES; y++) {
            for (int x = 0; x < data.gridWidth && x < COLS; x++) {
                const auto &cell = data.grid[y][x];
                int pair = colorPair(cell.fgColor, cell.bgColor);
                attron(COLOR_PAIR(pair));
                mvaddch(y, x, cell.character);
                attroff(COLOR_PAIR(pair));
            }
        }
        for (const auto &text : data.texts) {
            int pair = colorPair(text.color, Arcade::Color::BLACK);
            attron(COLOR_PAIR(pair));
            mvprintw(text.y, text.x, "%s", text.content.c_str());
            attroff(COLOR_PAIR(pair));
        }
    }

    void display() override
    {
        refresh();
    }

    Arcade::Key pollEvent() override
    {
        if (!_pendingEvents.empty()) {
            Arcade::Key key = _pendingEvents.front();
            _pendingEvents.pop_front();
            return key;
        }
        int ch = getch();
        if (ch == ERR)
            return Arcade::Key::NONE;
        if (ch == KEY_MOUSE) {
            MEVENT event;
            if (getmouse(&event) == OK) {
                if (event.bstate & BUTTON1_CLICKED)
                    queueMouseClick(event.x, event.y, Arcade::Key::MOUSE_LEFT);
                if (event.bstate & BUTTON3_CLICKED)
                    queueMouseClick(event.x, event.y, Arcade::Key::MOUSE_RIGHT);
                if (!_pendingEvents.empty()) {
                    Arcade::Key key = _pendingEvents.front();
                    _pendingEvents.pop_front();
                    return key;
                }
            }
            return Arcade::Key::NONE;
        }
        switch (ch) {
            case KEY_UP: return Arcade::Key::UP;
            case KEY_DOWN: return Arcade::Key::DOWN;
            case KEY_LEFT: return Arcade::Key::LEFT;
            case KEY_RIGHT: return Arcade::Key::RIGHT;
            case '\n': case KEY_ENTER: return Arcade::Key::ENTER;
            case KEY_BACKSPACE: case 127: return Arcade::Key::BACKSPACE;
            case ' ': return Arcade::Key::SPACE;
            case 27: return Arcade::Key::ESCAPE;
            case KEY_F(1): return Arcade::Key::NEXT_LIB;
            case KEY_F(2): return Arcade::Key::PREV_LIB;
            case KEY_F(3): return Arcade::Key::NEXT_GAME;
            case KEY_F(4): return Arcade::Key::PREV_GAME;
            case KEY_F(5): return Arcade::Key::RESTART;
            case KEY_F(6): return Arcade::Key::MENU;
            case KEY_F(7): return Arcade::Key::QUIT;
            default: break;
        }
        if (ch >= 'a' && ch <= 'z')
            return static_cast<Arcade::Key>(
                static_cast<int>(Arcade::Key::KEY_A) + (ch - 'a'));
        if (ch >= 'A' && ch <= 'Z')
            return static_cast<Arcade::Key>(
                static_cast<int>(Arcade::Key::KEY_A) + (ch - 'A'));
        return Arcade::Key::NONE;
    }

private:
    std::string _name;
    std::deque<Arcade::Key> _pendingEvents;
    Arcade::RenderData _lastData;
    bool _hasData = false;

    int ncursesColor(Arcade::Color c) const
    {
        switch (c) {
            case Arcade::Color::BLACK: return COLOR_BLACK;
            case Arcade::Color::RED: return COLOR_RED;
            case Arcade::Color::GREEN: return COLOR_GREEN;
            case Arcade::Color::YELLOW: return COLOR_YELLOW;
            case Arcade::Color::BLUE: return COLOR_BLUE;
            case Arcade::Color::MAGENTA: return COLOR_MAGENTA;
            case Arcade::Color::CYAN: return COLOR_CYAN;
            case Arcade::Color::WHITE: return COLOR_WHITE;
        }
        return COLOR_WHITE;
    }

    int colorPair(Arcade::Color fg, Arcade::Color bg) const
    {
        return ncursesColor(fg) * 8 + ncursesColor(bg) + 1;
    }

    void queueMouseClick(int mouseX, int mouseY, Arcade::Key action)
    {
        if (!_hasData) {
            _pendingEvents.push_back(action);
            return;
        }

        const int gx = mouseX;
        const int gy = mouseY;
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
    return new NcursesDisplay();
}
