#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"

class SubZone : public Zone {
public:
    SubZone(int posX, int posY, int rX, int rY, const std::string& typeName);
    ~SubZone() = default;

    void update(float deltaTime) override;
};

inline SubZone::SubZone(int posX, int posY, int rX, int rY, const std::string& typeName) {
    setName(typeName);
    isActive = false;
    isMultiTile = true;
    isSubTile = true;
    rootX = rX;
    rootY = rY;
    level = 0;

    this->posX = posX;
    this->posY = posY;

    position->x = tileSizeX * posX - posX * (tileSizeX / 2);
    position->y = tileSizeY * posY + posX * (tileSizeY / 2);

    delete shape;
    shape = NULL;
    delete text;
    text = NULL;
    delete sprite;
    sprite = NULL;
}

inline void SubZone::update(float deltaTime) {
    setDeltaTime(deltaTime);
    if (rootX >= 0 && rootX < tileMapW && rootY >= 0 && rootY < tileMapH) {
        if (entities[rootX][rootY] != NULL) {
            isActive = entities[rootX][rootY]->isActive;
        }
    }
}
