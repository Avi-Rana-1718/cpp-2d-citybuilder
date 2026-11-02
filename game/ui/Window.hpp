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
    
    if (shape == NULL) {
        shape = new sf::RectangleShape(sf::Vector2f(100.f, 40.f));
    }
    shape->setOutlineThickness(0);
    shape->setFillColor(sf::Color::Blue);

    std::cout << "\n" << getName() << " entity created with ID: " << getID();
}

void Window::update(float deltaTime) {
    setDeltaTime(deltaTime);
    resize();
    draw();
}

void Window::resize() {
    if (shape != NULL && text != NULL) {
        sf::RectangleShape* rect = dynamic_cast<sf::RectangleShape*>(shape);
        if (rect != NULL) {
            rect->setSize(sf::Vector2f(text->getGlobalBounds().getSize().x + 20, text->getGlobalBounds().getSize().y + 20));
            rect->setFillColor(sf::Color::Blue);
        }
        text->setOrigin(0, -10);
    }
}
