#pragma once
#include "./_Road.hpp"

class DirtRoad : public Road {
    public:
    DirtRoad(int posX, int posY);

    void update(float deltaTime);
    void check();
};

DirtRoad::DirtRoad(int posX, int posY) : Road(posX, posY) {
    setName("DirtRoad");

    delete shape;
    shape = NULL;
    delete text;
    text = NULL;

    position->x = tileSizeX * posX - posX * (tileSizeX / 2);
    position->y = tileSizeY * posY + posX * (tileSizeY / 2);

    sprite = new sf::Sprite;
    sprite->setTexture(m_textures["road"]);
    sprite->setScale(sf::Vector2f(2, 2));
    std::cout << "\n" << getName() << " entity created with ID: " << getID() << std::endl;
}

void DirtRoad::update(float deltaTime) {
    setDeltaTime(deltaTime);
    check();
    draw();
}

void DirtRoad::check() {
    if (sprite == NULL) return;

    sprite->setRotation(0);
    sprite->setOrigin(sf::Vector2f(0, 0));
    sprite->setScale(sf::Vector2f(2, 2));

    auto isRoad = [](int x, int y) -> bool {
        if (x < 0 || x >= tileMapW || y < 0 || y >= tileMapH) return false;
        return entities[x][y] != NULL && entities[x][y]->getName() == "DirtRoad";
    };

    bool left  = isRoad(posX - 1, posY);
    bool right = isRoad(posX + 1, posY);
    bool ne    = isRoad(posX + 1, posY - 1);
    bool sw    = isRoad(posX - 1, posY + 1);

    if (left && right) {
        sprite->setTexture(m_textures["road-mid"]);
    } else if (ne && sw) {
        sprite->setTexture(m_textures["road-mid"]);
        sprite->setScale(sf::Vector2f(-2, 2));
        sprite->setOrigin(sf::Vector2f(static_cast<float>(m_textures["road-mid"].getSize().x), 0));
    } else if (!right) {
        sprite->setTexture(m_textures["road"]);
        sprite->setRotation(180);
        sprite->setOrigin(sf::Vector2f(static_cast<float>(m_textures["road"].getSize().x),
                                       static_cast<float>(m_textures["road"].getSize().y)));
    } else {
        sprite->setTexture(m_textures["road"]);
    }
}
