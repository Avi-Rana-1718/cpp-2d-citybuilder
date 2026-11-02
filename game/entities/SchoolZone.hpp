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
    isMultiTile = true;
    isSubTile = false;
    rootX = posX;
    rootY = posY;
    footprintW = 2;
    footprintH = 2;
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
    sprite->setOrigin(0.f, 24.f);
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
    bool hasRoad = false;
    for (int x = posX - 1; x <= posX + 2; x++) {
        if (x < 0 || x >= tileMapW) continue;
        for (int y = posY - 1; y <= posY + 2; y++) {
            if (y < 0 || y >= tileMapH) continue;
            if ((x >= posX && x <= posX + 1) && (y >= posY && y <= posY + 1)) continue;

            if (entities[x][y] != NULL) {
                if (entities[x][y]->getName() == "DirtRoad" ||
                   (entities[x][y]->getName() == "ResidentalZone" && entities[x][y]->isActive)) {
                    hasRoad = true;
                    break;
                }
            }
        }
        if (hasRoad) break;
    }
    isActive = hasRoad;
}
