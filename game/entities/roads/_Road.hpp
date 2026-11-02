#pragma once
#include "../_Zone.hpp"

class Road : public Zone {

    public:
    Road() {}
    Road(int posX, int posY);

    virtual void update(float deltaTime);
};

Road::Road(int posX, int posY) : Zone(posX, posY) {
    setName("Road");
    if (text != NULL) {
        text->setString(getName());
    }

    this->posX = posX;
    this->posY = posY;
}

void Road::update(float deltaTime) {
    Zone::update(deltaTime);
}
