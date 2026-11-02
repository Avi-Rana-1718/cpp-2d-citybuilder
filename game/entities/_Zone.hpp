#pragma once
#include "../../src/Entity.hpp"
#include "../ui/Window.hpp"

class Zone : public Entity {

    public:
    Entity* tooltip;
    int capacity;
    int population;

    int buildCost;
    int zoneIncome;
    bool taxPaid;
    int happinessIndex[9]={};

    bool isHovered;
    bool isMultiTile = false;
    bool isSubTile = false;
    int rootX = -1;
    int rootY = -1;
    int footprintW = 1;
    int footprintH = 1;

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
