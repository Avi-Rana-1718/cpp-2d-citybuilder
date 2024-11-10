#pragma once
#include "Entity.hpp"
#include "_global.hpp"

class Scene {

    float lastAction;
    public:
    float _deltaTime;

    virtual void init();
    virtual void update(float deltaTime);
};

void Scene::init() {
    ui.clear();
   // entities.clear();
    bg.clear();
}

void Scene::update(float deltaTime) {
    this->_deltaTime=deltaTime;
    std::cout<<"\n In scene";
}