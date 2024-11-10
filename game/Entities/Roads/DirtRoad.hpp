#pragma once
#include "./_Road.hpp"

class DirtRoad : public  Road {
    public:
    DirtRoad(int posY, int posX);

    void update(float deltaTime);
    void check();
};

DirtRoad::DirtRoad(int posX, int posY) {
    setName("DirtRoad");
    text->setString(std::to_string(tileX) + " " + std::to_string(tileY));

    this->posX=posX;
    this->posY=posY;

    shape=NULL;
    text=NULL;

    position->x=tileSizeX*posX - posX*(tileSizeX/2);
    position->y=tileSizeY*posY + posX*(tileSizeY/2);

    sprite= new sf::Sprite;
    sprite->setScale(sf::Vector2f(2, 2));
    std::cout<<"\n"<<getName()<<" entity created with ID: "<<getID()<<std::endl;


}

void DirtRoad::update(float deltaTime) {
    setDeltaTime(deltaTime);
    draw();
    check();
}

void DirtRoad::check() {

    sprite->setRotation(0);
    sprite->setOrigin(sf::Vector2f(0, 0));
    
    if(entities[posX-1][posY]->getName()=="DirtRoad" && entities[posX+1][posY]->getName()=="DirtRoad") {
        sprite->setTexture(m_textures["road-mid"]);
    } else if (entities[posX+1][posY-1]->getName()=="DirtRoad" && entities[posX-1][posY+1]->getName()=="DirtRoad") {
        sprite->setTexture(m_textures["road-mid"]);
        sprite->setScale(sf::Vector2f(-2, 2));
        sprite->setOrigin(sf::Vector2f(m_textures["road-mid"].getSize().x, 0));
    }  else if (entities[posX+1][posY]->getName()!="DirtRoad") {
        sprite->setTexture(m_textures["road"]);
        sprite->setRotation(180);
        sprite->setOrigin(sf::Vector2f(m_textures["road"].getSize().x, m_textures["road"].getSize().y));
    } else {
        sprite->setTexture(m_textures["road"]);
    }
}