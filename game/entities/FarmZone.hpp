#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"

class FarmZone : public Zone {
public:
    FarmZone(int posX, int posY);
    ~FarmZone();

    void update(float deltaTime) override;
    void checkValid();
    void payTax() override;
};

FarmZone::FarmZone(int posX, int posY) {
    setName("FarmZone");
    isActive = false;
    level = 0;

    capacity = 10;
    population = 0;
    townCapacity[2] += capacity; // Counts toward agricultural employment
    zoneIncome = (std::rand() % 30) + 20;
    taxPaid = false;

    this->posX = posX;
    this->posY = posY;

    position->x = tileSizeX * posX - posX * (tileSizeX / 2);
    position->y = tileSizeY * posY + posX * (tileSizeY / 2);

    delete shape;
    shape = NULL;
    delete text;
    text = NULL;

    sprite = new sf::Sprite;
    sprite->setTexture(m_textures["farm"]);
    sprite->setScale(sf::Vector2f(2, 2));
}

void FarmZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
    checkValid();
    payTax();

    if (sprite != NULL) {
        sprite->setColor(isActive ? sf::Color(255, 255, 255, 255) : sf::Color(200, 200, 200, 130));
    }

    draw();
}

void FarmZone::checkValid() {
    bool f = false;
    for (int i = posX - 1; i <= posX + 1; i++) {
        if (i < 0 || i >= tileMapW) continue;
        for (int j = posY - 1; j <= posY + 1; j++) {
            if (j < 0 || j >= tileMapH || entities[i][j] == NULL || (posX == i && posY == j)) {
                continue;
            }

            if (entities[i][j]->getName() == "DirtRoad" ||
               (entities[i][j]->getName() == "FarmZone" && entities[i][j]->isActive)) {
                f = true;
                break;
            }
        }
        if (f) break;
    }
    isActive = f;
}

void FarmZone::payTax() {
    if (isActive && !taxPaid && day == 1) {
        taxPaid = true;
        money += static_cast<int>(zoneIncome * (townTaxes[2] / 100.0));
    } else if (day == 2) {
        taxPaid = false;
    }
}

FarmZone::~FarmZone() {
    townCapacity[2] -= capacity;
    if (townCapacity[2] < 0) townCapacity[2] = 0;
}
