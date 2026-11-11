#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <vector>
#include <map>
#include <iostream>

const float version = 2.0;
const std::string gameTitle="Chime V2";

// Preferred window size; the real size is screenW x screenH (window can be resized)
const int WINDOW_W=1500;
const int WINDOW_H=900;
const int MIN_WINDOW_W=1024;
const int MIN_WINDOW_H=680;
int preferredW=WINDOW_W, preferredH=WINDOW_H; // --size WxH
int screenW=WINDOW_W;
int screenH=WINDOW_H;

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
bool benchTown=false;  // --bench-town: a small starter town (~20 buildings) for balance checks
std::string savePath;  // --save file.sav: save the city when --bench ends
std::string loadPath;  // --load file.sav: open a saved city at start-up
bool benchMetro=false;  // --bench-metro: a zoned grid city with every service, for balance at ~2,000 people
bool starterCity=false; // --starter: bench town + road showcase (asphalt, avenue, roundabout), 60 days pre-simulated
// Screenshot helpers: --menu N opens build category N (1-5), --inspect X Y selects a tile, --view N sets the map view
bool showTitleMenu=false; // plain start (no options): open on the title screen
bool quitRequested=false; // a menu asked to quit
bool startStats=false;  // --stats: open the statistics panel (screenshots)
bool startLog=false;    // --messages: open the message log (screenshots)
bool startBudget=false; // --budget: open the budget panel (screenshots)
int startMenu=-1, startInspectX=-1, startInspectY=-1, startView=0;  // --bench-city: fill the map with a dense test city before benchmarking
int fastForwardDays=0; // --fast-forward N: simulate N days at start-up and print the economy trend
float startZoom=1.f; // --zoom Z: initial camera zoom (1 = default, 3.2 = max out)

std::map<std::string, sf::Texture> m_textures;
std::map<std::string, sf::Font> m_fonts;
std::map<std::string, sf::SoundBuffer> m_sounds;

#include "./Entity.hpp"

std::string townName="Untitled town";

int mayorFavorabilty[3] = {0, 0, 0};
// Tax rates live in game/simulation/EconomyData.hpp (taxRates[sector][wealth])

int townCapacity[3]={0, 0, 0};
int townPopulation[3] = {0, 0, 0};
int townDemand[3] = {0, 0, 0};
int townStats[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

std::vector<Entity*> bg;
Entity* entities[tileMapW][tileMapH]={NULL};
std::vector<Entity*> ui;
