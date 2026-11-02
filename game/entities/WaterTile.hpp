#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"

class WaterTile : public Zone {
public:
    WaterTile(int posX, int posY);
    void update(float deltaTime) override;
};

WaterTile::WaterTile(int posX, int posY) {
    setName("WaterTile");
    isActive = true;
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
    sprite->setTexture(m_textures["water"]);
    sprite->setScale(sf::Vector2f(2, 2));
}

void WaterTile::update(float deltaTime) {
    setDeltaTime(deltaTime);
    draw();
}
