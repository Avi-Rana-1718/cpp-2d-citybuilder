#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "_global.hpp"
#include "Entity.hpp"
#include "Scene.hpp"
#include "AssetManager.hpp"

#include "../game/scenes/Play_Scene.hpp"

class GameEngine {

    bool isRunning = true;
    Scene *currentScene;
    AssetManager assets;

    public:

    GameEngine();

    void run();
    void draw();
};

GameEngine::GameEngine() {
    _window.create(sf::VideoMode(WINDOW_W, WINDOW_H), gameTitle);
    view.setSize(sf::Vector2f(WINDOW_W, WINDOW_H));
    view.setCenter(sf::Vector2f(WINDOW_W / 2.f, WINDOW_H / 2.f));
    _window.setView(view);

    assets.addFont("../assets/Hack.ttf");
    assets.addTexture("grass", "grass.png");
    assets.addTexture("player", "player.png");
    assets.addTexture("road", "road.png");
    assets.addTexture("road-mid", "road-mid.png");
    assets.addTexture("residental", "residental.png");
    assets.addTexture("commercial", "commercial.png");
    assets.addTexture("industrial", "industrial.png");
    assets.addTexture("farm", "farm.png");
    assets.addTexture("school", "school.png");
    assets.addTexture("church", "church.png");
    assets.addTexture("park", "park.png");
    assets.addTexture("harbour", "harbour.png");
    assets.addTexture("water", "water.png");

    currentScene = new Play_Scene();
}

void GameEngine::run() {
    currentScene->init();
    gameTime.restart();

    while(isRunning) {
        sf::Time deltaTime = deltaClock.restart();
        frames++;

        if(framesClock.getElapsedTime().asSeconds() >= 1) {
            calculatedFPS = frames;
            frames = 0;
            framesClock.restart();
        }

        while(_window.pollEvent(_events)) {
            if(_events.type == sf::Event::Closed) {
                isRunning = false;
                _window.close();
            }
        }

        currentScene->update(deltaTime.asSeconds());
    } 
}
