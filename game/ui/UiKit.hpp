#pragma once
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>
#include <unordered_map>
#include "../../src/_global.hpp"

// -------------------------------------------------------------
// UI kit: palette and drawing helpers shared by every HUD element
// -------------------------------------------------------------
// Everything is drawn in screen space (default view). Icons are the
// 16x16 pixel-art PNGs in assets/sprites/ui, registered as "ui_<name>".

namespace uikit {

// ---- palette ----
const sf::Color PANEL(17, 22, 31, 236);
const sf::Color PANEL_RAISED(27, 34, 46, 245);
const sf::Color PANEL_HOVER(38, 48, 64, 250);
const sf::Color BORDER(52, 64, 84, 230);
const sf::Color ACCENT(64, 160, 230);
const sf::Color TEXT(226, 232, 240);
const sf::Color TEXT_DIM(138, 152, 170);
const sf::Color GOOD(72, 210, 120);
const sf::Color WARN(245, 196, 40);
const sf::Color BAD(235, 87, 72);
const sf::Color SHADOW(0, 0, 0, 90);

inline const sf::Font& font() { return m_fonts["../assets/Hack.ttf"]; }

// ---- shapes ----
// HUD shapes and labels mostly repeat frame to frame, so their geometry is
// cached and only colours/positions are updated (SFML rebuilds geometry on
// every setString / setPoint otherwise). Caches reset if they grow large.
template <typename T>
inline T& cacheSlot(std::unordered_map<std::string, T>& cache, const std::string& key, bool& fresh) {
    if (cache.size() > 2048) cache.clear();
    auto it = cache.find(key);
    fresh = (it == cache.end());
    return fresh ? cache[key] : it->second;
}

// Rounded rectangle as a convex polygon (6 points per corner)
inline void roundedRect(sf::RenderTarget& t, const sf::FloatRect& r, float radius,
                        const sf::Color& fill, const sf::Color& outline = sf::Color::Transparent,
                        float thickness = 0.f) {
    radius = std::max(0.f, std::min(radius, std::min(r.width, r.height) / 2.f));
    static std::unordered_map<std::string, sf::ConvexShape> shapes;
    char keyBuf[96];
    std::snprintf(keyBuf, sizeof(keyBuf), "%.1f,%.1f,%.1f,%.1f,%.1f,%.1f", r.left, r.top, r.width, r.height, radius, thickness);
    bool fresh = false;
    sf::ConvexShape& cached = cacheSlot(shapes, keyBuf, fresh);
    if (!fresh) {
        cached.setFillColor(fill);
        cached.setOutlineColor(outline);
        t.draw(cached);
        return;
    }
    const int seg = 6;
    sf::ConvexShape& s = cached;
    s.setPointCount(static_cast<size_t>(seg * 4));
    const float cx[4] = {r.left + r.width - radius, r.left + r.width - radius, r.left + radius, r.left + radius};
    const float cy[4] = {r.top + radius, r.top + r.height - radius, r.top + r.height - radius, r.top + radius};
    const float start[4] = {-90.f, 0.f, 90.f, 180.f};
    size_t k = 0;
    for (int c = 0; c < 4; c++) {
        for (int i = 0; i < seg; i++) {
            float a = (start[c] + 90.f * static_cast<float>(i) / static_cast<float>(seg - 1)) * 3.14159265f / 180.f;
            s.setPoint(k++, sf::Vector2f(cx[c] + std::cos(a) * radius, cy[c] + std::sin(a) * radius));
        }
    }
    s.setOutlineThickness(thickness);
    s.setFillColor(fill);
    s.setOutlineColor(outline);
    t.draw(s);
}

// Panel with a soft drop shadow
inline void panel(sf::RenderTarget& t, const sf::FloatRect& r, float radius = 10.f,
                  const sf::Color& fill = PANEL, const sf::Color& outline = BORDER) {
    roundedRect(t, sf::FloatRect(r.left + 2.f, r.top + 3.f, r.width, r.height), radius, SHADOW);
    roundedRect(t, r, radius, fill, outline, 1.f);
}

// Horizontal progress bar, frac in 0..1
inline void bar(sf::RenderTarget& t, const sf::FloatRect& r, float frac, const sf::Color& fill) {
    frac = std::max(0.f, std::min(1.f, frac));
    roundedRect(t, r, r.height / 2.f, sf::Color(10, 13, 19, 220));
    if (frac > 0.f) {
        roundedRect(t, sf::FloatRect(r.left, r.top, std::max(r.height, r.width * frac), r.height), r.height / 2.f, fill);
    }
}

// ---- text ----
// One cached sf::Text per (string, size, style): glyph geometry is built once
inline sf::Text& cachedText(const std::string& str, unsigned size, bool bold) {
    static std::unordered_map<std::string, sf::Text> texts;
    std::string key = std::to_string(size) + (bold ? "b|" : "r|") + str;
    bool fresh = false;
    sf::Text& t = cacheSlot(texts, key, fresh);
    if (fresh) {
        t.setFont(font());
        t.setCharacterSize(size);
        t.setStyle(bold ? sf::Text::Bold : sf::Text::Regular);
        t.setString(str);
    }
    return t;
}

inline float textWidth(const std::string& s, unsigned size, bool bold = false) {
    return cachedText(s, size, bold).getLocalBounds().width;
}

// Draws text with its top-left at (x, y); returns its width
inline float text(sf::RenderTarget& tgt, const std::string& s, float x, float y, unsigned size,
                  const sf::Color& color = TEXT, bool bold = false) {
    sf::Text& t = cachedText(s, size, bold);
    sf::FloatRect b = t.getLocalBounds();
    t.setFillColor(color);
    t.setPosition(std::round(x - b.left), std::round(y - b.top));
    tgt.draw(t);
    return b.width;
}

// Text vertically centred in a row of the given height
inline float textMid(sf::RenderTarget& tgt, const std::string& s, float x, float rowTop, float rowH,
                     unsigned size, const sf::Color& color = TEXT, bool bold = false) {
    sf::Text& t = cachedText(s, size, bold);
    sf::FloatRect b = t.getLocalBounds();
    t.setFillColor(color);
    t.setPosition(std::round(x - b.left), std::round(rowTop + (rowH - b.height) / 2.f - b.top));
    tgt.draw(t);
    return b.width;
}

// Shortens a string with ".." until it fits maxWidth pixels
inline std::string fitText(const std::string& s, unsigned size, float maxWidth, bool bold = false) {
    static std::unordered_map<std::string, std::string> fitted;
    char keyBuf[32];
    std::snprintf(keyBuf, sizeof(keyBuf), "%u%c%.0f|", size, bold ? 'b' : 'r', maxWidth);
    std::string key = keyBuf + s;
    auto it = fitted.find(key);
    if (it != fitted.end()) return it->second;
    if (fitted.size() > 2048) fitted.clear();
    if (textWidth(s, size, bold) <= maxWidth) return fitted[key] = s;
    std::string out = s;
    while (!out.empty() && textWidth(out + "..", size, bold) > maxWidth) out.pop_back();
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return fitted[key] = out + "..";
}

// Right-aligned text ending at x
inline void textRight(sf::RenderTarget& tgt, const std::string& s, float right, float y, unsigned size,
                      const sf::Color& color = TEXT, bool bold = false) {
    text(tgt, s, right - textWidth(s, size, bold), y, size, color, bold);
}

// ---- images ----
inline bool hasTexture(const std::string& key) {
    auto it = m_textures.find(key);
    return it != m_textures.end() && it->second.getSize().x > 0;
}

// Icon "ui_<name>" at (x, y), drawn size x size pixels
inline void icon(sf::RenderTarget& t, const std::string& name, float x, float y, float size = 32.f,
                 const sf::Color& tint = sf::Color::White) {
    const std::string key = "ui_" + name;
    if (!hasTexture(key)) return;
    sf::Sprite s(m_textures[key]);
    float sc = size / static_cast<float>(m_textures[key].getSize().x);
    s.setScale(sc, sc);
    s.setPosition(std::round(x), std::round(y));
    s.setColor(tint);
    t.draw(s);
}

// A texture scaled to fit inside a box (keeps aspect, bottom-centred, whole-pixel scale when possible)
inline void thumbnailTex(sf::RenderTarget& t, const sf::Texture* tex, const sf::FloatRect& box,
                         const sf::Color& tint = sf::Color::White) {
    if (tex == nullptr || tex->getSize().x == 0) return;
    float w = static_cast<float>(tex->getSize().x);
    float h = static_cast<float>(tex->getSize().y);
    float sc = std::min(box.width / w, box.height / h);
    if (sc >= 1.f) sc = std::floor(sc);
    sf::Sprite s(*tex);
    s.setScale(sc, sc);
    s.setPosition(std::round(box.left + (box.width - w * sc) / 2.f), std::round(box.top + box.height - h * sc));
    s.setColor(tint);
    t.draw(s);
}

// Texture by key, scaled to fit inside a box
inline void thumbnail(sf::RenderTarget& t, const std::string& key, const sf::FloatRect& box,
                      const sf::Color& tint = sf::Color::White) {
    if (!hasTexture(key)) return;
    thumbnailTex(t, &m_textures[key], box, tint);
}

// ---- formatting ----
inline std::string withCommas(long long v) {
    bool neg = v < 0;
    std::string digits = std::to_string(neg ? -v : v);
    std::string out;
    int n = 0;
    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; i--) {
        out.insert(out.begin(), digits[static_cast<size_t>(i)]);
        if (++n % 3 == 0 && i > 0) out.insert(out.begin(), ',');
    }
    return (neg ? "-" : "") + out;
}

inline std::string formatMoney(long long v) {
    return (v < 0 ? "-$" : "$") + withCommas(v < 0 ? -v : v);
}

// "SUBURBAN COTTAGES" -> "Suburban Cottages"
inline std::string titleCase(const std::string& s) {
    std::string out = s;
    bool startOfWord = true;
    for (char& c : out) {
        unsigned char u = static_cast<unsigned char>(c);
        if (std::isalpha(u)) {
            c = static_cast<char>(startOfWord ? std::toupper(u) : std::tolower(u));
            startOfWord = false;
        } else {
            startOfWord = (c == ' ' || c == '-');
        }
    }
    return out;
}

inline sf::Color withAlpha(sf::Color c, sf::Uint8 a) {
    c.a = a;
    return c;
}

} // namespace uikit
