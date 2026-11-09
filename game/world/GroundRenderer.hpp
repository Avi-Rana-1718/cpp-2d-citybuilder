#pragma once
#include <algorithm>
#include <SFML/Graphics.hpp>
#include "./WorldData.hpp"
#include "../simulation/GameClock.hpp"

// -------------------------------------------------------------
// Seasonal look
// -------------------------------------------------------------
// Colour multipliers for natural ground, plus a translucent snow layer.
// Multiplying can only darken, so winter whitening comes from the snow
// overlay drawn on top of land tiles.

struct SeasonLook {
    sf::Color grass = sf::Color::White;  // grass, sand, paths, forest floor, rock ground
    sf::Color water = sf::Color::White;
    sf::Color trees = sf::Color::White;  // tint on the forest canopy sprite
    int treeVariant = 0;                 // 0 green, 1 autumn colours, 2 snowy
    sf::Uint8 snow = 0;                  // snow overlay alpha on land

    bool operator==(const SeasonLook& o) const {
        return grass == o.grass && water == o.water && trees == o.trees &&
               treeVariant == o.treeVariant && snow == o.snow;
    }
    bool operator!=(const SeasonLook& o) const { return !(*this == o); }
};

inline SeasonLook baseSeasonLook(Season s) {
    SeasonLook l;
    switch (s) {
        case Season::Spring:
            l.grass = sf::Color(225, 255, 215); l.trees = sf::Color(215, 255, 205);
            break;
        case Season::Summer:
            break; // textures as drawn
        case Season::Autumn:
            l.grass = sf::Color(255, 222, 160); l.water = sf::Color(228, 234, 245);
            l.treeVariant = 1;
            break;
        case Season::Winter:
            l.grass = sf::Color(215, 225, 240); l.water = sf::Color(170, 192, 225);
            l.treeVariant = 2; l.snow = 120;
            break;
    }
    return l;
}

inline sf::Uint8 lerpByte(sf::Uint8 a, sf::Uint8 b, float t) {
    return static_cast<sf::Uint8>(static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t + 0.5f);
}
inline sf::Color lerpColor(const sf::Color& a, const sf::Color& b, float t) {
    return sf::Color(lerpByte(a.r, b.r, t), lerpByte(a.g, b.g, t), lerpByte(a.b, b.b, t), lerpByte(a.a, b.a, t));
}

// Look for a point in the year: holds the season's look, then blends into
// the next season over the last 20% of it (about 18 game days).
inline SeasonLook seasonLookAt(Season s, float seasonProgress) {
    SeasonLook cur = baseSeasonLook(s);
    const float blendStart = 0.8f;
    if (seasonProgress <= blendStart) return cur;
    Season next = static_cast<Season>((static_cast<int>(s) + 1) % 4);
    SeasonLook nxt = baseSeasonLook(next);
    float t = std::min(1.f, (seasonProgress - blendStart) / (1.f - blendStart));
    SeasonLook out;
    out.grass = lerpColor(cur.grass, nxt.grass, t);
    out.water = lerpColor(cur.water, nxt.water, t);
    out.trees = lerpColor(cur.trees, nxt.trees, t);
    out.snow = lerpByte(cur.snow, nxt.snow, t);
    out.treeVariant = (t < 0.5f) ? cur.treeVariant : nxt.treeVariant; // trees swap mid-blend
    return out;
}

// -------------------------------------------------------------
// Ground renderer
// -------------------------------------------------------------
// Natural ground is not stored as entities. It is drawn straight from
// world[][].terrain as one vertex array over a small texture atlas, so
// all 4,096 ground tiles cost a single draw call. The vertex array is
// rebuilt only when terrain changes (markDirty()).
//
// Tall terrain (forest trees, rock boulders) still needs depth sorting
// with buildings, so drawTall() is called from the depth-ordered pass.

class GroundRenderer {
    // Atlas slots, each one 32x16 ground texture wide
    enum Slot { SLOT_GRASS = 0, SLOT_SAND, SLOT_WATER, SLOT_PATH, SLOT_COUNT };

    sf::Texture atlas;
    sf::VertexArray vertices{sf::Triangles};
    sf::VertexArray snowVertices{sf::Triangles}; // untextured white diamonds over land
    SeasonLook look;
    sf::Sprite forestSprites[3]; // green, autumn, snowy
    sf::Sprite rockSprite;
    bool atlasReady = false;
    bool dirty = true;

    static constexpr unsigned SLOT_W = 32;
    static constexpr unsigned SLOT_H = 16;

    void buildAtlas() {
        const char* keys[SLOT_COUNT] = {"grass", "sand", "water", "forest_path"};
        sf::Image img;
        img.create(SLOT_W * SLOT_COUNT, SLOT_H, sf::Color::Transparent);
        for (int s = 0; s < SLOT_COUNT; s++) {
            if (!m_textures.count(keys[s])) continue;
            const sf::Texture& tex = m_textures[keys[s]];
            if (tex.getSize().x == 0 || tex.getSize().y == 0) continue;
            sf::Image src = tex.copyToImage();
            img.copy(src, SLOT_W * s, 0, sf::IntRect(0, 0, SLOT_W, SLOT_H), true);
        }
        atlas.loadFromImage(img);
        atlasReady = true;

        auto setupTall = [](sf::Sprite& sprite, const char* key) {
            if (!m_textures.count(key)) return;
            const sf::Texture& tex = m_textures[key];
            sprite.setTexture(tex, true);
            float h = static_cast<float>(tex.getSize().y);
            sprite.setOrigin(0.f, h > 16.f ? h - 16.f : 0.f);
            sprite.setScale(2.f, 2.f);
        };
        setupTall(forestSprites[0], "forest");
        setupTall(forestSprites[1], m_textures.count("forest_autumn") ? "forest_autumn" : "forest");
        setupTall(forestSprites[2], m_textures.count("forest_winter") ? "forest_winter" : "forest");
        setupTall(rockSprite, "rock");
    }

    void rebuild() {
        vertices.clear();
        snowVertices.clear();
        vertices.resize(static_cast<size_t>(tileMapW * tileMapH * 6));
        size_t v = 0;
        const float w = static_cast<float>(tileSizeX);
        const float h = static_cast<float>(tileSizeY);

        for (int i = 0; i < tileMapW; i++) {
            for (int j = 0; j < tileMapH; j++) {
                Slot slot = SLOT_GRASS;
                sf::Color tint = sf::Color::White;
                bool isWater = (world[i][j].terrain == Terrain::Water);
                switch (world[i][j].terrain) {
                    case Terrain::Sand:       slot = SLOT_SAND; break;
                    case Terrain::Water:      slot = SLOT_WATER; break;
                    case Terrain::ForestPath: slot = SLOT_PATH; break;
                    case Terrain::Forest:     tint = sf::Color(150, 185, 150); break; // shaded forest floor
                    case Terrain::Rock:       tint = sf::Color(165, 160, 140); break; // stony ground
                    case Terrain::Grass:
                    default: break;
                }

                tint = tint * (isWater ? look.water : look.grass);

                sf::Vector2f p = tileScreenPos(i, j);
                float u0 = static_cast<float>(slot * SLOT_W);
                float u1 = u0 + SLOT_W;

                sf::Vector2f pos[4] = {{p.x, p.y}, {p.x + w, p.y}, {p.x + w, p.y + h}, {p.x, p.y + h}};
                sf::Vector2f uv[4] = {{u0, 0.f}, {u1, 0.f}, {u1, static_cast<float>(SLOT_H)}, {u0, static_cast<float>(SLOT_H)}};
                const int order[6] = {0, 1, 2, 0, 2, 3};
                for (int k = 0; k < 6; k++) {
                    vertices[v].position = pos[order[k]];
                    vertices[v].texCoords = uv[order[k]];
                    vertices[v].color = tint;
                    v++;
                }

                // Snow sits on land only, as a diamond (the texture corners are transparent)
                if (look.snow > 0 && !isWater) {
                    sf::Color snowCol(255, 255, 255, look.snow);
                    sf::Vector2f d[4] = {{p.x + w / 2.f, p.y}, {p.x + w, p.y + h / 2.f},
                                         {p.x + w / 2.f, p.y + h}, {p.x, p.y + h / 2.f}};
                    const int dOrder[6] = {0, 1, 2, 0, 2, 3};
                    for (int k = 0; k < 6; k++) snowVertices.append(sf::Vertex(d[dOrder[k]], snowCol));
                }
            }
        }
        dirty = false;
    }

public:
    void markDirty() { dirty = true; }

    // Applies a seasonal look; rebuilds the ground only when it actually changed
    void setSeasonLook(const SeasonLook& l) {
        if (l != look) {
            look = l;
            dirty = true;
        }
    }
    const SeasonLook& getSeasonLook() const { return look; }
    size_t snowVertexCount() const { return snowVertices.getVertexCount(); }
    bool isDirty() const { return dirty; }
    size_t vertexCount() const { return vertices.getVertexCount(); }

    // Builds the vertex array if terrain changed. Safe without a window (used by tests).
    void prepare() {
        if (!atlasReady) buildAtlas();
        if (dirty) rebuild();
    }

    void draw(sf::RenderTarget& target) {
        prepare();
        sf::RenderStates states;
        states.texture = &atlas;
        target.draw(vertices, states);
    }

    // Snow layer over all land, drawn after structure ground (lots, roads)
    // so it settles on them too. No-op outside winter.
    void drawSnow(sf::RenderTarget& target) {
        prepare();
        if (snowVertices.getVertexCount() > 0) target.draw(snowVertices);
    }

    // Trees / boulders for natural tiles with no structure on them
    void drawTall(sf::RenderTarget& target, int i, int j) {
        Terrain t = world[i][j].terrain;
        int variant = std::max(0, std::min(2, look.treeVariant));
        sf::Sprite* sprite = (t == Terrain::Forest) ? &forestSprites[variant] : (t == Terrain::Rock ? &rockSprite : nullptr);
        if (sprite == nullptr || sprite->getTexture() == nullptr) return;
        sprite->setColor(t == Terrain::Forest ? look.trees : sf::Color::White);
        sprite->setPosition(tileScreenPos(i, j));
        target.draw(*sprite);
    }
};
