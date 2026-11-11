#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"

class SubZone : public Zone {
public:
    SubZone(int posX, int posY, int rX, int rY, const std::string& typeName, const std::string& baseTexKey = "");
    ~SubZone() = default;

    void update(float deltaTime) override;
    void simTick() override;
    void drawGround(sf::RenderWindow& window) override;
    void drawBuilding(sf::RenderWindow& window) override;
};

inline SubZone::SubZone(int posX, int posY, int rX, int rY, const std::string& typeName, const std::string& baseTexKey) {
    setName(typeName);
    isActive = false;
    isMultiTile = true;
    isSubTile = true;
    rootX = rX;
    rootY = rY;
    level = 0;
    zoneBaseTexture = baseTexKey;

    this->posX = posX;
    this->posY = posY;

    position->x = tileSizeX * posX - posX * (tileSizeX / 2);
    position->y = tileSizeY * posY + posX * (tileSizeY / 2);

    delete shape;
    shape = NULL;
    delete text;
    text = NULL;

    sprite = new sf::Sprite;
    if (!zoneBaseTexture.empty() && m_textures.count(zoneBaseTexture)) {
        sprite->setTexture(m_textures[zoneBaseTexture]);
    } else {
        sprite->setTexture(m_textures["grass"]);
    }
    sprite->setScale(sf::Vector2f(2.f, 2.f));
}

inline void SubZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
}

// Mirror the root tile's road-access state (read by HappinessSystem)
inline void SubZone::simTick() {
    if (rootX >= 0 && rootX < tileMapW && rootY >= 0 && rootY < tileMapH) {
        if (entities[rootX][rootY] != NULL) {
            isActive = entities[rootX][rootY]->isActive;
        }
    }
}

inline void SubZone::drawGround(sf::RenderWindow& window) {
    if (sprite != NULL) {
        sprite->setPosition(position->x, position->y);
        window.draw(*sprite);
    }
}

inline void SubZone::drawBuilding(sf::RenderWindow& window) {
    // Only master root tile draws the 2x2 building complex
}
