#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"

class IndustrialZone : public Zone {
public:
    int tier = 1; // 1: Workshop, 2: Factory, 3: Heavy Industry

    IndustrialZone(int posX, int posY);
    ~IndustrialZone();

    void update(float deltaTime) override;
    void checkValid();
    void payTax() override;
    void upgradeTier(int newTier);
};

IndustrialZone::IndustrialZone(int posX, int posY) {
    setName("IndustrialZone");
    isActive = false;
    tier = 1;
    level = 0;

    capacity = (std::rand() % 25) + 15;
    population = 0;
    townCapacity[2] += capacity;
    zoneIncome = (std::rand() % 60) + 40;
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
    sprite->setTexture(m_textures["industrial"]);
    if (m_textures["industrial"].getSize().y > 16) {
        sprite->setOrigin(0.f, static_cast<float>(m_textures["industrial"].getSize().y - 16));
    }
    sprite->setScale(sf::Vector2f(2, 2));
}

void IndustrialZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
    checkValid();
    payTax();

    if (sprite != NULL) {
        if (!isActive) {
            sprite->setColor(sf::Color(180, 180, 180, 130));
        } else if (tier == 1) {
            sprite->setColor(sf::Color(255, 255, 255, 255));
        } else if (tier == 2) {
            sprite->setColor(sf::Color(255, 220, 180, 255)); // Warm factory glow
        } else {
            sprite->setColor(sf::Color(255, 170, 130, 255)); // Heavy industry red tint
        }
    }

    draw();
}

void IndustrialZone::checkValid() {
    bool f = false;
    for (int i = posX - 1; i <= posX + 1; i++) {
        if (i < 0 || i >= tileMapW) continue;
        for (int j = posY - 1; j <= posY + 1; j++) {
            if (j < 0 || j >= tileMapH || entities[i][j] == NULL || (posX == i && posY == j)) {
                continue;
            }

            if (entities[i][j]->getName() == "DirtRoad" ||
               (entities[i][j]->getName() == "IndustrialZone" && entities[i][j]->isActive)) {
                f = true;
                break;
            }
        }
        if (f) break;
    }
    isActive = f;
}

void IndustrialZone::payTax() {
    if (isActive && !taxPaid && day == 1) {
        taxPaid = true;
        money += static_cast<int>(zoneIncome * (townTaxes[2] / 100.0));
    } else if (day == 2) {
        taxPaid = false;
    }
}

void IndustrialZone::upgradeTier(int newTier) {
    if (newTier <= tier || newTier > 3) return;
    int prevCapacity = capacity;
    tier = newTier;
    level = tier - 1;

    if (tier == 2) {
        capacity = static_cast<int>(capacity * 1.8f);
        zoneIncome = static_cast<int>(zoneIncome * 2.0f);
    } else if (tier == 3) {
        capacity = static_cast<int>(capacity * 2.2f);
        zoneIncome = static_cast<int>(zoneIncome * 2.5f);
    }

    townCapacity[2] += (capacity - prevCapacity);
}

IndustrialZone::~IndustrialZone() {
    townCapacity[2] -= capacity;
    if (townCapacity[2] < 0) townCapacity[2] = 0;
}
