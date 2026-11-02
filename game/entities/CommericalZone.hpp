#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"

class CommericalZone : public Zone {
public:
    int tier = 1; // 1: Corner Shop, 2: Shopping Plaza, 3: Office Tower

    CommericalZone(int posX, int posY);
    ~CommericalZone();

    void update(float deltaTime) override;
    void checkValid();
    void payTax() override;
    void upgradeTier(int newTier);
};

CommericalZone::CommericalZone(int posX, int posY) {
    setName("CommericalZone");
    isActive = false;
    tier = 1;
    level = 0;

    capacity = (std::rand() % 25) + 15;
    population = 0;
    townCapacity[1] += capacity;
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
    sprite->setTexture(m_textures["commercial"]);
    if (m_textures["commercial"].getSize().y > 16) {
        sprite->setOrigin(0.f, static_cast<float>(m_textures["commercial"].getSize().y - 16));
    }
    sprite->setScale(sf::Vector2f(2, 2));

    std::cout << "\n" << getName() << " entity created with ID: " << getID();
}

void CommericalZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
    checkValid();
    payTax();
    
    if (sprite != NULL) {
        if (!isActive) {
            sprite->setColor(sf::Color(255, 120, 120, 140)); // Reddish tint when disconnected
        } else if (tier == 1) {
            sprite->setColor(sf::Color(255, 255, 255, 255));
        } else if (tier == 2) {
            sprite->setColor(sf::Color(255, 245, 160, 255)); // Warm commercial gold
        } else {
            sprite->setColor(sf::Color(160, 220, 255, 255)); // High-density modern cyan
        }
    }

    draw();
}

void CommericalZone::checkValid() {
    bool f = false;
    for (int i = posX - 1; i <= posX + 1; i++) {
        if (i < 0 || i >= tileMapW) continue;
        for (int j = posY - 1; j <= posY + 1; j++) {
            if (j < 0 || j >= tileMapH || entities[i][j] == NULL || (posX == i && posY == j)) {
                continue;
            }

            if (entities[i][j]->getName() == "DirtRoad" ||
               (entities[i][j]->getName() == "CommericalZone" && entities[i][j]->isActive)) {
                f = true;
                break;
            }
        }
        if (f) break;
    }
    isActive = f;
}

void CommericalZone::payTax() {
    if (isActive && !taxPaid && day == 1) {
        taxPaid = true;
        money += static_cast<int>(zoneIncome * (townTaxes[1] / 100.0));
    } else if (day == 2) {
        taxPaid = false;
    }
}

void CommericalZone::upgradeTier(int newTier) {
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

    townCapacity[1] += (capacity - prevCapacity);
}

CommericalZone::~CommericalZone() {
    townCapacity[1] -= capacity;
    if (townCapacity[1] < 0) townCapacity[1] = 0;
}
