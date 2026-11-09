#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"
#include "../world/WorldData.hpp"

class HarbourZone : public Zone {
public:
    HarbourZone(int posX, int posY);
    ~HarbourZone() = default;

    void update(float deltaTime) override;
    void simTick() override;
    void drawGround(sf::RenderWindow& window) override;
    void drawBuilding(sf::RenderWindow& window) override;
    void checkValid() override;
    void payTax() override;
};

inline HarbourZone::HarbourZone(int posX, int posY) {
    setName("HarbourZone");
    isActive = false;
    isMultiTile = true;
    isSubTile = false;
    footprintW = 2;
    footprintH = 2;
    rootX = posX;
    rootY = posY;

    level = 0;
    zoneIncome = 120;
    taxPaid = false;

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
    if (m_textures.count("harbour")) {
        buildingSprite->setTexture(m_textures["harbour"]);
        if (m_textures["harbour"].getSize().y > 40) {
            buildingSprite->setOrigin(0.f, static_cast<float>(m_textures["harbour"].getSize().y - 40));
        } else if (m_textures["harbour"].getSize().y > 16) {
            buildingSprite->setOrigin(0.f, static_cast<float>(m_textures["harbour"].getSize().y - 16));
        }
        buildingSprite->setScale(sf::Vector2f(2.f, 2.f));
    }
}

inline void HarbourZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
}

// Once per simulation tick: taxes and tier progression
inline void HarbourZone::simTick() {
    payTax();
}

inline void HarbourZone::drawGround(sf::RenderWindow& window) {
    if (sprite != NULL) {
        sprite->setPosition(position->x, position->y);
        window.draw(*sprite);
    }
}

inline void HarbourZone::drawBuilding(sf::RenderWindow& window) {
    if (buildingSprite != NULL) {
        buildingSprite->setPosition(position->x - 16.f, position->y);
        buildingSprite->setColor(isActive ? sf::Color(255, 255, 255, 255) : sf::Color(200, 200, 200, 140));
        window.draw(*buildingSprite);
    }
}

inline void HarbourZone::checkValid() {
    // Active when a road shares an edge with the 2x2 footprint
    isActive = hasAdjacentRoad(posX, posY) && hasAdjacentWater(posX, posY);
}

inline void HarbourZone::payTax() {
    if (isActive && !taxPaid && day == 1) {
        taxPaid = true;
        money += zoneIncome;
    } else if (day == 2) {
        taxPaid = false;
    }
}
