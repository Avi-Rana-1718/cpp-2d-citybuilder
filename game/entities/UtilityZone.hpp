#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"
#include "../world/WorldData.hpp"

// -------------------------------------------------------------
// Utility building (2x2): power plant, transformer or water facility
// -------------------------------------------------------------
// One class for all eight kinds (see UtilityKind in EconomyData.hpp).
// UtilitySystem reads `kind`, works out the effective output from the
// land and season, and keeps the result in `output` for the inspect card.
// A water tower also keeps its stored water here between days.

inline const char* utilityTextureKey(int kind) {
    static const char* keys[UTIL_KIND_COUNT] = {"util_coal", "util_wind", "util_solar", "util_transformer",
                                                "util_well", "util_pump", "util_treatment", "util_tower",
                                                "util_outflow", "util_sewage"};
    return (kind >= 0 && kind < UTIL_KIND_COUNT) ? keys[kind] : "util_coal";
}

inline const char* utilityTitle(int kind) {
    static const char* titles[UTIL_KIND_COUNT] = {"Coal Power Plant", "Wind Turbines", "Solar Farm", "Transformer",
                                                  "Water Well", "Underground Pump", "Water Treatment", "Water Tower",
                                                  "Sewage Outflow", "Sewage Treatment"};
    return (kind >= 0 && kind < UTIL_KIND_COUNT) ? titles[kind] : "Utility";
}

inline bool isPowerUtility(int kind) { return kind <= UTIL_TRANSFORMER; }

class UtilityZone : public Zone {
public:
    int kind = UTIL_WIND;
    float output = 0.f;   // effective output last computed (MW / units / capacity)
    float stored = 0.f;   // water tower contents
    bool linked = false;        // touches a road / line of its utility (set by UtilitySystem)
    bool noTransformer = false; // power plant on a grid without any transformer

    UtilityZone(int posX, int posY, int kind_);
    ~UtilityZone() = default;

    void update(float deltaTime) override { setDeltaTime(deltaTime); }
    int dailyUpkeep() const override { return UTIL_UPKEEP[kind]; }
    void drawGround(sf::RenderWindow& window) override;
    void drawBuilding(sf::RenderWindow& window) override;
    // Utilities need no road: they work once a road, power line or pipe reaches them
    void checkValid() override { isActive = true; }

    // Average live groundwater (0..1) in the draw radius around the footprint centre
    static float groundwaterAround(int x, int y, int radius) {
        float sum = 0.f;
        int n = 0;
        for (int dx = -radius; dx <= radius; dx++)
            for (int dy = -radius; dy <= radius; dy++) {
                if (dx * dx + dy * dy > radius * radius) continue;
                int tx = x + dx, ty = y + dy;
                if (!inWorldBounds(tx, ty) || world[tx][ty].terrain == Terrain::Water) continue;
                sum += world[tx][ty].groundwater;
                n++;
            }
        return n > 0 ? sum / (100.f * static_cast<float>(n)) : 0.f;
    }
    int drawRadius() const { return kind == UTIL_WELL ? WELL_RADIUS : (kind == UTIL_PUMP ? PUMP_RADIUS : 0); }
    bool onCoal() const {
        for (const auto& o : FOOTPRINT_2X2) {
            int tx = posX + o[0], ty = posY + o[1];
            if (inWorldBounds(tx, ty) && world[tx][ty].deposit == Deposit::Coal) return true;
        }
        return false;
    }
};

inline UtilityZone::UtilityZone(int posX, int posY, int kind_) {
    setName("UtilityZone");
    kind = kind_;
    isActive = false;
    isMultiTile = true;
    isSubTile = false;
    footprintW = 2;
    footprintH = 2;
    rootX = posX;
    rootY = posY;
    level = 0;
    this->posX = posX;
    this->posY = posY;
    position->x = tileSizeX * posX - posX * (tileSizeX / 2);
    position->y = tileSizeY * posY + posX * (tileSizeY / 2);

    delete shape;
    shape = NULL;
    delete text;
    text = NULL;

    zoneBaseTexture = "grass";
    sprite = new sf::Sprite;
    sprite->setTexture(m_textures["grass"]);
    sprite->setScale(sf::Vector2f(2.f, 2.f));

    const std::string key = utilityTextureKey(kind);
    buildingSprite = new sf::Sprite;
    if (m_textures.count(key)) {
        const sf::Texture& tex = m_textures[key];
        buildingSprite->setTexture(tex);
        if (tex.getSize().y > 40) buildingSprite->setOrigin(0.f, static_cast<float>(tex.getSize().y - 40));
        buildingSprite->setScale(sf::Vector2f(2.f, 2.f));
    }
}

inline void UtilityZone::drawGround(sf::RenderWindow& window) {
    if (sprite != NULL) {
        sprite->setPosition(position->x, position->y);
        window.draw(*sprite);
    }
}

inline void UtilityZone::drawBuilding(sf::RenderWindow& window) {
    if (buildingSprite != NULL) {
        buildingSprite->setPosition(position->x - 16.f, position->y);
        buildingSprite->setColor(isActive ? sf::Color(255, 255, 255, 255) : sf::Color(200, 200, 200, 140));
        window.draw(*buildingSprite);
    }
}
