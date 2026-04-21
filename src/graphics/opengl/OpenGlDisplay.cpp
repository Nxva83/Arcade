#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "IDisplayModule.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <cstddef>
#include <array>
#include <deque>
#include <algorithm>
#include <cmath>
#include "OpenGlShaders.hpp"
#include "OpenGlFont.hpp"
#include "OpenGlRender.hpp"

static const int WIN_W = 1200;
static const int WIN_H = 600;
static const int CELL_SIZE = 20;
static constexpr float CAMERA_ROT_STEP = 3.1415926535f / 24.f;
static constexpr float CAMERA_TILT_STEP = 0.07f;
static constexpr float CAMERA_TILT_MIN = 0.35f;
static constexpr float CAMERA_TILT_MAX = 1.05f;
static constexpr float CAMERA_TILT_DEFAULT = 0.52f;

struct Vertex {
    float x, y;
    float r, g, b, a;
    float u, v;
    float useTex;
};

static void colorToFloat(Arcade::Color c, float &r, float &g, float &b)
{
    switch (c) {
        case Arcade::Color::BLACK: r=0.f;  g=0.f;  b=0.f;  break;
        case Arcade::Color::RED: r=1.f;  g=0.2f; b=0.2f; break;
        case Arcade::Color::GREEN: r=0.2f; g=1.f;  b=0.2f; break;
        case Arcade::Color::YELLOW: r=1.f;  g=1.f;  b=0.2f; break;
        case Arcade::Color::BLUE: r=0.2f; g=0.2f; b=1.f;  break;
        case Arcade::Color::MAGENTA: r=1.f;  g=0.2f; b=1.f; break;
        case Arcade::Color::CYAN: r=0.2f; g=1.f;  b=1.f; break;
        case Arcade::Color::WHITE: r=1.f;  g=1.f;  b=1.f; break;
        default: r=1.f;  g=1.f;  b=1.f;  break;
    }
}

namespace Arcade {

class OpenGLDisplay : public IDisplayModule {
public:
    OpenGLDisplay()
                : _name("OpenGL"), _window(nullptr), _shader(0), _vao(0), _vbo(0),
            _fontTex(0), _tileTex(0), _eventsPolled(false),
            _cameraOffsetX(0.f), _cameraOffsetY(0.f), _cameraZoom(1.f),
            _cameraYaw(0.f), _cameraTilt(CAMERA_TILT_DEFAULT), _cameraBelow(false),
            _applyCamera(false), _isRightDragging(false), _rightDragMoved(false),
            _lastMouseX(0.0), _lastMouseY(0.0), _currentMouseX(0.0), _currentMouseY(0.0),
            _cursorX(0), _cursorY(0), _hasCursor(false),
            _lastGridW(1), _lastGridH(1), _hasLastGrid(false), _isMinesweeperLayout(false),
            _hasLastSceneType(false), _lastSceneWasMenu(false)
    {}
    ~OpenGLDisplay() override { close(); }

    void init() override
    {
        if (!glfwInit())
            throw std::runtime_error("OpenGL: glfwInit failed");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_SAMPLES, 4);
        _window = glfwCreateWindow(WIN_W, WIN_H, "Arcade - OpenGL", nullptr, nullptr);
        if (!_window) { glfwTerminate(); throw std::runtime_error("OpenGL: glfwCreateWindow failed"); }
        glfwMakeContextCurrent(_window);
        glfwSwapInterval(1);
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) { glfwDestroyWindow(_window); glfwTerminate(); throw std::runtime_error("OpenGL: glewInit failed"); }

        glEnable(GL_MULTISAMPLE);
        glEnable(GL_FRAMEBUFFER_SRGB);

        glfwSetWindowUserPointer(_window, this);
        glfwSetKeyCallback(_window, &OpenGLDisplay::keyCallback);
        glfwSetMouseButtonCallback(_window, &OpenGLDisplay::mouseButtonCallback);
        glfwSetCursorPosCallback(_window, &OpenGLDisplay::cursorPosCallback);
        glfwSetScrollCallback(_window, &OpenGLDisplay::scrollCallback);

        _shader = createShaderProgram(VERT_SRC, FRAG_SRC);
        createBuffers();
        createFontTexture();
        createTilesTexture();

        glUseProgram(_shader);
        GLint loc = glGetUniformLocation(_shader, "uFont");
        if (loc >= 0)
            glUniform1i(loc, 0);

        GLint tilesLoc = glGetUniformLocation(_shader, "uTiles");
        if (tilesLoc >= 0)
            glUniform1i(tilesLoc, 1);

        GLint tloc = glGetUniformLocation(_shader, "uFontTexel");
        if (tloc >= 0)
            glUniform2f(tloc, 1.0f / (float)FONT_TEX_W, 1.0f / (float)FONT_TEX_H);

        GLint rloc = glGetUniformLocation(_shader, "uResolution");
        if (rloc >= 0)
            glUniform2f(rloc, (float)WIN_W, (float)WIN_H);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
        P2 toNdc(float px, float py) const
        {
            if (_applyCamera) {
                const float cx = (float)WIN_W * 0.5f;
                const float cy = (float)WIN_H * 0.5f;
                px = (px - cx) * _cameraZoom + cx + _cameraOffsetX;
                py = (py - cy) * _cameraZoom + cy + _cameraOffsetY;
            }
            return {
                px / (float)WIN_W * 2.f - 1.f,
                -py / (float)WIN_H * 2.f + 1.f
            };
        }
        P2 toScreenPixels(float px, float py) const
        {
            P2 ndc = toNdc(px, py);
            return {
                (ndc.x + 1.f) * (float)WIN_W * 0.5f,
                (1.f - ndc.y) * (float)WIN_H * 0.5f
            };
        }
        static P2 add(P2 a, P2 b) { return {a.x + b.x, a.y + b.y}; }
        static P2 sub(P2 a, P2 b) { return {a.x - b.x, a.y - b.y}; }
        static P2 mul(P2 a, float s) { return {a.x * s, a.y * s}; }
        static float cross2(P2 a, P2 b, P2 c)
        {
            return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        }
        static bool pointInTriangle(P2 p, P2 a, P2 b, P2 c)
        {
            const float c1 = cross2(a, b, p);
            const float c2 = cross2(b, c, p);
            const float c3 = cross2(c, a, p);
            const bool hasNeg = (c1 < 0.f) || (c2 < 0.f) || (c3 < 0.f);
            const bool hasPos = (c1 > 0.f) || (c2 > 0.f) || (c3 > 0.f);
            return !(hasNeg && hasPos);
        }
        static bool pointInQuad(P2 p, P2 a, P2 b, P2 c, P2 d)
        {
            return pointInTriangle(p, a, b, c) || pointInTriangle(p, a, c, d);
        }
        static P2 diamondTop(P2 center, P2 ex, P2 ey)    { return sub(center, mul(add(ex, ey), 0.5f)); }
        static P2 diamondRight(P2 center, P2 ex, P2 ey)  { return add(center, mul(sub(ex, ey), 0.5f)); }
        static P2 diamondBottom(P2 center, P2 ex, P2 ey) { return add(center, mul(add(ex, ey), 0.5f)); }
        static P2 diamondLeft(P2 center, P2 ex, P2 ey)   { return sub(center, mul(sub(ex, ey), 0.5f)); }
        void pushQuadPxUV4(P2 p0, P2 p1, P2 p2, P2 p3,
            float r, float g, float b, float a,
            float u0, float v0,
            float u1, float v1,
            float u2, float v2,
            float u3, float v3,
            float useTex)
        {
            P2 n0 = toNdc(p0.x, p0.y);
            P2 n1 = toNdc(p1.x, p1.y);
            P2 n2 = toNdc(p2.x, p2.y);
            P2 n3 = toNdc(p3.x, p3.y);
            _verts.push_back({n0.x, n0.y, r, g, b, a, u0, v0, useTex});
            _verts.push_back({n1.x, n1.y, r, g, b, a, u1, v1, useTex});
            _verts.push_back({n2.x, n2.y, r, g, b, a, u2, v2, useTex});
            _verts.push_back({n1.x, n1.y, r, g, b, a, u1, v1, useTex});
            _verts.push_back({n3.x, n3.y, r, g, b, a, u3, v3, useTex});
            _verts.push_back({n2.x, n2.y, r, g, b, a, u2, v2, useTex});
        }

        void pushDiamondOriented(P2 center, P2 ex, P2 ey,
                         float r, float g, float b, float a,
                         float useTex, float uvScale)
        {
            P2 top = diamondTop(center, ex, ey);
            P2 right = diamondRight(center, ex, ey);
            P2 bottom = diamondBottom(center, ex, ey);
            P2 left = diamondLeft(center, ex, ey);
            pushQuadPxUV4(
                top, right, left, bottom,
                r, g, b, a,
                0.5f * uvScale, 0.0f * uvScale,
                1.0f * uvScale, 0.5f * uvScale,
                0.0f * uvScale, 0.5f * uvScale,
                0.5f * uvScale, 1.0f * uvScale,
                useTex);
        }
        static P2 isoCenter(float x, float y,
                            float centerX, float centerY,
                            float yawCos, float yawSin,
                            float originX, float originY,
                            float halfW, float halfH)
        {
            const float dx = x - centerX;
            const float dy = y - centerY;
            const float rx = dx * yawCos - dy * yawSin;
            const float ry = dx * yawSin + dy * yawCos;
            return {
                originX + (rx - ry) * halfW,
                originY + (rx + ry) * halfH
            };
        }
        void pushGlyphIso(char ch, P2 center, P2 ex, P2 ey, float scale,
            float r, float g, float b)
        {
            unsigned char c = static_cast<unsigned char>(ch);
            if (c >= 128)
                c = static_cast<unsigned char>('?');
            if (c >= 'a' && c <= 'z')
                c = static_cast<unsigned char>('A' + (c - 'a'));
            const int col = (int)(c % FONT_COLS);
            const int row = (int)(c / FONT_COLS);
            const float u0 = (float)(col * FONT_GLYPH_W) / (float)FONT_TEX_W;
            const float u1 = (float)((col + 1) * FONT_GLYPH_W) / (float)FONT_TEX_W;
            const float v0 = (float)((row + 1) * FONT_GLYPH_H) / (float)FONT_TEX_H;
            const float v1 = (float)(row * FONT_GLYPH_H) / (float)FONT_TEX_H;
            P2 dx = mul(ex, 0.55f * scale);
            P2 dy = mul(ey, 0.55f * scale);
            P2 p0 = sub(sub(center, dx), dy);
            P2 p1 = add(sub(center, dy), dx);
            P2 p2 = sub(add(center, dy), dx);
            P2 p3 = add(add(center, dx), dy);
            pushQuadPxUV4(
                p0, p1, p2, p3,
                r, g, b, 1.f,
                u0, v0,
                u1, v0,
                u0, v1,
                u1, v1,
                1.f);
        }
        void pushGlyphPx(char ch, float px, float py, float scale,
            float r, float g, float b)
        {
            unsigned char c = static_cast<unsigned char>(ch);
            if (c >= 128)
                c = static_cast<unsigned char>('?');
            if (c >= 'a' && c <= 'z')
                c = static_cast<unsigned char>('A' + (c - 'a'));
            const int col = (int)(c % FONT_COLS);
            const int row = (int)(c / FONT_COLS);
            const float u0 = (float)(col * FONT_GLYPH_W) / (float)FONT_TEX_W;
            const float u1 = (float)((col + 1) * FONT_GLYPH_W) / (float)FONT_TEX_W;
            const float v0 = (float)((row + 1) * FONT_GLYPH_H) / (float)FONT_TEX_H;
            const float v1 = (float)(row * FONT_GLYPH_H) / (float)FONT_TEX_H;
            const float w = 11.f * scale;
            const float h = 15.f * scale;
            P2 p0 = {px, py};
            P2 p1 = {px + w, py};
            P2 p2 = {px, py + h};
            P2 p3 = {px + w, py + h};
            pushQuadPxUV4(
                p0, p1, p2, p3,
                r, g, b, 1.f,
                u0, v0,
                u1, v0,
                u0, v1,
                u1, v1,
                1.f);
        }
        void pushTextPx(const std::string &text, float px, float py, float scale,
            float r, float g, float b)
        {
            float cursor = 0.f;
            for (char ch : text) {
                if (ch == ' ') {
                    cursor += 7.f * scale;
                    continue;
                }
                pushGlyphPx(ch, px + cursor, py, scale, r, g, b);
                cursor += 10.f * scale;
            }
        }
    void close() override
    {
        if (!_window) return;
        if (_fontTex) { glDeleteTextures(1, &_fontTex); _fontTex = 0; }
        if (_tileTex) { glDeleteTextures(1, &_tileTex); _tileTex = 0; }
        if (_vao)    { glDeleteVertexArrays(1, &_vao); _vao = 0; }
        if (_vbo)    { glDeleteBuffers(1, &_vbo);      _vbo = 0; }
        if (_shader) { glDeleteProgram(_shader);        _shader = 0; }
        glfwDestroyWindow(_window);
        glfwTerminate();
        _window = nullptr;
    }
    const std::string &getName() const override { return _name; }
    void clear() override
    {
        _verts.clear();
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    void render(const RenderData &data) override
    {
        const int gw = std::max(1, data.gridWidth);
        const int gh = std::max(1, data.gridHeight);
        const bool menuScene = isMenuScene(data, gw, gh);
        if (_hasLastSceneType && menuScene != _lastSceneWasMenu)
            resetCameraView();
        _hasLastSceneType = true;
        _lastSceneWasMenu = menuScene;
        _lastGridW = gw;
        _lastGridH = gh;
        _hasLastGrid = true;
        _isMinesweeperLayout = isMinesweeperScene(data);
        if (menuScene) {
            renderMenuScene(data, gw, gh);
            return;
        }
        renderIsoScene(data, gw, gh);
    }
    void display() override
    {
        pollEventsOnce();
        flushDraw();
        glfwSwapBuffers(_window);
        _eventsPolled = false;
    }

    Key pollEvent() override
    {
        pollEventsOnce();
        if (glfwWindowShouldClose(_window))    return Key::QUIT;
        if (_eventQueue.empty())
            return Key::NONE;
        Key k = _eventQueue.front();
        _eventQueue.pop_front();
        return k;
    }

private:
    static void colorScaled(Arcade::Color color, float mult, float &r, float &g, float &b)
    {
        colorToFloat(color, r, g, b);
        r = std::clamp(r * mult, 0.f, 1.f);
        g = std::clamp(g * mult, 0.f, 1.f);
        b = std::clamp(b * mult, 0.f, 1.f);
    }

    bool isMenuScene(const RenderData &data, int gw, int gh) const
    {
        if (gw != 60 || gh != 30)
            return false;
        for (const auto &text : data.texts) {
            if (text.content.find("ARCADE") != std::string::npos)
                return true;
        }
        return false;
    }

    bool isMinesweeperScene(const RenderData &data) const
    {
        for (const auto &text : data.texts) {
            if (text.content.find("MINESWEEPER") != std::string::npos)
                return true;
        }
        return false;
    }

    void renderMenuScene(const RenderData &data, int gw, int gh)
    {
        for (int y = 0; y < gh; y++) {
            for (int x = 0; x < gw; x++) {
                const Cell &cell = data.grid[y][x];
                if (cell.character != ' ')
                    pushGlyph(cell.character, (float)x, (float)y, cell.fgColor);
            }
        }
        for (const auto &text : data.texts)
            pushText(text.content, (float)text.x, (float)text.y, text.color);
    }

    IsoLayout buildIsoLayout(int gw, int gh) const
    {
        const float margin = 26.f;
        const float panelW = 320.f;
        const float availW = std::max(200.f, (float)WIN_W - panelW - margin * 2.f);
        const float denom = (float)(gw + gh - 2);
        float halfW = (denom > 0.f) ? (availW / denom) : 16.f;
        halfW = std::clamp(halfW, 6.f, 22.f);
        const float pitchScale = std::clamp(_cameraTilt, CAMERA_TILT_MIN, CAMERA_TILT_MAX);
        const bool cameraBelow = _cameraBelow;
        const float viewSign = cameraBelow ? -1.f : 1.f;
        const float halfH = halfW * pitchScale;
        const float yawCos = std::cos(_cameraYaw);
        const float yawSin = std::sin(_cameraYaw);
        const float centerX = (float)(gw - 1) * 0.5f;
        const float centerY = (float)(gh - 1) * 0.5f;

        auto projectCorner = [&](float gx, float gy) {
            const float dx = gx - centerX;
            const float dy = gy - centerY;
            const float rx = dx * yawCos - dy * yawSin;
            const float ry = dx * yawSin + dy * yawCos;
            return P2{(rx - ry) * halfW, (rx + ry) * halfH * viewSign};
        };

        const P2 c00 = projectCorner(0.f, 0.f);
        const P2 c10 = projectCorner((float)(gw - 1), 0.f);
        const P2 c01 = projectCorner(0.f, (float)(gh - 1));
        const P2 c11 = projectCorner((float)(gw - 1), (float)(gh - 1));

        float minX = std::min(std::min(c00.x, c10.x), std::min(c01.x, c11.x));
        float maxX = std::max(std::max(c00.x, c10.x), std::max(c01.x, c11.x));
        float minY = std::min(std::min(c00.y, c10.y), std::min(c01.y, c11.y));
        float maxY = std::max(std::max(c00.y, c10.y), std::max(c01.y, c11.y));

        const float extrudeDir = cameraBelow ? 1.f : -1.f;
        const float approxHeight = halfW * 1.45f;
        minY += extrudeDir * approxHeight;
        maxY += extrudeDir * approxHeight;

        const float playMinX = margin;
        const float playMaxX = (float)WIN_W - panelW - margin;
        const float playMinY = margin;
        const float playMaxY = (float)WIN_H - margin;
        const float targetCenterX = (playMinX + playMaxX) * 0.5f;
        const float targetCenterY = (playMinY + playMaxY) * 0.5f;

        const float originX = targetCenterX - (minX + maxX) * 0.5f;
        const float originY = targetCenterY - (minY + maxY) * 0.5f;

        return {
            panelW,
            halfW,
            halfH,
            pitchScale,
            cameraBelow,
            originX,
            originY,
            centerX,
            centerY,
            yawCos,
            yawSin,
            {(yawCos - yawSin) * halfW, (yawCos + yawSin) * halfH * viewSign},
            {(-yawSin - yawCos) * halfW, (-yawSin + yawCos) * halfH * viewSign}
        };
    }

    std::vector<RenderCell> buildRenderOrder(int gw, int gh, const IsoLayout &layout) const
    {
        std::vector<RenderCell> orderedCells;
        orderedCells.reserve((std::size_t)gw * (std::size_t)gh);
        for (int y = 0; y < gh; y++) {
            for (int x = 0; x < gw; x++) {
                const float dx = (float)x - layout.centerX;
                const float dy = (float)y - layout.centerY;
                const float rx = dx * layout.yawCos - dy * layout.yawSin;
                const float ry = dx * layout.yawSin + dy * layout.yawCos;
                orderedCells.push_back({x, y, rx + ry, rx - ry});
            }
        }
        if (!layout.cameraBelow) {
            std::sort(orderedCells.begin(), orderedCells.end(),
                [](const RenderCell &lhs, const RenderCell &rhs) {
                    if (lhs.depth < rhs.depth) return true;
                    if (lhs.depth > rhs.depth) return false;
                    return lhs.tie < rhs.tie;
                });
        } else {
            std::sort(orderedCells.begin(), orderedCells.end(),
                [](const RenderCell &lhs, const RenderCell &rhs) {
                    if (lhs.depth > rhs.depth) return true;
                    if (lhs.depth < rhs.depth) return false;
                    return lhs.tie > rhs.tie;
                });
        }
        return orderedCells;
    }

    void renderIsoScene(const RenderData &data, int gw, int gh)
    {
        _pickCells.clear();
        _hasCursor = false;
        const IsoLayout layout = buildIsoLayout(gw, gh);
        const std::vector<RenderCell> orderedCells = buildRenderOrder(gw, gh, layout);

        _applyCamera = true;
        for (const auto &cellInfo : orderedCells) {
            const Cell &cell = data.grid[cellInfo.y][cellInfo.x];
            if (_isMinesweeperLayout &&
                cell.character == ' ' &&
                cell.bgColor == Arcade::Color::BLACK)
                continue;
            if (cell.bgColor == Arcade::Color::BLUE && !_hasCursor) {
                _cursorX = cellInfo.x;
                _cursorY = cellInfo.y;
                _hasCursor = true;
            }
            const bool isCursorCell = (cell.bgColor == Arcade::Color::BLUE);
            const bool hasGlyph = (cell.character != ' ');
            const P2 blockEx = mul(layout.ex, 0.78f);
            const P2 blockEy = mul(layout.ey, 0.78f);
            P2 c0 = isoCenter((float)cellInfo.x, (float)cellInfo.y,
                layout.centerX, layout.centerY,
                layout.yawCos, layout.yawSin,
                layout.originX, layout.originY,
                layout.halfW, layout.halfH);
            const float h = layout.halfW * (hasGlyph ? 1.65f : 1.25f);
            const float extrudeDir = layout.cameraBelow ? 1.f : -1.f;
            P2 cTop = {c0.x, c0.y + extrudeDir * h};
            P2 cursorCenter = cTop;
            P2 bR = diamondRight(c0, blockEx, blockEy);
            P2 bB = diamondBottom(c0, blockEx, blockEy);
            P2 bT = diamondTop(c0, blockEx, blockEy);
            P2 bL = diamondLeft(c0, blockEx, blockEy);
            P2 tR = diamondRight(cTop, blockEx, blockEy);
            P2 tB = diamondBottom(cTop, blockEx, blockEy);
            P2 tT = diamondTop(cTop, blockEx, blockEy);
            P2 tL = diamondLeft(cTop, blockEx, blockEy);
            _pickCells.push_back({
                cellInfo.x,
                cellInfo.y,
                toScreenPixels(cTop.x, cTop.y),
                toScreenPixels(tT.x, tT.y),
                toScreenPixels(tR.x, tR.y),
                toScreenPixels(tB.x, tB.y),
                toScreenPixels(tL.x, tL.y)
            });
            float tr, tg, tb;
            colorScaled(cell.bgColor, 1.05f, tr, tg, tb);
            float sr, sg, sb;
            colorScaled(cell.bgColor, 0.58f, sr, sg, sb);
            float lr, lg, lb;
            colorScaled(cell.bgColor, 0.76f, lr, lg, lb);
            if (layout.cameraBelow) {
                pushQuadPxUV4(tR, tT, bT, bR,
                    lr, lg, lb, 1.f,
                    0.f, 0.f,
                    1.f, 0.f,
                    0.f, 1.f,
                    1.f, 1.f,
                    2.f);
                pushQuadPxUV4(tL, tT, bT, bL,
                    sr, sg, sb, 1.f,
                    0.f, 0.f,
                    1.f, 0.f,
                    0.f, 1.f,
                    1.f, 1.f,
                    2.f);
            } else {
                pushQuadPxUV4(tR, tB, bB, bR,
                    lr, lg, lb, 1.f,
                    0.f, 0.f,
                    1.f, 0.f,
                    0.f, 1.f,
                    1.f, 1.f,
                    2.f);
                pushQuadPxUV4(tL, tB, bB, bL,
                    sr, sg, sb, 1.f,
                    0.f, 0.f,
                    1.f, 0.f,
                    0.f, 1.f,
                    1.f, 1.f,
                    2.f);
            }
            pushDiamondOriented(cTop, blockEx, blockEy, tr, tg, tb, 1.f, 2.f, 2.0f);
            if (hasGlyph) {
                float gr, gg, gb;
                colorScaled(cell.fgColor, 1.0f, gr, gg, gb);
                pushGlyphIso(cell.character, cTop, blockEx, blockEy, 0.65f, gr, gg, gb);
            }

            if (isCursorCell) {
                pushDiamondOriented(cursorCenter,
                    mul(blockEx, 0.42f),
                    mul(blockEy, 0.42f),
                    0.25f, 0.95f, 1.0f,
                    0.95f,
                    0.f,
                    1.f);
            }
        }
        _applyCamera = false;

        const float panelW = 285.f;
        const float panelH = 138.f;
        const float panelX = (float)WIN_W - panelW - 12.f;
        const float panelY = 10.f;
        pushQuadPxUV4(
            {panelX, panelY},
            {panelX + panelW, panelY},
            {panelX, panelY + panelH},
            {panelX + panelW, panelY + panelH},
            0.08f, 0.10f, 0.14f, 0.92f,
            0.f, 0.f,
            1.f, 0.f,
            0.f, 1.f,
            1.f, 1.f,
            0.f);

        std::string scoreLine = "Score: -";
        for (const auto &text : data.texts) {
            if (text.content.find("Score") != std::string::npos) {
                scoreLine = text.content;
                break;
            }
        }

        const float textStartX = panelX + 10.f;
        const float textStartY = panelY + 10.f;
        pushTextPx(scoreLine, textStartX, textStartY, 0.95f, 0.25f, 0.95f, 1.0f);
        pushTextPx("MOVE: ARROWS", textStartX, textStartY + 19.f, 0.72f, 1.f, 1.f, 1.f);
        pushTextPx("ACTION: SPACE/ENTER/F", textStartX, textStartY + 35.f, 0.72f, 1.f, 1.f, 1.f);
        pushTextPx("CAM: A/E W/S", textStartX, textStartY + 51.f, 0.72f, 1.f, 1.f, 1.f);
        pushTextPx("CAM: RMB DRAG + WHEEL", textStartX, textStartY + 67.f, 0.72f, 1.f, 1.f, 1.f);
        pushTextPx("F1/F2 GFX  F3/F4 GAME", textStartX, textStartY + 83.f, 0.72f, 1.f, 1.f, 1.f);
        pushTextPx("F6 MENU  F7/ESC QUIT", textStartX, textStartY + 99.f, 0.72f, 1.f, 1.f, 1.f);
    }

    std::string _name;
    GLFWwindow *_window;
    GLuint _shader;
    GLuint _vao, _vbo;
    GLuint _fontTex;
    GLuint _tileTex;
    std::vector<Vertex> _verts;
    bool _eventsPolled;
    std::deque<Key> _eventQueue;
    float _cameraOffsetX;
    float _cameraOffsetY;
    float _cameraZoom;
    float _cameraYaw;
    float _cameraTilt;
    bool _cameraBelow;
    bool _applyCamera;
    bool _isRightDragging;
    bool _rightDragMoved;
    double _lastMouseX;
    double _lastMouseY;
    double _currentMouseX;
    double _currentMouseY;
    
    struct PickCell {
        int gx, gy;
        P2 center;
        P2 top;
        P2 right;
        P2 bottom;
        P2 left;
    };
    std::vector<PickCell> _pickCells;
    int _cursorX;
    int _cursorY;
    bool _hasCursor;
    int _lastGridW;
    int _lastGridH;
    bool _hasLastGrid;
    bool _isMinesweeperLayout;
    bool _hasLastSceneType;
    bool _lastSceneWasMenu;

    void resetCameraView()
    {
        _cameraOffsetX = 0.f;
        _cameraOffsetY = 0.f;
        _cameraZoom = 1.f;
        _cameraYaw = 0.f;
        _cameraTilt = CAMERA_TILT_DEFAULT;
        _cameraBelow = false;
        _isRightDragging = false;
        _rightDragMoved = false;
    }

    void pollEventsOnce()
    {
        if (_eventsPolled)
            return;
        glfwPollEvents();
        _eventsPolled = true;
    }
    static void keyCallback(GLFWwindow *window, int key, int, int action, int)
    {
        if (action != GLFW_PRESS && action != GLFW_REPEAT)
            return;
        auto *self = static_cast<OpenGLDisplay *>(glfwGetWindowUserPointer(window));
        if (!self)
            return;
        self->pushKeyEvent(key);
    }
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int)
    {
        auto *self = static_cast<OpenGLDisplay *>(glfwGetWindowUserPointer(window));
        if (!self)
            return;
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            glfwGetCursorPos(window, &self->_currentMouseX, &self->_currentMouseY);
            self->handleMouseClick(Key::MOUSE_LEFT);
            return;
        }
        if (button != GLFW_MOUSE_BUTTON_RIGHT)
            return;
        if (action == GLFW_PRESS) {
            self->_isRightDragging = true;
            self->_rightDragMoved = false;
            glfwGetCursorPos(window, &self->_lastMouseX, &self->_lastMouseY);
        } else if (action == GLFW_RELEASE) {
            self->_isRightDragging = false;
            if (!self->_rightDragMoved) {
                glfwGetCursorPos(window, &self->_currentMouseX, &self->_currentMouseY);
                self->handleMouseClick(Key::MOUSE_RIGHT);
            }
        }
    }

    static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos)
    {
        auto *self = static_cast<OpenGLDisplay *>(glfwGetWindowUserPointer(window));
        if (!self)
            return;
        self->_currentMouseX = xpos;
        self->_currentMouseY = ypos;
        if (!self->_isRightDragging)
            return;
        const double dx = xpos - self->_lastMouseX;
        const double dy = ypos - self->_lastMouseY;
        self->_lastMouseX = xpos;
        self->_lastMouseY = ypos;
        if (std::fabs(dx) > 3.0 || std::fabs(dy) > 3.0)
            self->_rightDragMoved = true;
        self->_cameraOffsetX += static_cast<float>(dx);
        self->_cameraOffsetY += static_cast<float>(dy);
    }

    static void scrollCallback(GLFWwindow *window, double, double yoffset)
    {
        auto *self = static_cast<OpenGLDisplay *>(glfwGetWindowUserPointer(window));
        if (!self || yoffset == 0.0)
            return;
        const float factor = (yoffset > 0.0) ? 1.1f : 0.9f;
        self->_cameraZoom *= factor;
        self->_cameraZoom = std::clamp(self->_cameraZoom, 0.45f, 2.8f);
    }

    void handleMouseClick(Key mouseAction)
    {
        if (!_hasCursor || !_hasLastGrid || _pickCells.empty())
            return;

        const float mx = (float)_currentMouseX;
        const float my = (float)_currentMouseY;
        const P2 mouse = {mx, my};

        int minX = 0;
        int minY = 0;
        int maxX = _lastGridW - 1;
        int maxY = _lastGridH - 1;
        if (_isMinesweeperLayout) {
            minX = 1;
            minY = 1;
            maxX = std::max(minX, _lastGridW - 20);
            maxY = std::max(minY, _lastGridH - 2);
        }

        int targetX = -1;
        int targetY = -1;
        float bestDist = std::numeric_limits<float>::max();
        for (const auto &pick : _pickCells) {
            if (pick.gx < minX || pick.gx > maxX || pick.gy < minY || pick.gy > maxY)
                continue;
            if (!pointInQuad(mouse, pick.top, pick.right, pick.bottom, pick.left))
                continue;
            const float dx = mx - pick.center.x;
            const float dy = my - pick.center.y;
            const float dist = dx * dx + dy * dy;
            if (dist <= bestDist) {
                bestDist = dist;
                targetX = pick.gx;
                targetY = pick.gy;
            }
        }

        if (targetX < 0 || targetY < 0) {
            for (const auto &pick : _pickCells) {
                if (pick.gx < minX || pick.gx > maxX || pick.gy < minY || pick.gy > maxY)
                    continue;
                const float dx = mx - pick.center.x;
                const float dy = my - pick.center.y;
                const float dist = dx * dx + dy * dy;
                if (dist < bestDist) {
                    bestDist = dist;
                    targetX = pick.gx;
                    targetY = pick.gy;
                }
            }
        }

        if (targetX < 0 || targetY < 0)
            return;
        int dx = targetX - _cursorX;
        int dy = targetY - _cursorY;
        while (dx > 0) { _eventQueue.push_back(Key::RIGHT); dx--; }
        while (dx < 0) { _eventQueue.push_back(Key::LEFT); dx++; }
        while (dy > 0) { _eventQueue.push_back(Key::DOWN); dy--; }
        while (dy < 0) { _eventQueue.push_back(Key::UP); dy++; }
        _eventQueue.push_back(mouseAction);
    }

    void pushKeyEvent(int glfwKey)
    {
        switch (glfwKey) {
            case GLFW_KEY_A:
                _cameraYaw -= CAMERA_ROT_STEP;
                return;
            case GLFW_KEY_E:
                _cameraYaw += CAMERA_ROT_STEP;
                return;
            case GLFW_KEY_W:
                if (_cameraBelow) {
                    _cameraTilt -= CAMERA_TILT_STEP;
                    if (_cameraTilt <= CAMERA_TILT_MIN) {
                        _cameraTilt = CAMERA_TILT_MIN;
                        _cameraBelow = false;
                    }
                } else {
                    _cameraTilt = std::min(CAMERA_TILT_MAX, _cameraTilt + CAMERA_TILT_STEP);
                }
                return;
            case GLFW_KEY_S:
                if (!_cameraBelow) {
                    _cameraTilt -= CAMERA_TILT_STEP;
                    if (_cameraTilt <= CAMERA_TILT_MIN) {
                        _cameraTilt = CAMERA_TILT_MIN;
                        _cameraBelow = true;
                    }
                } else {
                    _cameraTilt = std::min(CAMERA_TILT_MAX, _cameraTilt + CAMERA_TILT_STEP);
                }
                return;
        }
        switch (glfwKey) {
            case GLFW_KEY_F3:
            case GLFW_KEY_F4:
            case GLFW_KEY_F6:
                resetCameraView();
                break;
            default:
                break;
        }
        switch (glfwKey) {
            case GLFW_KEY_LEFT: _eventQueue.push_back(Key::LEFT); break;
            case GLFW_KEY_RIGHT: _eventQueue.push_back(Key::RIGHT); break;
            case GLFW_KEY_UP: _eventQueue.push_back(Key::UP); break;
            case GLFW_KEY_DOWN: _eventQueue.push_back(Key::DOWN); break;
            case GLFW_KEY_ENTER: _eventQueue.push_back(Key::ENTER); break;
            case GLFW_KEY_BACKSPACE: _eventQueue.push_back(Key::BACKSPACE); break;
            case GLFW_KEY_SPACE: _eventQueue.push_back(Key::SPACE); break;
            case GLFW_KEY_ESCAPE: _eventQueue.push_back(Key::ESCAPE); break;
            case GLFW_KEY_F1: _eventQueue.push_back(Key::NEXT_LIB); break;
            case GLFW_KEY_F2: _eventQueue.push_back(Key::PREV_LIB); break;
            case GLFW_KEY_F3: _eventQueue.push_back(Key::NEXT_GAME); break;
            case GLFW_KEY_F4: _eventQueue.push_back(Key::PREV_GAME); break;
            case GLFW_KEY_F5: _eventQueue.push_back(Key::RESTART); break;
            case GLFW_KEY_F6: _eventQueue.push_back(Key::MENU); break;
            case GLFW_KEY_F7: _eventQueue.push_back(Key::QUIT); break;
            default:
                if (glfwKey >= GLFW_KEY_A && glfwKey <= GLFW_KEY_Z)
                    _eventQueue.push_back(static_cast<Key>(static_cast<int>(Key::KEY_A) + (glfwKey - GLFW_KEY_A)));
                break;
        }
    }
    void pushQuadUV(float cx, float cy, float cw, float ch,
        Arcade::Color color, float alpha,
        float u0, float v0, float u1, float v1,
        float useTex)
    {
        float r, g, b;
        colorToFloat(color, r, g, b);
        float px = cx * CELL_SIZE,  py = cy * CELL_SIZE;
        float pw = cw * CELL_SIZE,  ph = ch * CELL_SIZE;
        float x0 =  px        / WIN_W * 2.f - 1.f;
        float y0 = -py        / WIN_H * 2.f + 1.f;
        float x1 = (px + pw)  / WIN_W * 2.f - 1.f;
        float y1 = -(py + ph) / WIN_H * 2.f + 1.f;
        _verts.push_back({x0, y0, r, g, b, alpha, u0, v0, useTex});
        _verts.push_back({x1, y0, r, g, b, alpha, u1, v0, useTex});
        _verts.push_back({x0, y1, r, g, b, alpha, u0, v1, useTex});
        _verts.push_back({x1, y0, r, g, b, alpha, u1, v0, useTex});
        _verts.push_back({x1, y1, r, g, b, alpha, u1, v1, useTex});
        _verts.push_back({x0, y1, r, g, b, alpha, u0, v1, useTex});
    }

    void pushGlyph(char ch, float gx, float gy, Arcade::Color color)
    {
        unsigned char c = static_cast<unsigned char>(ch);
        if (c >= 128)
            c = static_cast<unsigned char>('?');
        if (c >= 'a' && c <= 'z')
            c = static_cast<unsigned char>('A' + (c - 'a'));

        const int col = (int)(c % FONT_COLS);
        const int row = (int)(c / FONT_COLS);
        const float u0 = (float)(col * FONT_GLYPH_W) / (float)FONT_TEX_W;
        const float u1 = (float)((col + 1) * FONT_GLYPH_W) / (float)FONT_TEX_W;
        const float v0 = (float)((row + 1) * FONT_GLYPH_H) / (float)FONT_TEX_H;
        const float v1 = (float)(row * FONT_GLYPH_H) / (float)FONT_TEX_H;
        pushQuadUV(gx + 0.05f, gy + 0.05f, 0.9f, 0.9f, color, 1.f, u0, v0, u1, v1, 1.f);
    }
    void pushText(const std::string &s, float x, float y, Arcade::Color color)
    {
        float cursor = x;
        for (char ch : s) {
            if (ch == ' ') {
                cursor += 1.f;
                continue;
            }
            pushGlyph(ch, cursor, y, color);
            cursor += 1.f;
        }
    }
    void flushDraw()
    {
        if (_verts.empty()) return;
        glUseProgram(_shader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _fontTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _tileTex);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(_vao);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        GLsizeiptr sz = (GLsizeiptr)(_verts.size() * sizeof(Vertex));
        glBufferData(GL_ARRAY_BUFFER, sz, nullptr, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sz, _verts.data());
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_verts.size());
        glBindVertexArray(0);
        _verts.clear();
    }
    GLuint compileShader(GLenum type, const char *src)
    {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512] = {};
            glGetShaderInfoLog(s, sizeof(log), nullptr, log);
            glDeleteShader(s);
            throw std::runtime_error(std::string("Shader error: ") + log);
        }
        return s;
    }
    GLuint createShaderProgram(const char *vert, const char *frag)
    {
        GLuint v = compileShader(GL_VERTEX_SHADER,   vert);
        GLuint f = compileShader(GL_FRAGMENT_SHADER, frag);
        GLuint p = glCreateProgram();
        glAttachShader(p, v); glAttachShader(p, f);
        glLinkProgram(p);
        GLint ok = 0;
        glGetProgramiv(p, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[512] = {};
            glGetProgramInfoLog(p, sizeof(log), nullptr, log);
            glDeleteProgram(p);
            throw std::runtime_error(std::string("Link error: ") + log);
        }
        glDeleteShader(v); glDeleteShader(f);
        return p;
    }
    void createBuffers()
    {
        glGenVertexArrays(1, &_vao);
        glGenBuffers(1, &_vbo);
        glBindVertexArray(_vao);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBufferData(GL_ARRAY_BUFFER, 8192 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, x)));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, r)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, u)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, useTex)));
        glEnableVertexAttribArray(3);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void createFontTexture()
    {
        std::vector<unsigned char> tex(FONT_TEX_W * FONT_TEX_H, 0);
        for (int c = 0; c < 128; c++) {
            const int col = c % FONT_COLS;
            const int row = c / FONT_COLS;
            const unsigned char *g = glyph8x8((unsigned char)c);
            for (int gy = 0; gy < FONT_GLYPH_H; gy++) {
                const unsigned char bits = g[gy];
                const int dstY = row * FONT_GLYPH_H + (FONT_GLYPH_H - 1 - gy);
                for (int gx = 0; gx < FONT_GLYPH_W; gx++) {
                    const bool on = (bits & (1u << (7 - gx))) != 0;
                    const int dstX = col * FONT_GLYPH_W + gx;
                    tex[dstY * FONT_TEX_W + dstX] = on ? 255 : 0;
                }
            }
        }
        glGenTextures(1, &_fontTex);
        glBindTexture(GL_TEXTURE_2D, _fontTex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, FONT_TEX_W, FONT_TEX_H, 0, GL_RED, GL_UNSIGNED_BYTE, tex.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    void createTilesTexture()
    {
        const int W = 128;
        const int H = 128;
        std::vector<unsigned char> tex((std::size_t)W * (std::size_t)H * 3, 0);
        auto put = [&](int x, int y, unsigned char r, unsigned char g, unsigned char b) {
            std::size_t i = ((std::size_t)y * (std::size_t)W + (std::size_t)x) * 3;
            tex[i + 0] = r;
            tex[i + 1] = g;
            tex[i + 2] = b;
        };
        for (int y = 0; y < H; y++) {
            for (int x = 0; x < W; x++) {
                int cx = x / 16;
                int cy = y / 16;
                bool checker = ((cx + cy) & 1) != 0;
                unsigned char base = checker ? 180 : 140;
                unsigned char v = (unsigned char)(base + ((x * 13 + y * 7) % 21) - 10);
                if (x % 16 == 0 || y % 16 == 0)
                    v = (unsigned char)std::min(255, (int)v + 35);
                put(x, y, v, v, v);
            }
        }
        glGenTextures(1, &_tileTex);
        glBindTexture(GL_TEXTURE_2D, _tileTex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, W, H, 0, GL_RGB, GL_UNSIGNED_BYTE, tex.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
};
}

extern "C" Arcade::IDisplayModule *entryPointDisplay()
{
    return new Arcade::OpenGLDisplay();
}
