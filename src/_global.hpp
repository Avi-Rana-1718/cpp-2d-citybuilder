#pragma once

const float version = 2.0;
const std::string gameTitle="Chime V2";

const int WINDOW_W=1500;
const int WINDOW_H=900;

sf::RenderWindow _window;
sf::Event _events;
sf::View view;
sf::Clock framesClock; 
sf::Clock deltaClock;
sf::Clock gameTime;

// game stuff
int yr, mon, day;
int money = 200;


int tileX=0;
int tileY=0;

float zoom=1.0;

const int tileMapW=20;
const int tileMapH=20;
const int tileSizeX=64;
const int tileSizeY=32;

int entityCount=0;
int frames=0;
int calculatedFPS=0;

std::map<std::string, sf::Texture> m_textures;
std::map<std::string, sf::Font> m_fonts;
std::map<std::string, sf::SoundBuffer> m_sounds;

#include "./Entity.hpp"

std::string townName="Untitled town";

int mayorFavorabilty[3] = {0, 0, 0}; // where 1st = residential, 2nd = commerical and 3rd = industrial population
int townTaxes[3]={5, 5, 5};


int townCapacity[3]={0, 0, 0};
int townPopulation[3] = {0, 0, 0}; // where 1st = residential, 2nd = commerical and 3rd = industrial population
int townDemand[3] = {0, 0, 0};
int townStats[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

std::vector<Entity*> bg;
Entity* entities[tileMapW][tileMapH]={NULL};
std::vector<Entity*> ui;