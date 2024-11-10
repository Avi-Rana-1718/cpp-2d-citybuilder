#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"

class CommericalZone : public Zone {

    public:
    CommericalZone(int posX, int posY);

    void update(float deltaTime);

    void checkValid();
    void payTax();

};

CommericalZone::CommericalZone(int posX, int posY) {
    setName("CommericalZone");
    text->setString(getName());

    isActive=false;
    level=0;

    capacity=std::rand()%30;
    population=0;
    townCapacity[1]+=capacity;
    zoneIncome=(std::rand()%70)+30;
    taxPaid=false;


    this->posX=posX;
    this->posY=posY;

    position->x=tileSizeX*posX;
    position->y=tileSizeY*posY;

    shape = new sf::RectangleShape(sf::Vector2f(tileSizeX, tileSizeY));
    shape->setOutlineThickness(2);

    std::cout<<"\n"<<getName()<<" entity created with ID: "<<getID();
}

void CommericalZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
    draw();
    checkValid();
    payTax();
    
    shape->setFillColor((isActive)?(sf::Color(253, 218, 13)):(sf::Color::Red));
}

void CommericalZone::checkValid()  {
    for(int i=posX-1;i<=posX+1;i++) {
        if(i==0||i==tileMapW){
            continue;
        }
        for(int j=posY-1;j<=posY+1;j++) {
            if(j==0||j==tileMapH||entities[i][j]==NULL || (posX==i&&posY==j)){
                continue;
            }

            if(entities[i][j]->getName()=="DirtRoad" || (entities[i][j]->getName()=="CommericalZone" && entities[i][j]->isActive)) {
                isActive=true;    
            }


        }
    }
}

void CommericalZone::payTax() {
    if(isActive && !taxPaid && day==1) {
        std::cout<<getName()<<" paid taxes!"<<std::endl;
        taxPaid=true;
        money+=(zoneIncome)*(townTaxes[0]/100.0);
    } else if(day==2) {
        taxPaid=false;
    }
}