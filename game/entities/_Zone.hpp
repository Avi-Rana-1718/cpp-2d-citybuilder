#pragma once
#include "../../src/Entity.hpp"
#include "../ui/Window.hpp"
#include "../simulation/EconomyData.hpp"

class Zone : public Entity {
public:
    Entity* tooltip;
    int capacity;
    int population;

    int buildCost;
    int zoneIncome;
    int happinessIndex[9] = {};

    bool isHovered;
    bool isMultiTile = false;
    bool isSubTile = false;
    int rootX = -1;
    int rootY = -1;
    int footprintW = 1;
    int footprintH = 1;

    int tier = 1;
    int variant = 1;
    int overallHappiness = 50;
    // Tier rules: buildings placed by hand keep their tier (fixedTier);
    // buildings grown on a zone upgrade up to the zone density's maxTier
    bool fixedTier = false;
    bool zoned = false;
    int maxTier = 3;
    int fireDays = 0;       // > 0 while burning (days so far)
    float depleteCarry = 0.f; // mines: fractional deposit used up

    std::string zoneBaseTexture = "";
    std::string buildingTexture = "";
    sf::Sprite* buildingSprite = NULL;

    Zone() {
        capacity = 0;
        population = 0;
        buildCost = 0;
        zoneIncome = 0;
        isHovered = false;
        tooltip = NULL;
        buildingSprite = NULL;
    }

    Zone(int posX, int posY);
    virtual ~Zone();

    virtual void update(float deltaTime) override;
    virtual void drawGround(sf::RenderWindow& window);
    virtual void drawBuilding(sf::RenderWindow& window);
    // ---- Economy ----
    // (sector, wealth) bucket for demand and taxes; sector -1 = not taxed
    virtual int sector() const { return -1; }
    virtual int wealth() const { return tier <= 1 ? WEALTH_LOW : (tier == 2 ? WEALTH_MID : WEALTH_HIGH); }
    // Building-specific income factor (farms: soil x season)
    virtual float incomeMultiplier() const { return 1.f; }
    // Daily running cost (roads and public services)
    virtual int dailyUpkeep() const { return 0; }

    // Tax paid per game day: occupants x income per occupant x bucket tax rate
    virtual float dailyTax() const {
        if (!isActive || isSubTile || population <= 0) return 0.f;
        int s = sector();
        if (s < 0) return 0.f;
        int w = wealth();
        return static_cast<float>(population) * INCOME_PER_CAPITA[s][w] *
               (static_cast<float>(taxRates[s][w]) / 100.f) * incomeMultiplier();
    }

    // Recomputes isActive (road access). Called when the building is placed
    // and when a road or water tile on its border changes, not every frame.
    virtual void checkValid() {}

    // Slow simulation step, run once per simulation tick (1 s) from
    // Play_Scene, not every frame: tax collection, tier changes, etc.
    virtual void simTick() {}
};

inline Zone::Zone(int posX, int posY) {
    setName("Zone");
    isActive = false;
    isHovered = false;
    tooltip = NULL;
    buildingSprite = NULL;

    this->posX = posX;
    this->posY = posY;

    position->x = tileSizeX * posX - posX * (tileSizeX / 2);
    position->y = tileSizeY * posY + posX * (tileSizeY / 2);

    delete shape;
    shape = NULL;
    delete text;
    text = NULL;

    std::cout << "\n" << getName() << " entity created with ID: " << getID();
}

inline Zone::~Zone() {
    delete buildingSprite;
    buildingSprite = NULL;
    delete tooltip;
    tooltip = NULL;
}

inline void Zone::update(float deltaTime) {
    setDeltaTime(deltaTime);
}

inline void Zone::drawGround(sf::RenderWindow& window) {
    if (sprite != NULL) {
        sprite->setPosition(position->x, position->y);
        window.draw(*sprite);
    }
}

inline void Zone::drawBuilding(sf::RenderWindow& window) {
    if (buildingSprite != NULL) {
        buildingSprite->setPosition(position->x, position->y);
        window.draw(*buildingSprite);
    }
}


