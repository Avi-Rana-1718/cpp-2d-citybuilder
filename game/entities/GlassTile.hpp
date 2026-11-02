#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"

class GrassTile : public Zone {

    public:
    GrassTile(int posX, int posY);
    
    void update(float deltaTime);

};

GrassTile::GrassTile(int posX, int posY) {
    setName("GrassTile");

    isActive = false;
    level = 0;

    this->posX = posX;
    this->posY = posY;

    position->x = tileSizeX * posX - posX * (tileSizeX / 2);
    position->y = tileSizeY * posY + posX * (tileSizeY / 2);

    delete shape;
    shape = NULL;
    delete text;
    text = NULL;

    std::cout << "\n" << getName() << " entity created with ID: " << getID();

    sprite = new sf::Sprite;
    sprite->setTexture(m_textures["grass"]);
    sprite->setScale(sf::Vector2f(2, 2));
}

void GrassTile::update(float deltaTime) {
    setDeltaTime(deltaTime);
    draw();
}
