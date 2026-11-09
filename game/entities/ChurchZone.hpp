#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"
#include "../world/WorldData.hpp"

class ChurchZone : public Zone {
public:
    int serviceRadius = 4;

    ChurchZone(int posX, int posY);
    ~ChurchZone() = default;

    void update(float deltaTime) override;
    int dailyUpkeep() const override { return UPKEEP_CHURCH; }
    void drawGround(sf::RenderWindow& window) override;
    void drawBuilding(sf::RenderWindow& window) override;
    void checkValid() override;
};

inline ChurchZone::ChurchZone(int posX, int posY) {
    setName("ChurchZone");
    isActive = false;
    isMultiTile = true;
    isSubTile = false;
    footprintW = 2;
    footprintH = 2;
    rootX = posX;
    rootY = posY;
    serviceRadius = 4;
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
    if (m_textures.count("church")) {
        buildingSprite->setTexture(m_textures["church"]);
        if (m_textures["church"].getSize().y > 40) {
            buildingSprite->setOrigin(0.f, static_cast<float>(m_textures["church"].getSize().y - 40));
        } else if (m_textures["church"].getSize().y > 16) {
            buildingSprite->setOrigin(0.f, static_cast<float>(m_textures["church"].getSize().y - 16));
        }
        buildingSprite->setScale(sf::Vector2f(2.f, 2.f));
    }
}

inline void ChurchZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
}

inline void ChurchZone::drawGround(sf::RenderWindow& window) {
    if (sprite != NULL) {
        sprite->setPosition(position->x, position->y);
        window.draw(*sprite);
    }
}

inline void ChurchZone::drawBuilding(sf::RenderWindow& window) {
    if (buildingSprite != NULL) {
        buildingSprite->setPosition(position->x - 16.f, position->y);
        buildingSprite->setColor(isActive ? sf::Color(255, 255, 255, 255) : sf::Color(200, 200, 200, 140));
        window.draw(*buildingSprite);
    }
}

inline void ChurchZone::checkValid() {
    // Active when a road shares an edge with the 2x2 footprint
    isActive = hasAdjacentRoad(posX, posY);
}
