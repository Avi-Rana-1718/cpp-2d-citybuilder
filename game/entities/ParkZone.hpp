#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"

class ParkZone : public Zone {
public:
    int serviceRadius = 4;

    ParkZone(int posX, int posY);
    ~ParkZone() = default;

    void update(float deltaTime) override;
    int dailyUpkeep() const override { return UPKEEP_PARK; }
    void drawGround(sf::RenderWindow& window) override;
    void drawBuilding(sf::RenderWindow& window) override;
};

inline ParkZone::ParkZone(int posX, int posY) {
    setName("ParkZone");
    isActive = true; // Public nature parks are inherently active
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
    if (m_textures.count("park")) {
        buildingSprite->setTexture(m_textures["park"]);
        if (m_textures["park"].getSize().y > 40) {
            buildingSprite->setOrigin(0.f, static_cast<float>(m_textures["park"].getSize().y - 40));
        } else if (m_textures["park"].getSize().y > 16) {
            buildingSprite->setOrigin(0.f, static_cast<float>(m_textures["park"].getSize().y - 16));
        }
        buildingSprite->setScale(sf::Vector2f(2.f, 2.f));
    }
}

inline void ParkZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
}

inline void ParkZone::drawGround(sf::RenderWindow& window) {
    if (sprite != NULL) {
        sprite->setPosition(position->x, position->y);
        window.draw(*sprite);
    }
}

inline void ParkZone::drawBuilding(sf::RenderWindow& window) {
    if (buildingSprite != NULL) {
        buildingSprite->setPosition(position->x - 16.f, position->y);
        window.draw(*buildingSprite);
    }
}
