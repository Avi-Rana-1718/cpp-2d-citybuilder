#pragma once
#include "../../src/Entity.hpp"

class Player : public Entity {

    float lastActionTime;

    public: 
    Player();

    void update(float deltaTime);

    void input();
};

Player::Player() {
    setName("Player");
    text->setString(getName());

    text=NULL;
    shape=NULL;
    

    sprite = new sf::Sprite;
    sprite->setTexture(m_textures["player"]);
    sprite->setScale(sf::Vector2f(2, 2));


    lastActionTime=0;
    std::cout<<"\n"<<getName()<<" entity created with ID: "<<getID();
}

void Player::update(float deltaTime) {
    setDeltaTime(deltaTime);
    input();
    draw();
}

void Player::input() {
    if(gameTime.getElapsedTime().asSeconds()<lastActionTime+0.075) {
        return;
    }

    lastActionTime=gameTime.getElapsedTime().asSeconds();

    if(sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
        tileY--;
        position->y=entities[tileX][tileY]->position->y;
        position->x=entities[tileX][tileY]->position->x;
    }
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
        tileY++;
        position->y=entities[tileX][tileY]->position->y;
        position->x=entities[tileX][tileY]->position->x;
    }
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
        tileX--;
        position->y=entities[tileX][tileY]->position->y;
        position->x=entities[tileX][tileY]->position->x;
    }
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
        tileX++; 
        position->y=entities[tileX][tileY]->position->y;
        position->x=entities[tileX][tileY]->position->x;  
    }

}