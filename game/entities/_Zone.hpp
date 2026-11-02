#pragma once
#include "../../src/Entity.hpp"
#include "../ui/Window.hpp"

class Zone : public Entity {
public:
    Entity* tooltip;
    int capacity;
    int population;

    int buildCost;
    int zoneIncome;
    bool taxPaid;
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

    std::string zoneBaseTexture = "";
    std::string buildingTexture = "";
    sf::Sprite* buildingSprite = NULL;

    Zone() {
        capacity = 0;
        population = 0;
        buildCost = 0;
        zoneIncome = 0;
        taxPaid = false;
        isHovered = false;
        tooltip = NULL;
        buildingSprite = NULL;
    }

    Zone(int posX, int posY);
    virtual ~Zone();

    virtual void update(float deltaTime) override;
    virtual void drawGround(sf::RenderWindow& window);
    virtual void drawBuilding(sf::RenderWindow& window);
    virtual void payTax();
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

inline void Zone::payTax() {
}
