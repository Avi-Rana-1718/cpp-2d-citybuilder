#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <vector>
#include <map>
#include <iostream>

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

const int tileMapW=64;
const int tileMapH=64;
const int tileSizeX=64;
const int tileSizeY=32;

int entityCount=0;
int frames=0;
int calculatedFPS=0;

// Benchmark mode (--bench N): run N seconds after warm-up, print frame stats, exit
float benchSeconds=0.f;
std::string benchShot; // --shot file.png: save a screenshot when --bench ends
bool headless=false;   // --headless (implied by --bench, undo with --show): off-screen window that never takes focus
bool benchCity=false;
// Screenshot helpers: --menu N opens build category N (1-5), --inspect X Y selects a tile, --view N sets the map view
int startMenu=-1, startInspectX=-1, startInspectY=-1, startView=0;  // --bench-city: fill the map with a dense test city before benchmarking
float startZoom=1.f; // --zoom Z: initial camera zoom (1 = default, 3.2 = max out)

std::map<std::string, sf::Texture> m_textures;
std::map<std::string, sf::Font> m_fonts;
std::map<std::string, sf::SoundBuffer> m_sounds;

#include "./Entity.hpp"

std::string townName="Untitled town";

int mayorFavorabilty[3] = {0, 0, 0};
int townTaxes[3]={5, 5, 5};

int townCapacity[3]={0, 0, 0};
int townPopulation[3] = {0, 0, 0};
int townDemand[3] = {0, 0, 0};
int townStats[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

std::vector<Entity*> bg;
Entity* entities[tileMapW][tileMapH]={NULL};
std::vector<Entity*> ui;
