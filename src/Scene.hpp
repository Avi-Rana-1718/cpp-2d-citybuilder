#pragma once
#include "Entity.hpp"
#include "_global.hpp"

class Scene {

    float lastAction;
    public:
    float _deltaTime;

    virtual void init();
    virtual void update(float deltaTime);
    // Called after the window is resized (screenW / screenH already updated)
    virtual void onResize(unsigned width, unsigned height) { (void)width; (void)height; }
    virtual void onClose() {}
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