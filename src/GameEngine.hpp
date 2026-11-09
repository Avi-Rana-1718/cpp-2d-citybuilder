#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <algorithm>
#include <cstdio>

#include "_global.hpp"
#include "Entity.hpp"
#include "Scene.hpp"
#include "AssetManager.hpp"

#include "../game/scenes/Play_Scene.hpp"

// Defined in src/HeadlessWindow.cpp: an off-screen, never-focused native window
void* createHeadlessWindowHandle(int width, int height);

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
    void* headlessHandle = headless ? createHeadlessWindowHandle(preferredW, preferredH) : nullptr;
    if (headlessHandle != nullptr) {
        _window.create(static_cast<sf::WindowHandle>(headlessHandle));
    } else {
        // Preferred size, shrunk to fit the desktop (leave room for borders / taskbar)
        sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        unsigned w = std::min<unsigned>(preferredW, desktop.width > 80 ? desktop.width - 40 : desktop.width);
        unsigned h = std::min<unsigned>(preferredH, desktop.height > 160 ? desktop.height - 120 : desktop.height);
        w = std::max<unsigned>(w, MIN_WINDOW_W);
        h = std::max<unsigned>(h, MIN_WINDOW_H);
        _window.create(sf::VideoMode(w, h), gameTitle);
        if (headless) _window.setVisible(false); // non-Windows fallback
    }
    screenW = static_cast<int>(_window.getSize().x);
    screenH = static_cast<int>(_window.getSize().y);
    view.setSize(sf::Vector2f(static_cast<float>(screenW), static_cast<float>(screenH)));
    view.setCenter(sf::Vector2f(screenW / 2.f, screenH / 2.f));
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

    // Natural terrain
    assets.addTexture("sand", "sand.png");
    assets.addTexture("forest", "forest.png");
    assets.addTexture("forest_autumn", "forest_autumn.png");
    assets.addTexture("forest_winter", "forest_winter.png");

    // HUD icons (tools/gen_ui_icons.py), registered as "ui_<name>"
    const char* uiIcons[] = {
        "inspect", "residential", "commercial", "industrial", "civic", "infrastructure", "road", "bulldoze",
        "coin", "people", "smile", "season_spring", "season_summer", "season_autumn", "season_winter",
        "view_normal", "view_fertility", "view_water", "view_deposits", "warning", "star",
        "speed_pause", "speed_1", "speed_2", "speed_4"};
    for (const char* name : uiIcons) {
        assets.addTexture(std::string("ui_") + name, std::string("ui/") + name + ".png");
    }
    assets.addTexture("rock", "rock.png");
    assets.addTexture("forest_path", "forest_path.png");

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

    currentScene = new Play_Scene();
}

void GameEngine::run() {
    currentScene->init();
    gameTime.restart();

    // Benchmark state (only used with --bench)
    const float benchWarmup = 1.0f;
    float benchElapsed = 0.f;
    float benchMeasured = 0.f;
    float benchWorst = 0.f;
    int benchFrames = 0;
    int benchWorstFrame = 0;
    float benchWorstAt = 0.f;

    while(isRunning) {
        sf::Time deltaTime = deltaClock.restart();
        frames++;

        if (benchSeconds > 0.f) {
            float dt = deltaTime.asSeconds();
            benchElapsed += dt;
            if (benchElapsed > benchWarmup) {
                benchMeasured += dt;
                if (dt > benchWorst) {
                    benchWorst = dt;
                    benchWorstFrame = benchFrames;
                    benchWorstAt = benchElapsed;
                }
                benchFrames++;
                if (benchMeasured >= benchSeconds) {
                    std::printf("\nBENCH frames=%d avg_ms=%.3f fps=%.1f worst_ms=%.3f worst_frame=%d worst_at_s=%.2f\n",
                                benchFrames, 1000.f * benchMeasured / benchFrames,
                                benchFrames / benchMeasured, 1000.f * benchWorst, benchWorstFrame, benchWorstAt);
                    std::fflush(stdout);
                    if (!benchShot.empty()) {
                        sf::Texture shot;
                        shot.create(_window.getSize().x, _window.getSize().y);
                        shot.update(_window);
                        shot.copyToImage().saveToFile(benchShot);
                    }
                    isRunning = false;
                    _window.close();
                    break;
                }
            }
        }

        if(framesClock.getElapsedTime().asSeconds() >= 1) {
            calculatedFPS = frames;
            frames = 0;
            framesClock.restart();
        }

        while(_window.pollEvent(_events)) {
            if(_events.type == sf::Event::Closed) {
                isRunning = false;
                _window.close();
            } else if (_events.type == sf::Event::Resized) {
                unsigned w = std::max<unsigned>(_events.size.width, MIN_WINDOW_W);
                unsigned h = std::max<unsigned>(_events.size.height, MIN_WINDOW_H);
                if (w != _events.size.width || h != _events.size.height) {
                    _window.setSize(sf::Vector2u(w, h)); // clamp; SFML sends another Resized
                }
                screenW = static_cast<int>(w);
                screenH = static_cast<int>(h);
                currentScene->onResize(w, h);
            }
        }

        currentScene->update(deltaTime.asSeconds());
    } 
}
