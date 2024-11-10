#pragma once
#include "../../src/Scene.hpp"
#include "../../src/_global.hpp"

#include "../Entities/Player.hpp"
#include "../Entities/_Zone.hpp"
#include "../Entities/ResidentalZone.hpp"
#include "../Entities/CommericalZone.hpp"
#include "../Entities/Roads/DirtRoad.hpp"
#include "../Entities/GlassTile.hpp"
#include "../UI/Window.hpp"

enum population {
    RESIDENTAL,//
    COMMERICAL,//
    INDUSTRIAL
};

enum populationStats {
    TAXES,
    POPULATION,
    EDUCATION,
    SAFETY, // combination of police and fire bridage
    HEALTH,
    RECREATION, // parks and misc
    TRAFFIC,
    EASEOFACCESS,
    JOBS
};

class Play_Scene : public Scene {
    //mayor can be removed and u will lose

    Entity* player=NULL;
    Entity* uiStats= NULL;
    Entity* uiMoney=NULL;
    Entity* uiTime=NULL;
    Entity* uiDebug = NULL;



    float lastActionTime;

    public:
    void init();
    void update(float deltaTime);

    void input();
    void refreshUI();

    void populateCity();
    void taxZone();

    
};

void Play_Scene::init() {
    std::cout<<"Scene created with name: Play_Scene";
 
    townDemand[0]=1000;
    townDemand[1]=500;
    lastActionTime=0;

    uiStats = new Window;
    uiStats->text->setCharacterSize(18);
    uiStats->position->x = 1000;
    uiStats->position->y = 500;
    ui.push_back(uiStats);

    uiMoney = new Window;
    uiMoney->text->setCharacterSize(18);
    uiMoney->position->x = 20;
    uiMoney->position->y = WINDOW_H - uiMoney->shape->getGlobalBounds().getSize().y- 40;
    ui.push_back(uiMoney);

    uiTime = new Window;
    uiTime->text->setCharacterSize(18);
    uiTime->position->x = 20 + uiMoney->shape->getGlobalBounds().getPosition().x + uiMoney->shape->getGlobalBounds().getSize().x;
    uiTime->position->y = WINDOW_H - uiMoney->text->getGlobalBounds().getSize().y- 40;
    ui.push_back(uiTime);

    uiDebug = new Window;
    uiDebug->text->setCharacterSize(18);
    uiDebug->position->x = WINDOW_W - uiDebug->shape->getGlobalBounds().getSize().x-20;
    uiDebug->position->y = 20;
    ui.push_back(uiDebug);

    player = new Player;
    ui.push_back(player);
    
    // ui.push_back(new Window());

    for(int i=0;i<tileMapW;i++) {
        for(int j=0;j<tileMapH;j++) {
            entities[i][j]=new GrassTile(i, j);
        }
    }

    //ui.push_back(Zone::tooltip);

    entities[10][4]=new ResidentalZone(10, 4);

    view.move(*entities[0][0]->position);
    _window.setView(view);
}

void Play_Scene::update(float deltaTime) {
    this->_deltaTime=deltaTime;
   
    _window.clear();
    for(int i=0;i<bg.size();i++) {
        bg[i]->update(deltaTime);
    }
    for(int i=0;i<tileMapW;i++) {
        for(int j=0;j<tileMapH;j++) {
            if(entities[i][j]!=NULL)
              entities[i][j]->update(deltaTime);
        }
    }
    for(int i=0;i<ui.size();i++) {
        ui[i]->update(deltaTime);
    }
    _window.display();

    input();
    refreshUI();

    //happen randomly
    if(std::rand()%1000==7 && (int)gameTime.getElapsedTime().asSeconds()%7==0)
       populateCity();
}

void Play_Scene::input() {
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::R) && gameTime.getElapsedTime().asSeconds()>lastActionTime+0.075) {
        delete entities[tileX][tileY];
        entities[tileX][tileY]=new ResidentalZone(tileX, tileY);
        lastActionTime=gameTime.getElapsedTime().asSeconds();
    }
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && gameTime.getElapsedTime().asSeconds()>lastActionTime+0.075) {
        delete entities[tileX][tileY];
        entities[tileX][tileY]=new DirtRoad(tileX, tileY);
        lastActionTime=gameTime.getElapsedTime().asSeconds();
    }
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::C) && gameTime.getElapsedTime().asSeconds()>lastActionTime+0.075) {
        delete entities[tileX][tileY];
        entities[tileX][tileY]=new CommericalZone(tileX, tileY);
        lastActionTime=gameTime.getElapsedTime().asSeconds();
    }
    // if(sf::Keyboard::isKeyPressed(sf::Keyboard::Z) && gameTime.getElapsedTime().asSeconds()>lastActionTime+0.075) {
    //     lastActionTime=gameTime.getElapsedTime().asSeconds();
    //     zoom+=0.1f;
    //     view.zoom(zoom);
    //     _window.setView(view);
    //     std::cout<<zoom<<std::endl;

    // }
    // if(sf::Keyboard::isKeyPressed(sf::Keyboard::X) && gameTime.getElapsedTime().asSeconds()>lastActionTime+0.075) {
    //     lastActionTime=gameTime.getElapsedTime().asSeconds();
    //     zoom-=0.1f;
    //     view.zoom(zoom);
    //     std::cout<<zoom<<std::endl;
    // }
    // if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && gameTime.getElapsedTime().asSeconds()>lastActionTime+0.075) {
    //     view.move(0, -tileSize);
    //     _window.setView(view);
    // }
}

void Play_Scene::refreshUI() {
    yr = gameTime.getElapsedTime().asSeconds()/1800;
    mon = (gameTime.getElapsedTime().asSeconds()-(yr*1800))/155;
    day =(gameTime.getElapsedTime().asSeconds()-(yr*1800)-(mon*155))/5;

    uiDebug->text->setString("Debug \nFPS: " + std::to_string(calculatedFPS) + "\nPosition: " + std::to_string(tileX) + " " + std::to_string(tileY));
    uiDebug->position->x = WINDOW_W - uiDebug->shape->getGlobalBounds().getSize().x-20;
    uiTime->text->setString(std::to_string(day)+"/"+std::to_string(mon)+"/"+std::to_string(yr));
    uiTime->position->x = 40 + uiMoney->shape->getGlobalBounds().getPosition().x + uiMoney->shape->getGlobalBounds().getSize().x;
    uiMoney->text->setString("$" + std::to_string(money));
    uiStats->text->setString(" Population:\n Residental: " + std::to_string(townPopulation[0]) + "\n Commerical: " + std::to_string(townPopulation[1]) + "\n Industrial: " + std::to_string(townPopulation[2]) + "\n\n Capacity\n Residental: " + std::to_string(townCapacity[0]) + "\n Commerical: " +std::to_string(townCapacity[1]) + "\n Industrial: " + std::to_string(townCapacity[2]) + "\n\n Demand\n Residental: " + std::to_string(townDemand[0]) + "\n Commerical: " +std::to_string(townDemand[1]) + "\n Industrial: " + std::to_string(townDemand[2]));

}

void Play_Scene::populateCity() {
    for(int i=0;i<3;i++) {
        int k = townCapacity[i]-townPopulation[i];
        if(townDemand[i]<=0 || k<=0)
            continue;

            int populationInc = (k>10)?(std::rand()%(k/10)+1):(std::rand()%(k)+1);
            townPopulation[i]+=populationInc;
            townDemand[i]-=populationInc;
    }
}