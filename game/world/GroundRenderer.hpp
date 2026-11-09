#pragma once
#include <SFML/Graphics.hpp>
#include "./WorldData.hpp"

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
    sf::Sprite forestSprite;
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
        setupTall(forestSprite, "forest");
        setupTall(rockSprite, "rock");
    }

    void rebuild() {
        vertices.clear();
        vertices.resize(static_cast<size_t>(tileMapW * tileMapH * 6));
        size_t v = 0;
        const float w = static_cast<float>(tileSizeX);
        const float h = static_cast<float>(tileSizeY);

        for (int i = 0; i < tileMapW; i++) {
            for (int j = 0; j < tileMapH; j++) {
                Slot slot = SLOT_GRASS;
                sf::Color tint = sf::Color::White;
                switch (world[i][j].terrain) {
                    case Terrain::Sand:       slot = SLOT_SAND; break;
                    case Terrain::Water:      slot = SLOT_WATER; break;
                    case Terrain::ForestPath: slot = SLOT_PATH; break;
                    case Terrain::Forest:     tint = sf::Color(150, 185, 150); break; // shaded forest floor
                    case Terrain::Rock:       tint = sf::Color(165, 160, 140); break; // stony ground
                    case Terrain::Grass:
                    default: break;
                }

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
            }
        }
        dirty = false;
    }

public:
    void markDirty() { dirty = true; }
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

    // Trees / boulders for natural tiles with no structure on them
    void drawTall(sf::RenderTarget& target, int i, int j) {
        Terrain t = world[i][j].terrain;
        sf::Sprite* sprite = (t == Terrain::Forest) ? &forestSprite : (t == Terrain::Rock ? &rockSprite : nullptr);
        if (sprite == nullptr || sprite->getTexture() == nullptr) return;
        sprite->setPosition(tileScreenPos(i, j));
        target.draw(*sprite);
    }
};
