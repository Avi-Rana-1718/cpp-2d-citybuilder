#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"
#include "../world/WorldData.hpp"

class SchoolZone : public Zone {
public:
    int serviceRadius = 5;

    SchoolZone(int posX, int posY);
    ~SchoolZone() = default;

    void update(float deltaTime) override;
    int dailyUpkeep() const override { return UPKEEP_SCHOOL; }
    void drawGround(sf::RenderWindow& window) override;
    void drawBuilding(sf::RenderWindow& window) override;
    void checkValid() override;
};

inline SchoolZone::SchoolZone(int posX, int posY) {
    setName("SchoolZone");
    isActive = false;
    isMultiTile = true;
    isSubTile = false;
    footprintW = 2;
    footprintH = 2;
    rootX = posX;
    rootY = posY;
    serviceRadius = 5;
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

    buildingSprite = new sf::Sprite;
    if (m_textures.count("school")) {
        buildingSprite->setTexture(m_textures["school"]);
        if (m_textures["school"].getSize().y > 40) {
            buildingSprite->setOrigin(0.f, static_cast<float>(m_textures["school"].getSize().y - 40));
        } else if (m_textures["school"].getSize().y > 16) {
            buildingSprite->setOrigin(0.f, static_cast<float>(m_textures["school"].getSize().y - 16));
        }
        buildingSprite->setScale(sf::Vector2f(2.f, 2.f));
    }
}

inline void SchoolZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
}

inline void SchoolZone::drawGround(sf::RenderWindow& window) {
    if (sprite != NULL) {
        sprite->setPosition(position->x, position->y);
        window.draw(*sprite);
    }
}

inline void SchoolZone::drawBuilding(sf::RenderWindow& window) {
    if (buildingSprite != NULL) {
        buildingSprite->setPosition(position->x - 16.f, position->y);
        buildingSprite->setColor(isActive ? sf::Color(255, 255, 255, 255) : sf::Color(200, 200, 200, 140));
        window.draw(*buildingSprite);
    }
}

inline void SchoolZone::checkValid() {
    // Active when a road shares an edge with the 2x2 footprint
    isActive = hasAdjacentRoad(posX, posY);
}
