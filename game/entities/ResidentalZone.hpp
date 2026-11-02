#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"
#include "../ui/Window.hpp"

class ResidentalZone : public Zone {
public:
    int tier = 1; // 1: Suburban Cottage, 2: Brick Apartments, 3: Highrise Estate
    int overallHappiness = 50;

    ResidentalZone(int posX, int posY);
    ~ResidentalZone();

    void update(float deltaTime) override;
    void checkValid();
    void showTooltip();
    void payTax() override;
    void upgradeTier(int newTier);
};

ResidentalZone::ResidentalZone(int posX, int posY) {
    setName("ResidentalZone");
    isActive = false;
    tier = 1;
    level = 0;
    overallHappiness = 50;

    capacity = (std::rand() % 20) + 10;
    population = 0;
    townCapacity[0] += capacity;
    zoneIncome = (std::rand() % 45) + 25;
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

    sprite = new sf::Sprite;
    sprite->setTexture(m_textures["residental"]);
    sprite->setScale(sf::Vector2f(2, 2));

    std::cout << "\n" << getName() << " entity created with ID: " << getID();
}

void ResidentalZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
    checkValid();
    payTax();

    if (sprite != NULL) {
        if (!isActive) {
            sprite->setColor(sf::Color(255, 255, 255, 120));
        } else if (tier == 1) {
            sprite->setColor(sf::Color(255, 255, 255, 255));
        } else if (tier == 2) {
            sprite->setColor(sf::Color(220, 255, 220, 255)); // Upgraded green tint
        } else {
            sprite->setColor(sf::Color(200, 240, 255, 255)); // Highrise estate tint
        }
    }

    draw();
}

void ResidentalZone::checkValid() {
    bool f = false;
    for (int i = posX - 1; i <= posX + 1; i++) {
        if (i < 0 || i >= tileMapW) continue;
        for (int j = posY - 1; j <= posY + 1; j++) {
            if (j < 0 || j >= tileMapH || entities[i][j] == NULL || (posX == i && posY == j)) {
                continue;
            }

            if (entities[i][j]->getName() == "DirtRoad" ||
               (entities[i][j]->getName() == "ResidentalZone" && entities[i][j]->isActive)) {
                f = true; 
                break;
            }
        }
        if (f) break;
    }

    isActive = f;
}

void ResidentalZone::payTax() {
    if (isActive && !taxPaid && day == 1) {
        taxPaid = true;
        money += static_cast<int>(zoneIncome * (townTaxes[0] / 100.0));
    } else if (day == 2) {
        taxPaid = false;
    }
}

void ResidentalZone::upgradeTier(int newTier) {
    if (newTier <= tier || newTier > 3) return;
    int prevCapacity = capacity;
    tier = newTier;
    level = tier - 1;

    if (tier == 2) {
        capacity = static_cast<int>(capacity * 2.0f);
        zoneIncome = static_cast<int>(zoneIncome * 2.2f);
    } else if (tier == 3) {
        capacity = static_cast<int>(capacity * 2.5f);
        zoneIncome = static_cast<int>(zoneIncome * 2.8f);
    }

    townCapacity[0] += (capacity - prevCapacity);
}

void ResidentalZone::showTooltip() {
    if (sprite == NULL) return;
    if (sprite->getGlobalBounds().contains(_window.mapPixelToCoords(sf::Mouse::getPosition(_window)))) {
        if (!isHovered) {
            isHovered = true;
        }
    } else if (tooltip != NULL) {
        isHovered = false;
        delete tooltip;
        tooltip = NULL;
    }
}

ResidentalZone::~ResidentalZone() {
    townCapacity[0] -= capacity;
    if (townCapacity[0] < 0) townCapacity[0] = 0;
    if (tooltip != NULL) {
        delete tooltip;
        tooltip = NULL;
    }
}
