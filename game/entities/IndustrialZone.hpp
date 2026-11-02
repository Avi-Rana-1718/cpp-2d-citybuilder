#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"
#include "../ui/Window.hpp"

class IndustrialZone : public Zone {
public:
    IndustrialZone(int posX, int posY, int initialTier = 1, int initialVariant = 1);
    ~IndustrialZone();

    void update(float deltaTime) override;
    void drawGround(sf::RenderWindow& window) override;
    void drawBuilding(sf::RenderWindow& window) override;

    void checkValid();
    void payTax() override;
    void upgradeTier(int newTier);
    void downgradeTier(int newTier);
    void updateBuildingTexture();
    std::string getVariantName() const;
    int pollutionEmission = 20;
};

inline IndustrialZone::IndustrialZone(int posX, int posY, int initialTier, int initialVariant) {
    setName("IndustrialZone");
    isActive = false;
    isMultiTile = true;
    isSubTile = false;
    footprintW = 2;
    footprintH = 2;
    rootX = posX;
    rootY = posY;

    tier = initialTier;
    variant = (initialVariant >= 1 && initialVariant <= 2) ? initialVariant : ((std::rand() % 2) + 1);
    level = 0;
    overallHappiness = 50;

    if (tier == 1) {
        capacity = (std::rand() % 10) + 15;
        zoneIncome = (std::rand() % 25) + 45;
        pollutionEmission = 18;
    } else if (tier == 2) {
        capacity = (std::rand() % 20) + 40;
        zoneIncome = (std::rand() % 35) + 110;
        pollutionEmission = 28;
    } else {
        capacity = (std::rand() % 30) + 90;
        zoneIncome = (std::rand() % 50) + 210;
        pollutionEmission = 0; // High-tech clean fab
    }
    population = 0;
    townCapacity[2] += capacity;
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

    zoneBaseTexture = "zone_industrial";
    sprite = new sf::Sprite;
    if (m_textures.count(zoneBaseTexture)) {
        sprite->setTexture(m_textures[zoneBaseTexture]);
    } else {
        sprite->setTexture(m_textures["grass"]);
    }
    sprite->setScale(sf::Vector2f(2.f, 2.f));

    buildingSprite = new sf::Sprite;
    updateBuildingTexture();

    std::cout << "\n" << getName() << " 2x2 entity created at (" << posX << ", " << posY << ") with ID: " << getID();
}

inline void IndustrialZone::updateBuildingTexture() {
    if (buildingSprite == NULL) return;
    std::string key = "ind_t" + std::to_string(tier) + "_v" + std::to_string(variant);
    if (!m_textures.count(key)) {
        key = "industrial";
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

inline std::string IndustrialZone::getVariantName() const {
    if (tier == 1) {
        return (variant == 1) ? "Scrap Workshop & Foundry" : "Storage Yard & Warehouse";
    } else if (tier == 2) {
        return (variant == 1) ? "Manufacturing Plant" : "Heavy Machine Assembly";
    } else {
        return (variant == 1) ? "Microchip Clean-Fab" : "Robotics Distribution Hub";
    }
}

inline void IndustrialZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
    checkValid();
    payTax();

    if (isActive) {
        if (townDemand[2] > 350 && tier < 3) {
            upgradeTier(3);
        } else if (townDemand[2] > 180 && tier == 1) {
            upgradeTier(2);
        } else if (townDemand[2] < 80 && tier > 1) {
            downgradeTier(tier - 1);
        }
    }
}

inline void IndustrialZone::drawGround(sf::RenderWindow& window) {
    if (sprite != NULL) {
        sprite->setPosition(position->x, position->y);
        window.draw(*sprite);
    }
}

inline void IndustrialZone::drawBuilding(sf::RenderWindow& window) {
    if (buildingSprite != NULL) {
        buildingSprite->setPosition(position->x, position->y);
        buildingSprite->setColor(isActive ? sf::Color(255, 255, 255, 255) : sf::Color(215, 215, 215, 175));
        window.draw(*buildingSprite);
    }
}

inline void IndustrialZone::checkValid() {
    bool hasRoad = false;
    for (int i = posX - 1; i <= posX + 2; i++) {
        if (i < 0 || i >= tileMapW) continue;
        for (int j = posY - 1; j <= posY + 2; j++) {
            if (j < 0 || j >= tileMapH || entities[i][j] == NULL) continue;
            if (entities[i][j] == this) continue;
            Zone* otherZ = dynamic_cast<Zone*>(entities[i][j]);
            if (otherZ != NULL && otherZ->isSubTile && otherZ->rootX == posX && otherZ->rootY == posY) continue;

            if (entities[i][j]->getName() == "DirtRoad") {
                hasRoad = true;
                break;
            }
        }
        if (hasRoad) break;
    }
    isActive = hasRoad;
}

inline void IndustrialZone::payTax() {
    if (isActive && !taxPaid && day == 1) {
        taxPaid = true;
        money += static_cast<int>(zoneIncome * (townTaxes[2] / 100.0));
    } else if (day == 2) {
        taxPaid = false;
    }
}

inline void IndustrialZone::upgradeTier(int newTier) {
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

    townCapacity[2] += (capacity - prevCapacity);
    updateBuildingTexture();
}

inline void IndustrialZone::downgradeTier(int newTier) {
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

    townCapacity[2] -= (prevCapacity - capacity);
    if (townCapacity[2] < 0) townCapacity[2] = 0;
    updateBuildingTexture();
}

inline IndustrialZone::~IndustrialZone() {
    townCapacity[2] -= capacity;
    if (townCapacity[2] < 0) townCapacity[2] = 0;
    if (tooltip != NULL) {
        delete tooltip;
        tooltip = NULL;
    }
}
