#pragma once
#include "../../src/Entity.hpp"
#include "../UI/Window.hpp"

class Zone : public Entity {

    public:
    Entity* tooltip;
    int capacity;
    int population;

    int buildCost;
    int zoneIncome;
    bool taxPaid;

    bool isHovered;
    Zone() {
        capacity=0;
    }

    Zone(int posX, int posY);

    virtual void update(float deltaTime);
    virtual void payTax();

};

Zone::Zone(int posX, int posY) {
    setName("Zone");
    
    isActive=false;
    isHovered=false;

    tooltip=NULL;

    this->posX=posX;
    this->posY=posY;

    shape = new sf::RectangleShape(sf::Vector2f(tileSizeX, tileSizeY));
    shape->setFillColor(sf::Color::Red);
    shape->setOutlineThickness(0);

    std::cout<<"\n"<<getName()<<" entity created with ID: "<<getID();
}

void Zone::update(float deltaTime) {
    setDeltaTime(deltaTime);
    draw();
}
void Zone::payTax() {
}