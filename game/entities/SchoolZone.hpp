#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"

class SchoolZone : public Zone {
public:
    int serviceRadius = 5;

    SchoolZone(int posX, int posY);
    ~SchoolZone() = default;

    void update(float deltaTime) override;
    void checkValid();
};

inline SchoolZone::SchoolZone(int posX, int posY) {
    setName("SchoolZone");
    isActive = false;
    serviceRadius = 5;
    level = 0;

    this->posX = posX;
    this->posY = posY;

    position->x = tileSizeX * posX - posX * (tileSizeX / 2);
    position->y = tileSizeY * posY + posX * (tileSizeY / 2);

    delete shape;
    shape = NULL;
    delete text;
    text = NULL;

    sprite = new sf::Sprite;
    sprite->setTexture(m_textures["school"]);
    if (m_textures["school"].getSize().y > 16) {
        sprite->setOrigin(0.f, static_cast<float>(m_textures["school"].getSize().y - 16));
    }
    sprite->setScale(sf::Vector2f(2.f, 2.f));
}

inline void SchoolZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
    checkValid();

    if (sprite != NULL) {
        sprite->setColor(isActive ? sf::Color(255, 255, 255, 255) : sf::Color(200, 200, 200, 130));
    }

    draw();
}

inline void SchoolZone::checkValid() {
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
