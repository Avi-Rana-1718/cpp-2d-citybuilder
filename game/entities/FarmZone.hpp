#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"
#include "../world/WorldData.hpp"

class FarmZone : public Zone {
public:
    float fertilityMul = 1.0f; // soil yield multiplier from world[][].fertility

    FarmZone(int posX, int posY, int initialVariant = 1);
    ~FarmZone();

    void update(float deltaTime) override;
    void simTick() override;
    void drawGround(sf::RenderWindow& window) override;
    void drawBuilding(sf::RenderWindow& window) override;

    void checkValid() override;
    void payTax() override;
    void updateBuildingTexture();
    std::string getVariantName() const;
};

inline FarmZone::FarmZone(int posX, int posY, int initialVariant) {
    setName("FarmZone");
    isActive = false;
    isMultiTile = true;
    isSubTile = false;
    footprintW = 2;
    footprintH = 2;
    rootX = posX;
    rootY = posY;

    tier = 1;
    variant = (initialVariant >= 1 && initialVariant <= 2) ? initialVariant : ((std::rand() % 2) + 1);
    level = 0;
    overallHappiness = 60;

    capacity = (variant == 1) ? 20 : 25;
    population = 0;
    townCapacity[2] += capacity;
    zoneIncome = (variant == 1) ? 35 : 45;
    fertilityMul = fertilityMultiplier(footprintFertility(posX, posY));
    zoneIncome = static_cast<int>(static_cast<float>(zoneIncome) * fertilityMul + 0.5f);
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

    zoneBaseTexture = "zone_farm";
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

inline void FarmZone::updateBuildingTexture() {
    if (buildingSprite == NULL) return;
    std::string key = "farm_v" + std::to_string(variant);
    if (!m_textures.count(key)) {
        key = "farm";
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

inline std::string FarmZone::getVariantName() const {
    return (variant == 1) ? "Barn & Wheat Silo" : "Windmill Homestead";
}

inline void FarmZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
}

// Once per simulation tick: taxes and tier progression
inline void FarmZone::simTick() {
    payTax();
}

inline void FarmZone::drawGround(sf::RenderWindow& window) {
    if (sprite != NULL) {
        sprite->setPosition(position->x, position->y);
        window.draw(*sprite);
    }
}

inline void FarmZone::drawBuilding(sf::RenderWindow& window) {
    if (buildingSprite != NULL) {
        buildingSprite->setPosition(position->x - 16.f, position->y);
        buildingSprite->setColor(isActive ? sf::Color(255, 255, 255, 255) : sf::Color(215, 215, 215, 175));
        window.draw(*buildingSprite);
    }
}

inline void FarmZone::checkValid() {
    // Active when a road shares an edge with the 2x2 footprint
    isActive = hasAdjacentRoad(posX, posY);
}

inline void FarmZone::payTax() {
    if (isActive && !taxPaid && day == 1) {
        taxPaid = true;
        money += static_cast<int>(zoneIncome * (townTaxes[2] / 100.0));
    } else if (day == 2) {
        taxPaid = false;
    }
}

inline FarmZone::~FarmZone() {
    townCapacity[2] -= capacity;
    if (townCapacity[2] < 0) townCapacity[2] = 0;
    if (tooltip != NULL) {
        delete tooltip;
        tooltip = NULL;
    }
}
