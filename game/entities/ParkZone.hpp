#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"

class ParkZone : public Zone {
public:
    int serviceRadius = 4;

    ParkZone(int posX, int posY);
    ~ParkZone() = default;

    void update(float deltaTime) override;
    void checkValid();
};

ParkZone::ParkZone(int posX, int posY) {
    setName("ParkZone");
    isActive = true; // Natural park is accessible without strict road requirement
    serviceRadius = 4;
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
    sprite->setTexture(m_textures["park"]);
    if (m_textures["park"].getSize().y > 16) {
        sprite->setOrigin(0.f, static_cast<float>(m_textures["park"].getSize().y - 16));
    }
    sprite->setScale(sf::Vector2f(2, 2));
}

void ParkZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
    draw();
}
