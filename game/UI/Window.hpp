#pragma once
#include "../../src/Entity.hpp"

class Window : public Entity {
    public:
    Window();
    
    void update(float deltaTime);
    void resize();
};

Window::Window() {
    setName("Window");
    //text->setString(getName());
    
    shape->setOutlineThickness(0);
    shape->setFillColor(sf::Color::Blue);

    //text=NULL;
    std::cout<<"\n"<<getName()<<" entity created with ID: "<<getID();
}

void Window::update(float deltaTime) {
    setDeltaTime(deltaTime);
    draw();

    resize();
    

}

void Window::resize() {
    shape = new sf::RectangleShape(sf::Vector2f(text->getGlobalBounds().getSize().x+20, text->getGlobalBounds().getSize().y+20));
    text->setOrigin(0, -10);
    shape->setFillColor(sf::Color::Blue);
}