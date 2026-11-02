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

    // Zone Designation Ground Tiles
    assets.addTexture("zone_residental", "zone_residental.png");
    assets.addTexture("zone_commercial", "zone_commercial.png");
    assets.addTexture("zone_industrial", "zone_industrial.png");
    assets.addTexture("zone_farm", "zone_farm.png");

    // 2x2 Residential Tiers & Variants
    assets.addTexture("res_t1_v1", "res_t1_v1.png");
    assets.addTexture("res_t1_v2", "res_t1_v2.png");
    assets.addTexture("res_t2_v1", "res_t2_v1.png");
    assets.addTexture("res_t2_v2", "res_t2_v2.png");
    assets.addTexture("res_t3_v1", "res_t3_v1.png");
    assets.addTexture("res_t3_v2", "res_t3_v2.png");

    // 2x2 Commercial Tiers & Variants
    assets.addTexture("com_t1_v1", "com_t1_v1.png");
    assets.addTexture("com_t1_v2", "com_t1_v2.png");
    assets.addTexture("com_t2_v1", "com_t2_v1.png");
    assets.addTexture("com_t2_v2", "com_t2_v2.png");
    assets.addTexture("com_t3_v1", "com_t3_v1.png");
    assets.addTexture("com_t3_v2", "com_t3_v2.png");

    // 2x2 Industrial Tiers & Variants
    assets.addTexture("ind_t1_v1", "ind_t1_v1.png");
    assets.addTexture("ind_t1_v2", "ind_t1_v2.png");
    assets.addTexture("ind_t2_v1", "ind_t2_v1.png");
    assets.addTexture("ind_t2_v2", "ind_t2_v2.png");
    assets.addTexture("ind_t3_v1", "ind_t3_v1.png");
    assets.addTexture("ind_t3_v2", "ind_t3_v2.png");

    // 2x2 Farm Variants
    assets.addTexture("farm_v1", "farm_v1.png");
    assets.addTexture("farm_v2", "farm_v2.png");

    // Autonomous Vehicles (4 isometric directions & models)
    assets.addTexture("car_se", "car_se.png");
    assets.addTexture("car_nw", "car_nw.png");
    assets.addTexture("car_sw", "car_sw.png");
    assets.addTexture("car_ne", "car_ne.png");
    assets.addTexture("car_sedan_se", "car_sedan_se.png");
    assets.addTexture("car_sedan_nw", "car_sedan_nw.png");
    assets.addTexture("car_sedan_sw", "car_sedan_sw.png");
    assets.addTexture("car_sedan_ne", "car_sedan_ne.png");
    assets.addTexture("car_taxi_se", "car_taxi_se.png");
    assets.addTexture("car_taxi_nw", "car_taxi_nw.png");
    assets.addTexture("car_taxi_sw", "car_taxi_sw.png");
    assets.addTexture("car_taxi_ne", "car_taxi_ne.png");
    assets.addTexture("car_coupe_se", "car_coupe_se.png");
    assets.addTexture("car_coupe_nw", "car_coupe_nw.png");
    assets.addTexture("car_coupe_sw", "car_coupe_sw.png");
    assets.addTexture("car_coupe_ne", "car_coupe_ne.png");
    assets.addTexture("car_van_se", "car_van_se.png");
    assets.addTexture("car_van_nw", "car_van_nw.png");
    assets.addTexture("car_van_sw", "car_van_sw.png");
    assets.addTexture("car_van_ne", "car_van_ne.png");

    // Citizens & Pedestrians
    assets.addTexture("citizen_walk_0", "citizen_walk_0.png");
    assets.addTexture("citizen_walk_1", "citizen_walk_1.png");
    assets.addTexture("citizen_0_walk_0", "citizen_0_walk_0.png");
    assets.addTexture("citizen_0_walk_1", "citizen_0_walk_1.png");
    assets.addTexture("citizen_1_walk_0", "citizen_1_walk_0.png");
    assets.addTexture("citizen_1_walk_1", "citizen_1_walk_1.png");
    assets.addTexture("citizen_2_walk_0", "citizen_2_walk_0.png");
    assets.addTexture("citizen_2_walk_1", "citizen_2_walk_1.png");
    assets.addTexture("citizen_3_walk_0", "citizen_3_walk_0.png");
    assets.addTexture("citizen_3_walk_1", "citizen_3_walk_1.png");

    // Dynamic Building Animation Elements
    assets.addTexture("windmill_blades", "windmill_blades.png");
    assets.addTexture("smoke_puff_0", "smoke_puff_0.png");
    assets.addTexture("smoke_puff_1", "smoke_puff_1.png");
    assets.addTexture("smoke_puff_2", "smoke_puff_2.png");
    assets.addTexture("smoke_puff_3", "smoke_puff_3.png");
    assets.addTexture("fountain_0", "fountain_0.png");
    assets.addTexture("fountain_1", "fountain_1.png");
    assets.addTexture("fountain_2", "fountain_2.png");
    assets.addTexture("beacon_light", "beacon_light.png");
    assets.addTexture("harbour_crane_light", "harbour_crane_light.png");

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
