#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"
#include "../ui/Window.hpp"

class ResidentalZone : public Zone {
public:
    ResidentalZone(int posX, int posY);
    ~ResidentalZone();

    void update(float deltaTime) override;
    void drawGround(sf::RenderWindow& window) override;
    void drawBuilding(sf::RenderWindow& window) override;

    void checkValid();
    void payTax() override;
    void upgradeTier(int newTier);
    void downgradeTier(int newTier);
    void updateBuildingTexture();
    std::string getVariantName() const;
};

inline ResidentalZone::ResidentalZone(int posX, int posY) {
    setName("ResidentalZone");
    isActive = false;
    isMultiTile = true;
    isSubTile = false;
    footprintW = 2;
    footprintH = 2;
    rootX = posX;
    rootY = posY;

    tier = 1;
    variant = (std::rand() % 2) + 1;
    level = 0;
    overallHappiness = 50;

    capacity = (std::rand() % 15) + 20; // 2x2 has 20-35 starting capacity
    population = 0;
    townCapacity[0] += capacity;
    zoneIncome = (std::rand() % 40) + 40;
    taxPaid = false;

    this->posX = posX;
    this->posY = posY;
    tooltip = NULL;

    position->x = tileSizeX * posX - posX * (tileSizeX / 2);
    position->y = tileSizeY * posY + posX * (tileSizeY / 2);

    delete shape;
    shape = NULL;
    delete text;
    text = NULL;

    // 1. Ground Tile: identical zone base
    zoneBaseTexture = "zone_residental";
    sprite = new sf::Sprite;
    if (m_textures.count(zoneBaseTexture)) {
        sprite->setTexture(m_textures[zoneBaseTexture]);
    } else {
        sprite->setTexture(m_textures["grass"]);
    }
    sprite->setScale(sf::Vector2f(2.f, 2.f));

    // 2. 3D Isometric Building Sprite
    buildingSprite = new sf::Sprite;
    updateBuildingTexture();

    std::cout << "\n" << getName() << " 2x2 entity created at (" << posX << ", " << posY << ") with ID: " << getID();
}

inline void ResidentalZone::updateBuildingTexture() {
    if (buildingSprite == NULL) return;
    std::string key = "res_t" + std::to_string(tier) + "_v" + std::to_string(variant);
    if (!m_textures.count(key)) {
        key = "residental";
    }

    if (m_textures.count(key)) {
        buildingSprite->setTexture(m_textures[key]);
        if (m_textures[key].getSize().y > 40) {
            buildingSprite->setOrigin(0.f, static_cast<float>(m_textures[key].getSize().y - 40));
        } else if (m_textures[key].getSize().y > 16) {
            buildingSprite->setOrigin(0.f, static_cast<float>(m_textures[key].getSize().y - 16));
        } else {
            buildingSprite->setOrigin(0.f, 0.f);
        }
        buildingSprite->setScale(sf::Vector2f(2.f, 2.f));
    }
}

inline std::string ResidentalZone::getVariantName() const {
    if (tier == 1) {
        return (variant == 1) ? "Suburban Cottages" : "Bungalow Homestead";
    } else if (tier == 2) {
        return (variant == 1) ? "Brick Brownstone Apartments" : "Urban Condominiums";
    } else {
        return (variant == 1) ? "Highrise Luxury Tower" : "Twin Highrise Plaza";
    }
}

inline void ResidentalZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
    checkValid();
    payTax();

    // Occurance & tier progression based on happiness
    if (isActive) {
        if (overallHappiness >= 80 && tier < 3) {
            upgradeTier(3);
        } else if (overallHappiness >= 60 && tier == 1) {
            upgradeTier(2);
        } else if (overallHappiness < 45 && tier > 1) {
            downgradeTier(tier - 1);
        }
    }
}

inline void ResidentalZone::drawGround(sf::RenderWindow& window) {
    if (sprite != NULL) {
        sprite->setPosition(position->x, position->y);
        window.draw(*sprite);
    }
}

inline void ResidentalZone::drawBuilding(sf::RenderWindow& window) {
    if (isActive && buildingSprite != NULL) {
        buildingSprite->setPosition(position->x, position->y);
        window.draw(*buildingSprite);
    }
}

inline void ResidentalZone::checkValid() {
    bool hasRoadOrActive = false;

    // Check perimeter (from posX-1 to posX+2, posY-1 to posY+2)
    for (int i = posX - 1; i <= posX + 2; i++) {
        if (i < 0 || i >= tileMapW) continue;
        for (int j = posY - 1; j <= posY + 2; j++) {
            if (j < 0 || j >= tileMapH || entities[i][j] == NULL) continue;
            if (entities[i][j] == this) continue;
            Zone* otherZ = dynamic_cast<Zone*>(entities[i][j]);
            if (otherZ != NULL && otherZ->isSubTile && otherZ->rootX == posX && otherZ->rootY == posY) continue;

            if (entities[i][j]->getName() == "DirtRoad" ||
               (entities[i][j]->getName() == "ResidentalZone" && entities[i][j]->isActive)) {
                hasRoadOrActive = true;
                break;
            }
        }
        if (hasRoadOrActive) break;
    }

    isActive = hasRoadOrActive;
}

inline void ResidentalZone::payTax() {
    if (isActive && !taxPaid && day == 1) {
        taxPaid = true;
        money += static_cast<int>(zoneIncome * (townTaxes[0] / 100.0));
    } else if (day == 2) {
        taxPaid = false;
    }
}

inline void ResidentalZone::upgradeTier(int newTier) {
    if (newTier <= tier || newTier > 3) return;
    int prevCapacity = capacity;
    tier = newTier;
    variant = (std::rand() % 2) + 1;
    level = tier - 1;

    if (tier == 2) {
        capacity = static_cast<int>(capacity * 2.2f);
        zoneIncome = static_cast<int>(zoneIncome * 2.2f);
    } else if (tier == 3) {
        capacity = static_cast<int>(capacity * 2.5f);
        zoneIncome = static_cast<int>(zoneIncome * 2.8f);
    }

    townCapacity[0] += (capacity - prevCapacity);
    updateBuildingTexture();
}

inline void ResidentalZone::downgradeTier(int newTier) {
    if (newTier >= tier || newTier < 1) return;
    int prevCapacity = capacity;
    tier = newTier;
    variant = (std::rand() % 2) + 1;
    level = tier - 1;

    if (tier == 2) {
        capacity = static_cast<int>(capacity * 0.6f);
        zoneIncome = static_cast<int>(zoneIncome * 0.6f);
    } else if (tier == 1) {
        capacity = 25;
        zoneIncome = 50;
    }

    townCapacity[0] -= (prevCapacity - capacity);
    if (townCapacity[0] < 0) townCapacity[0] = 0;
    updateBuildingTexture();
}

inline ResidentalZone::~ResidentalZone() {
    townCapacity[0] -= capacity;
    if (townCapacity[0] < 0) townCapacity[0] = 0;
    if (tooltip != NULL) {
        delete tooltip;
        tooltip = NULL;
    }
}
