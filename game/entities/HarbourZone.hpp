#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"

class HarbourZone : public Zone {
public:
    HarbourZone(int posX, int posY);
    ~HarbourZone();

    void update(float deltaTime) override;
    void checkValid();
    void payTax() override;
};

HarbourZone::HarbourZone(int posX, int posY) {
    setName("HarbourZone");
    isActive = false;
    level = 0;

    capacity = 30; // 30 maritime trade & transport jobs
    population = 0;
    townCapacity[1] += capacity;
    zoneIncome = (std::rand() % 50) + 100;
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
    sprite->setTexture(m_textures["harbour"]);
    if (m_textures["harbour"].getSize().y > 16) {
        sprite->setOrigin(0.f, static_cast<float>(m_textures["harbour"].getSize().y - 16));
    }
    sprite->setScale(sf::Vector2f(2, 2));
}

void HarbourZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
    checkValid();
    payTax();

    if (sprite != NULL) {
        sprite->setColor(isActive ? sf::Color(255, 255, 255, 255) : sf::Color(200, 200, 200, 130));
    }

    draw();
}

void HarbourZone::checkValid() {
    bool f = false;
    for (int i = posX - 1; i <= posX + 1; i++) {
        if (i < 0 || i >= tileMapW) continue;
        for (int j = posY - 1; j <= posY + 1; j++) {
            if (j < 0 || j >= tileMapH || entities[i][j] == NULL || (posX == i && posY == j)) {
                continue;
            }

            if (entities[i][j]->getName() == "DirtRoad" ||
                entities[i][j]->getName() == "WaterTile" ||
               (entities[i][j]->getName() == "HarbourZone" && entities[i][j]->isActive)) {
                f = true;
                break;
            }
        }
        if (f) break;
    }
    isActive = f;
}

void HarbourZone::payTax() {
    if (isActive && !taxPaid && day == 1) {
        taxPaid = true;
        money += static_cast<int>(zoneIncome * (townTaxes[1] / 100.0));
    } else if (day == 2) {
        taxPaid = false;
    }
}

HarbourZone::~HarbourZone() {
    townCapacity[1] -= capacity;
    if (townCapacity[1] < 0) townCapacity[1] = 0;
}
