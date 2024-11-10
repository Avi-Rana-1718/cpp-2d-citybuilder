#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"
#include "../UI/Window.hpp"

class ResidentalZone : public Zone {

    public:
    ResidentalZone(int posX, int posY);
    ~ResidentalZone();

    void update(float deltaTime);

    void checkValid();
    void showTooltip();
    
    void payTax();

};

ResidentalZone::ResidentalZone(int posX, int posY) {
    setName("ResidentalZone");
    text->setString(getName());

        level=0;
        capacity=std::rand()%30;
        population=0;
        townCapacity[0]+=capacity;
        zoneIncome=(std::rand()%50) + 20;
        taxPaid=false;
        
        this->posX=posX;
        this->posY=posY;

        tooltip=NULL;

        position->x=tileSizeX*posX - posX*(tileSizeX/2);
        position->y=tileSizeY*posY + posX*(tileSizeY/2);

        shape = NULL;
        text=NULL;

        std::cout<<"\n"<<getName()<<" entity created with ID: "<<getID();

        sprite= new sf::Sprite;
        sprite->setTexture(m_textures["residental"]);
        sprite->setScale(sf::Vector2f(2, 2));
}

void ResidentalZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
    draw();
    checkValid();
    payTax();

    sprite->setColor((isActive)?(sf::Color(255, 255, 255, 255)):(sf::Color(255, 255, 255, 125)));

}

void ResidentalZone::checkValid()  {
    bool f=false;
    for(int i=posX-1;i<=posX+1;i++) {
        if(i==0||i==tileMapW){
            continue;
        }
        for(int j=posY-1;j<=posY+1;j++) {
            if(j==0||j==tileMapH||entities[i][j]==NULL || (posX==i&&posY==j)){
                continue;
            }

            if(entities[i][j]->getName()=="DirtRoad" || (entities[i][j]->getName()=="ResidentalZone" && entities[i][j]->isActive)) {
                f=true; 
                break;
            }
        }
    }

    isActive=f;

}

void ResidentalZone::payTax() {
    if(isActive && !taxPaid && day==1) {
        std::cout<<getName()<<" paid taxes!"<<std::endl;
        taxPaid=true;
        money+=(zoneIncome)*(townTaxes[0]/100.0);
    } else if(day==2) {
        taxPaid=false;
    }
}

void ResidentalZone::showTooltip() {

    // make one global entity callewd tooltip and manipulate that
    if(sprite->getGlobalBounds().contains(_window.mapPixelToCoords(sf::Mouse::getPosition(_window)))) {
        if(isHovered==false) {
            isHovered=true;
        }
    } else if (tooltip!=NULL) {
        isHovered=false;
        delete tooltip;
        tooltip=NULL;
    }
}

ResidentalZone::~ResidentalZone() {
       // townCapacity[0]-=capacity;
}