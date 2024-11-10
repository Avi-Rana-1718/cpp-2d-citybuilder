#pragma once
#include "../_Zone.hpp"

class Road : public Zone {

    public:
    Road() {

    }
    Road(int posX, int posY);

    virtual void update(float deltaTime);
};

Road::Road(int posX, int posY) {
    setName("Road");
    text->setString(getName());

    this->posX=posX;
    this->posY=posY;

}

void Road::update(float deltaTime) {
    //
}