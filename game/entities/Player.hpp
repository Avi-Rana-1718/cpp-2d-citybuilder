#pragma once
#include "../../src/Entity.hpp"

class Player : public Entity {

    public: 
    Player();

    void update(float deltaTime);
};

Player::Player() {
    setName("Player");

    delete text;
    text = NULL;
    delete shape;
    shape = NULL;

    sprite = new sf::Sprite;
    sprite->setTexture(m_textures["player"]);
    sprite->setScale(sf::Vector2f(2, 2));

    isActive = true;
    std::cout << "\n" << getName() << " entity created with ID: " << getID();
}

void Player::update(float deltaTime) {
    setDeltaTime(deltaTime);
    if (isActive) {
        draw();
    }
}
