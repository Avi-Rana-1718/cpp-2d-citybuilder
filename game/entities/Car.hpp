#pragma once
#include "../../src/Entity.hpp"

class Car : public Entity {

    public:
    Car();
    void update(float deltaTime);

};

Car::Car() {
    setName("Car");
    isActive = false;
}

void Car::update(float deltaTime) {
    setDeltaTime(deltaTime);
    draw();
}
