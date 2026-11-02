#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
#include "../../src/Scene.hpp"
#include "../../src/_global.hpp"

#include "../entities/Player.hpp"
#include "../entities/_Zone.hpp"
#include "../entities/ResidentalZone.hpp"
#include "../entities/CommericalZone.hpp"
#include "../entities/IndustrialZone.hpp"
#include "../entities/FarmZone.hpp"
#include "../entities/HarbourZone.hpp"
#include "../entities/WaterTile.hpp"
#include "../entities/SchoolZone.hpp"
#include "../entities/ChurchZone.hpp"
#include "../entities/ParkZone.hpp"
#include "../entities/SubZone.hpp"
#include "../entities/roads/DirtRoad.hpp"
#include "../entities/GlassTile.hpp"
#include "../simulation/HappinessSystem.hpp"
#include "../ui/Window.hpp"
#include "../ui/Toolbar.hpp"

enum population {
    RESIDENTAL,
    COMMERICAL,
    INDUSTRIAL
};

enum populationStats {
    TAXES,
    POPULATION,
    EDUCATION,
    SAFETY,
    HEALTH,
    RECREATION,
    TRAFFIC,
    EASEOFACCESS,
    JOBS
};

class Play_Scene : public Scene {
    Entity* player = NULL;
    Entity* uiStats = NULL;
    Entity* uiMoney = NULL;
    Entity* uiTime = NULL;
    Entity* uiDebug = NULL;

    Toolbar toolbar;
    HappinessSystem happinessSystem;

    float lastActionTime = 0.f;
    float cameraSpeed = 600.f;
    float simAccumulator = 0.f;

    // Drag-to-multibuild state
    bool isDragging = false;
    bool wasLeftPressed = false;
    sf::Vector2i dragStart{-1, -1};
    sf::Vector2i dragCurrent{-1, -1};

    // TheoTown Top Bar & Info Card State
    int gameSpeed = 1; // 0: Paused, 1: Normal, 2: Fast
    int lastDailyIncome = 45;

public:
    ~Play_Scene();
    void init() override;
    void update(float deltaTime) override;

private:
    void handleCameraMovement(float deltaTime);
    void handleToolHotkeys();
    bool handleTileHover(const sf::Vector2i& mousePixel, bool mouseOverUI);
    void handleGestures(bool isHoveringTile, bool mouseOverUI);

    void renderWorld(float deltaTime);
    void renderDragPreview();
    void renderHUD(float deltaTime);

    void refreshUI();
    void populateCity();

    std::vector<sf::Vector2i> computeDragTiles() const;
    void placeTile(int x, int y, BuildTool tool);
    void executeMultibuild();
    void buildTestWorld();
};

Play_Scene::~Play_Scene() {
    delete player;
    player = NULL;
}

void Play_Scene::init() {
    std::cout << "Scene created with name: Play_Scene\n";

    townDemand[0] = 1000;
    townDemand[1] = 500;
    townDemand[2] = 400;
    money = 5000;
    lastActionTime = 0.f;

    // Legacy Window pointers preserved for backwards compatibility and test stability
    uiStats = new Window;
    uiStats->text->setCharacterSize(14);
    uiStats->position->x = -9999;
    uiStats->position->y = -9999;
    ui.push_back(uiStats);

    uiMoney = new Window;
    uiMoney->text->setCharacterSize(18);
    uiMoney->position->x = -9999;
    uiMoney->position->y = -9999;
    ui.push_back(uiMoney);

    uiTime = new Window;
    uiTime->text->setCharacterSize(18);
    uiTime->position->x = -9999;
    uiTime->position->y = -9999;
    ui.push_back(uiTime);

    uiDebug = new Window;
    uiDebug->text->setCharacterSize(15);
    uiDebug->position->x = -9999;
    uiDebug->position->y = -9999;
    ui.push_back(uiDebug);

    // Initialize Tile Cursor
    player = new Player;

    // Fill map with default grass tiles
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            entities[i][j] = new GrassTile(i, j);
        }
    }

    // Build showcase test world with all sprites
    buildTestWorld();

    // Initialize TheoTown bottom dock
    toolbar.init(static_cast<float>(WINDOW_W), static_cast<float>(WINDOW_H));

    // Center camera on city center
    view.setSize(sf::Vector2f(static_cast<float>(WINDOW_W), static_cast<float>(WINDOW_H)));
    view.setCenter(sf::Vector2f(520.f, 500.f));
    _window.setView(view);
}

void Play_Scene::update(float deltaTime) {
    this->_deltaTime = deltaTime;

    // 1. Process Input
    handleCameraMovement(deltaTime);
    handleToolHotkeys();

    sf::Vector2i mousePixel = sf::Mouse::getPosition(_window);
    sf::Vector2f mousePixelF(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));

    // Check if mouse is interacting with UI elements
    bool mouseOverUI = toolbar.contains(mousePixelF) || (mousePixelF.y <= 50.f);

    bool isLeftPressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    if (isLeftPressed && !wasLeftPressed && mouseOverUI) {
        toolbar.handleClick(mousePixelF);
    }

    bool isHoveringTile = handleTileHover(mousePixel, mouseOverUI);
    handleGestures(isHoveringTile, mouseOverUI);

    refreshUI();

    // 2. Render Passes
    _window.clear(sf::Color(15, 20, 25));
    renderWorld(deltaTime);
    renderHUD(deltaTime);
    _window.display();

    // 3. Economy & Simulation Tick
    simAccumulator += deltaTime;
    if (simAccumulator >= 1.0f) {
        happinessSystem.updateSimulation();
        simAccumulator = 0.f;
    }

    if (std::rand() % 1000 == 7 && static_cast<int>(gameTime.getElapsedTime().asSeconds()) % 7 == 0) {
        populateCity();
    }
}

void Play_Scene::handleCameraMovement(float deltaTime) {
    sf::Vector2f camMove(0.f, 0.f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
        camMove.y -= cameraSpeed * deltaTime;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        camMove.y += cameraSpeed * deltaTime;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        camMove.x -= cameraSpeed * deltaTime;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        camMove.x += cameraSpeed * deltaTime;
    }

    if (camMove.x != 0.f || camMove.y != 0.f) {
        view.move(camMove);
    }
}

void Play_Scene::handleToolHotkeys() {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
        toolbar.setActiveTool(TOOL_NONE);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
        toolbar.setActiveTool(TOOL_RESIDENTIAL);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::C)) {
        toolbar.setActiveTool(TOOL_COMMERCIAL);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::I)) {
        toolbar.setActiveTool(TOOL_INDUSTRIAL);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
        toolbar.setActiveTool(TOOL_FARM);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::H)) {
        toolbar.setActiveTool(TOOL_HARBOUR);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && !sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
        // Only trigger water tool if shift is not pressed (to allow WASD pan)
        if (toolbar.getActiveTool() == TOOL_NONE) {
            toolbar.setActiveTool(TOOL_WATER);
        }
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) {
        toolbar.setActiveTool(TOOL_SCHOOL);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) {
        toolbar.setActiveTool(TOOL_CHURCH);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) {
        toolbar.setActiveTool(TOOL_PARK);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
        toolbar.setActiveTool(TOOL_ROAD);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::X)) {
        toolbar.setActiveTool(TOOL_BULLDOZE);
    }
}

bool Play_Scene::handleTileHover(const sf::Vector2i& mousePixel, bool mouseOverUI) {
    if (mouseOverUI) {
        if (player != NULL && !isDragging) {
            player->isActive = false;
        }
        return false;
    }

    sf::Vector2f mouseWorld = _window.mapPixelToCoords(mousePixel, view);
    int hoveredX = -1;
    int hoveredY = -1;

    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            if (entities[i][j] == NULL || entities[i][j]->position == NULL) continue;
            float px = entities[i][j]->position->x;
            float py = entities[i][j]->position->y;

            if (mouseWorld.x >= px && mouseWorld.x < px + tileSizeX &&
                mouseWorld.y >= py && mouseWorld.y < py + tileSizeY) {
                float dx = std::abs(mouseWorld.x - (px + tileSizeX / 2.0f));
                float dy = std::abs(mouseWorld.y - (py + tileSizeY / 2.0f));
                if ((dx / (tileSizeX / 2.0f)) + (dy / (tileSizeY / 2.0f)) <= 1.0f) {
                    hoveredX = i;
                    hoveredY = j;
                    break;
                }
            }
        }
        if (hoveredX != -1) break;
    }

    if (hoveredX != -1 && hoveredY != -1) {
        tileX = hoveredX;
        tileY = hoveredY;
        if (player != NULL) {
            player->isActive = true;
            player->position->x = entities[tileX][tileY]->position->x;
            player->position->y = entities[tileX][tileY]->position->y;
        }
        return true;
    }

    if (player != NULL && !isDragging) {
        player->isActive = false;
    }
    return false;
}

void Play_Scene::handleGestures(bool isHoveringTile, bool mouseOverUI) {
    float now = gameTime.getElapsedTime().asSeconds();
    bool isLeftPressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    bool isRightPressed = sf::Mouse::isButtonPressed(sf::Mouse::Right);

    // Right-click: abort drag or deselect tool
    if (isRightPressed) {
        if (isDragging) {
            isDragging = false;
            dragStart = {-1, -1};
        } else if (toolbar.getActiveTool() != TOOL_NONE) {
            toolbar.setActiveTool(TOOL_NONE);
        }
    }

    // Left-click Drag Gestures (World Only)
    if (!mouseOverUI && toolbar.getActiveTool() != TOOL_NONE) {
        if (isLeftPressed && !wasLeftPressed) {
            if (isHoveringTile) {
                isDragging = true;
                dragStart = {tileX, tileY};
                dragCurrent = {tileX, tileY};
            }
        } else if (isLeftPressed && wasLeftPressed) {
            if (isDragging && isHoveringTile) {
                dragCurrent = {tileX, tileY};
            }
        } else if (!isLeftPressed && wasLeftPressed) {
            if (isDragging) {
                if (isHoveringTile) {
                    dragCurrent = {tileX, tileY};
                }
                executeMultibuild();
                isDragging = false;
                dragStart = {-1, -1};
                lastActionTime = now;
            }
        }
    } else if (isLeftPressed && !wasLeftPressed && mouseOverUI) {
        isDragging = false;
    }

    wasLeftPressed = isLeftPressed;
}

std::vector<sf::Vector2i> Play_Scene::computeDragTiles() const {
    std::vector<sf::Vector2i> tiles;
    if (dragStart.x < 0 || dragStart.y < 0 || dragCurrent.x < 0 || dragCurrent.y < 0) {
        return tiles;
    }

    if (toolbar.getActiveTool() == TOOL_ROAD) {
        int stepX = (dragCurrent.x >= dragStart.x) ? 1 : -1;
        int stepY = (dragCurrent.y >= dragStart.y) ? 1 : -1;

        for (int x = dragStart.x; ; x += stepX) {
            tiles.push_back({x, dragStart.y});
            if (x == dragCurrent.x) break;
        }
        for (int y = dragStart.y + stepY; ; y += stepY) {
            if ((stepY > 0 && y > dragCurrent.y) || (stepY < 0 && y < dragCurrent.y)) break;
            tiles.push_back({dragCurrent.x, y});
            if (y == dragCurrent.y) break;
        }
    } else {
        int minX = std::max(0, std::min(dragStart.x, dragCurrent.x));
        int maxX = std::min(tileMapW - 1, std::max(dragStart.x, dragCurrent.x));
        int minY = std::max(0, std::min(dragStart.y, dragCurrent.y));
        int maxY = std::min(tileMapH - 1, std::max(dragStart.y, dragCurrent.y));

        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                tiles.push_back({x, y});
            }
        }
    }

    return tiles;
}

void Play_Scene::placeTile(int x, int y, BuildTool tool) {
    if (x < 0 || x >= tileMapW || y < 0 || y >= tileMapH) return;
    if (tool == TOOL_NONE) return;

    int cost = 0;
    switch (tool) {
        case TOOL_RES_COTTAGES:   cost = 100; break;
        case TOOL_RES_APARTMENTS: cost = 250; break;
        case TOOL_RES_TOWER:      cost = 600; break;
        case TOOL_COM_GROCERY:    cost = 120; break;
        case TOOL_COM_PLAZA:      cost = 280; break;
        case TOOL_COM_TOWER:      cost = 750; break;
        case TOOL_IND_WORKSHOP:   cost = 150; break;
        case TOOL_IND_FACTORY:    cost = 350; break;
        case TOOL_IND_TECH:       cost = 800; break;
        case TOOL_FARM_BARN:      cost = 70;  break;
        case TOOL_FARM_WINDMILL:  cost = 90;  break;
        case TOOL_ROAD:           cost = 10;  break;
        case TOOL_HARBOUR:        cost = 250; break;
        case TOOL_WATER:          cost = 20;  break;
        case TOOL_SCHOOL:         cost = 200; break;
        case TOOL_CHURCH:         cost = 180; break;
        case TOOL_PARK:           cost = 80;  break;
        case TOOL_BULLDOZE:       cost = 5;   break;
        default: return;
    }

    if (money < cost) return;

    // Helper to demolish any multi-tile or single structure cleanly
    auto demolishAt = [&](int tx, int ty) {
        if (tx < 0 || tx >= tileMapW || ty < 0 || ty >= tileMapH) return;
        if (entities[tx][ty] == NULL) return;
        Zone* z = dynamic_cast<Zone*>(entities[tx][ty]);
        if (z != NULL && z->isMultiTile) {
            int rx = z->isSubTile ? z->rootX : tx;
            int ry = z->isSubTile ? z->rootY : ty;
            if (rx >= 0 && rx + 1 < tileMapW && ry >= 0 && ry + 1 < tileMapH) {
                for (int dx = 0; dx <= 1; dx++) {
                    for (int dy = 0; dy <= 1; dy++) {
                        delete entities[rx + dx][ry + dy];
                        entities[rx + dx][ry + dy] = new GrassTile(rx + dx, ry + dy);
                    }
                }
            }
        } else {
            delete entities[tx][ty];
            entities[tx][ty] = new GrassTile(tx, ty);
        }
    };

    if (tool == TOOL_BULLDOZE) {
        if (entities[x][y] == NULL || entities[x][y]->getName() == "GrassTile") {
            return;
        }
        demolishAt(x, y);
        money -= cost;
        return;
    }

    // 1x1 Roads and Water
    if (tool == TOOL_ROAD || tool == TOOL_WATER) {
        demolishAt(x, y);
        delete entities[x][y];
        entities[x][y] = NULL;
        money -= cost;

        if (tool == TOOL_ROAD) {
            entities[x][y] = new DirtRoad(x, y);
            DirtRoad* r = dynamic_cast<DirtRoad*>(entities[x][y]);
            if (r != NULL) r->check();
        } else {
            entities[x][y] = new WaterTile(x, y);
        }
        return;
    }

    // 2x2 Structures (Residential, Commercial, Industrial, Farm, School, Church, Park, Harbour)
    if (x + 1 >= tileMapW || y + 1 >= tileMapH) return;

    // Demolish existing structures across the 2x2 area
    for (int dx = 0; dx <= 1; dx++) {
        for (int dy = 0; dy <= 1; dy++) {
            demolishAt(x + dx, y + dy);
        }
    }

    for (int dx = 0; dx <= 1; dx++) {
        for (int dy = 0; dy <= 1; dy++) {
            delete entities[x + dx][y + dy];
            entities[x + dx][y + dy] = NULL;
        }
    }

    money -= cost;

    std::string zoneBase = "grass";
    std::string typeName = "";
    Zone* masterZone = NULL;

    switch (tool) {
        case TOOL_RES_COTTAGES:
            masterZone = new ResidentalZone(x, y, 1, 1);
            zoneBase = "zone_residental";
            typeName = "ResidentalZone";
            break;
        case TOOL_RES_APARTMENTS:
            masterZone = new ResidentalZone(x, y, 2, 1);
            zoneBase = "zone_residental";
            typeName = "ResidentalZone";
            break;
        case TOOL_RES_TOWER:
            masterZone = new ResidentalZone(x, y, 3, 1);
            zoneBase = "zone_residental";
            typeName = "ResidentalZone";
            break;
        case TOOL_COM_GROCERY:
            masterZone = new CommericalZone(x, y, 1, 1);
            zoneBase = "zone_commercial";
            typeName = "CommericalZone";
            break;
        case TOOL_COM_PLAZA:
            masterZone = new CommericalZone(x, y, 2, 1);
            zoneBase = "zone_commercial";
            typeName = "CommericalZone";
            break;
        case TOOL_COM_TOWER:
            masterZone = new CommericalZone(x, y, 3, 1);
            zoneBase = "zone_commercial";
            typeName = "CommericalZone";
            break;
        case TOOL_IND_WORKSHOP:
            masterZone = new IndustrialZone(x, y, 1, 1);
            zoneBase = "zone_industrial";
            typeName = "IndustrialZone";
            break;
        case TOOL_IND_FACTORY:
            masterZone = new IndustrialZone(x, y, 2, 1);
            zoneBase = "zone_industrial";
            typeName = "IndustrialZone";
            break;
        case TOOL_IND_TECH:
            masterZone = new IndustrialZone(x, y, 3, 1);
            zoneBase = "zone_industrial";
            typeName = "IndustrialZone";
            break;
        case TOOL_FARM_BARN:
            masterZone = new FarmZone(x, y, 1);
            zoneBase = "zone_farm";
            typeName = "FarmZone";
            break;
        case TOOL_FARM_WINDMILL:
            masterZone = new FarmZone(x, y, 2);
            zoneBase = "zone_farm";
            typeName = "FarmZone";
            break;
        case TOOL_SCHOOL:
            masterZone = new SchoolZone(x, y);
            zoneBase = "grass";
            typeName = "SchoolZone";
            break;
        case TOOL_CHURCH:
            masterZone = new ChurchZone(x, y);
            zoneBase = "grass";
            typeName = "ChurchZone";
            break;
        case TOOL_PARK:
            masterZone = new ParkZone(x, y);
            zoneBase = "grass";
            typeName = "ParkZone";
            break;
        case TOOL_HARBOUR:
            masterZone = new HarbourZone(x, y);
            zoneBase = "grass";
            typeName = "HarbourZone";
            break;
        default:
            return;
    }

    entities[x][y] = masterZone;
    entities[x + 1][y]     = new SubZone(x + 1, y,     x, y, typeName, zoneBase);
    entities[x][y + 1]     = new SubZone(x,     y + 1, x, y, typeName, zoneBase);
    entities[x + 1][y + 1] = new SubZone(x + 1, y + 1, x, y, typeName, zoneBase);
}

void Play_Scene::executeMultibuild() {
    BuildTool tool = toolbar.getActiveTool();
    if (tool == TOOL_NONE) return;

    if (tool == TOOL_ROAD || tool == TOOL_WATER) {
        auto tiles = computeDragTiles();
        for (const auto& pt : tiles) {
            placeTile(pt.x, pt.y, tool);
        }
    } else {
        // 2x2 tool or single placement: place at dragStart (or dragCurrent)
        int gx = isDragging ? dragCurrent.x : tileX;
        int gy = isDragging ? dragCurrent.y : tileY;
        placeTile(gx, gy, tool);
    }

    // Refresh road orientations across map
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            if (entities[i][j] != NULL && entities[i][j]->getName() == "DirtRoad") {
                DirtRoad* road = dynamic_cast<DirtRoad*>(entities[i][j]);
                if (road != NULL) road->check();
            }
        }
    }
}

void Play_Scene::renderWorld(float deltaTime) {
    _window.setView(view);

    // Background
    for (size_t i = 0; i < bg.size(); i++) {
        if (bg[i] != NULL) bg[i]->update(deltaTime);
    }

    // Update simulation logic and status
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            if (entities[i][j] != NULL)
                entities[i][j]->update(deltaTime);
        }
    }

    // PASS 1: Render All Ground Tiles (Grass, Roads, Water, Zone Bases)
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            if (entities[i][j] != NULL) {
                Zone* z = dynamic_cast<Zone*>(entities[i][j]);
                if (z != NULL) {
                    z->drawGround(_window);
                } else {
                    entities[i][j]->draw();
                }
            }
        }
    }

    // PASS 2: Render All 3D Isometric Buildings in Isometric Depth Order (i + j)
    for (int depth = 0; depth < tileMapW + tileMapH; depth++) {
        for (int i = 0; i <= depth; i++) {
            int j = depth - i;
            if (i < tileMapW && j >= 0 && j < tileMapH) {
                if (entities[i][j] != NULL) {
                    Zone* z = dynamic_cast<Zone*>(entities[i][j]);
                    if (z != NULL && !z->isSubTile) {
                        z->drawBuilding(_window);
                    }
                }
            }
        }
    }

    // PASS 3: Drag & Build Ghost Preview & World Cursor
    renderDragPreview();

    if (player != NULL && player->isActive && !isDragging) {
        player->update(deltaTime);
    }
}

void Play_Scene::renderDragPreview() {
    BuildTool tool = toolbar.getActiveTool();
    if (tool == TOOL_NONE) return;

    sf::Sprite ghostSprite;

    // 1x1 Isometric Diamond (Four vertices forming pure 2:1 rhombus)
    sf::ConvexShape diamond1x1(4);
    diamond1x1.setPoint(0, sf::Vector2f(32.f, 0.f));
    diamond1x1.setPoint(1, sf::Vector2f(64.f, 16.f));
    diamond1x1.setPoint(2, sf::Vector2f(32.f, 32.f));
    diamond1x1.setPoint(3, sf::Vector2f(0.f, 16.f));

    auto highlightTile = [&](int tx, int ty, const sf::Color& fill, const sf::Color& outline) {
        if (tx < 0 || tx >= tileMapW || ty < 0 || ty >= tileMapH) return;
        if (entities[tx][ty] == NULL || entities[tx][ty]->position == NULL) return;
        diamond1x1.setPosition(entities[tx][ty]->position->x, entities[tx][ty]->position->y);
        diamond1x1.setFillColor(fill);
        diamond1x1.setOutlineColor(outline);
        diamond1x1.setOutlineThickness(1.5f);
        _window.draw(diamond1x1);
    };

    if (tool == TOOL_ROAD || tool == TOOL_WATER) {
        if (!isDragging) return;
        auto dragTiles = computeDragTiles();
        for (const auto& pt : dragTiles) {
            int gx = pt.x, gy = pt.y;
            if (gx < 0 || gx >= tileMapW || gy < 0 || gy >= tileMapH) continue;
            if (entities[gx][gy] == NULL || entities[gx][gy]->position == NULL) continue;
            float px = entities[gx][gy]->position->x;
            float py = entities[gx][gy]->position->y;

            std::string tkey = (tool == TOOL_ROAD) ? "road" : "water";
            if (m_textures.count(tkey)) {
                ghostSprite.setTexture(m_textures[tkey]);
                ghostSprite.setOrigin(0.f, 0.f);
                ghostSprite.setScale(2.f, 2.f);
                ghostSprite.setColor(sf::Color(255, 255, 255, 190));
                ghostSprite.setPosition(px, py);
                _window.draw(ghostSprite);
            }
        }
        return;
    }

    if (tool == TOOL_BULLDOZE) {
        int gx = isDragging ? dragCurrent.x : tileX;
        int gy = isDragging ? dragCurrent.y : tileY;
        if (gx >= 0 && gx < tileMapW && gy >= 0 && gy < tileMapH) {
            if (entities[gx][gy] != NULL && entities[gx][gy]->position != NULL) {
                Zone* z = dynamic_cast<Zone*>(entities[gx][gy]);
                if (z != NULL && z->isMultiTile) {
                    int rx = z->isSubTile ? z->rootX : gx;
                    int ry = z->isSubTile ? z->rootY : gy;
                    if (rx >= 0 && rx < tileMapW && ry >= 0 && ry < tileMapH) {
                        for (int dx = 0; dx <= 1; dx++) {
                            for (int dy = 0; dy <= 1; dy++) {
                                highlightTile(rx + dx, ry + dy, sf::Color(255, 60, 60, 140), sf::Color::Red);
                            }
                        }
                        return;
                    }
                }
                highlightTile(gx, gy, sf::Color(255, 60, 60, 140), sf::Color::Red);
            }
        }
        return;
    }

    // 2x2 Building Tools: Show individual 2:1 isometric diamond highlights and ghost building
    int gx = isDragging ? dragCurrent.x : tileX;
    int gy = isDragging ? dragCurrent.y : tileY;

    if (gx >= 0 && gx + 1 < tileMapW && gy >= 0 && gy + 1 < tileMapH) {
        if (entities[gx][gy] != NULL && entities[gx][gy]->position != NULL) {
            float px = entities[gx][gy]->position->x;
            float py = entities[gx][gy]->position->y;

            // Highlight all 4 constituent 2:1 isometric tile diamonds
            for (int dx = 0; dx <= 1; dx++) {
                for (int dy = 0; dy <= 1; dy++) {
                    highlightTile(gx + dx, gy + dy, sf::Color(46, 204, 113, 70), sf::Color(46, 204, 113, 220));
                }
            }

            std::string textureKey = "";
            switch (tool) {
                case TOOL_RES_COTTAGES:   textureKey = "res_t1_v1"; break;
                case TOOL_RES_APARTMENTS: textureKey = "res_t2_v1"; break;
                case TOOL_RES_TOWER:      textureKey = "res_t3_v1"; break;
                case TOOL_COM_GROCERY:    textureKey = "com_t1_v1"; break;
                case TOOL_COM_PLAZA:      textureKey = "com_t2_v1"; break;
                case TOOL_COM_TOWER:      textureKey = "com_t3_v1"; break;
                case TOOL_IND_WORKSHOP:   textureKey = "ind_t1_v1"; break;
                case TOOL_IND_FACTORY:    textureKey = "ind_t2_v1"; break;
                case TOOL_IND_TECH:       textureKey = "ind_t3_v1"; break;
                case TOOL_FARM_BARN:      textureKey = "farm_v1";   break;
                case TOOL_FARM_WINDMILL:  textureKey = "farm_v2";   break;
                case TOOL_SCHOOL:         textureKey = "school";    break;
                case TOOL_CHURCH:         textureKey = "church";    break;
                case TOOL_PARK:           textureKey = "park";      break;
                case TOOL_HARBOUR:        textureKey = "harbour";   break;
                default: break;
            }

            if (!textureKey.empty() && m_textures.count(textureKey)) {
                ghostSprite.setTexture(m_textures[textureKey]);
                if (m_textures[textureKey].getSize().y > 40) {
                    ghostSprite.setOrigin(0.f, static_cast<float>(m_textures[textureKey].getSize().y - 40));
                } else if (m_textures[textureKey].getSize().y > 16) {
                    ghostSprite.setOrigin(0.f, static_cast<float>(m_textures[textureKey].getSize().y - 16));
                } else {
                    ghostSprite.setOrigin(0.f, 0.f);
                }
                ghostSprite.setScale(2.f, 2.f);
                ghostSprite.setColor(sf::Color(255, 255, 255, 195));
                ghostSprite.setPosition(px, py);
                _window.draw(ghostSprite);
            }
        }
    }
}

void Play_Scene::renderHUD(float deltaTime) {
    _window.setView(_window.getDefaultView());

    const sf::Font& font = m_fonts["../assets/Hack.ttf"];

    // -------------------------------------------------------------
    // 1. TheoTown Top Header Bar (Fixed at top screen space)
    // -------------------------------------------------------------
    sf::RectangleShape topBar(sf::Vector2f(static_cast<float>(WINDOW_W), 48.f));
    topBar.setPosition(0.f, 0.f);
    topBar.setFillColor(sf::Color(14, 18, 25, 240));
    topBar.setOutlineColor(sf::Color(45, 60, 80, 220));
    topBar.setOutlineThickness(1.5f);
    _window.draw(topBar);

    // City Name Pill
    sf::Text titleText;
    titleText.setFont(font);
    titleText.setCharacterSize(15);
    titleText.setFillColor(sf::Color(52, 152, 219));
    titleText.setString("CHIME CITY");
    titleText.setPosition(20.f, 6.f);
    _window.draw(titleText);

    sf::Text approvalText;
    approvalText.setFont(font);
    approvalText.setCharacterSize(11);
    approvalText.setFillColor(sf::Color(180, 195, 210));
    approvalText.setString("Mayor: 88%");
    approvalText.setPosition(20.f, 26.f);
    _window.draw(approvalText);

    // Date & Speed Controls
    sf::Text dateText;
    dateText.setFont(font);
    dateText.setCharacterSize(13);
    dateText.setFillColor(sf::Color(230, 235, 240));
    dateText.setString("Yr " + std::to_string(yr) + " * M " + std::to_string(mon) + " * D " + std::to_string(day));
    dateText.setPosition(180.f, 15.f);
    _window.draw(dateText);

    // Treasury Badge
    sf::Text moneyText;
    moneyText.setFont(font);
    moneyText.setCharacterSize(17);
    moneyText.setFillColor(sf::Color(46, 204, 113));
    moneyText.setString("$" + std::to_string(money));
    moneyText.setPosition(390.f, 6.f);
    _window.draw(moneyText);

    sf::Text cashflowText;
    cashflowText.setFont(font);
    cashflowText.setCharacterSize(11);
    cashflowText.setFillColor(sf::Color(135, 211, 124));
    cashflowText.setString("+$45/day");
    cashflowText.setPosition(390.f, 28.f);
    _window.draw(cashflowText);

    // Population & RCI Demand Meter
    sf::Text popText;
    popText.setFont(font);
    popText.setCharacterSize(13);
    popText.setFillColor(sf::Color(240, 240, 245));
    popText.setString("Pop: " + std::to_string(townPopulation[0] + townPopulation[1] + townPopulation[2]));
    popText.setPosition(540.f, 15.f);
    _window.draw(popText);

    // Mini RCI Demand Bars
    float rciX = 660.f;
    float rciY = 16.f;
    // R (Green)
    sf::RectangleShape barR(sf::Vector2f(28.f, 16.f));
    barR.setPosition(rciX, rciY);
    barR.setFillColor(sf::Color(46, 204, 113));
    _window.draw(barR);
    // C (Blue)
    sf::RectangleShape barC(sf::Vector2f(22.f, 16.f));
    barC.setPosition(rciX + 32.f, rciY);
    barC.setFillColor(sf::Color(52, 152, 219));
    _window.draw(barC);
    // I (Orange)
    sf::RectangleShape barI(sf::Vector2f(18.f, 16.f));
    barI.setPosition(rciX + 58.f, rciY);
    barI.setFillColor(sf::Color(230, 126, 34));
    _window.draw(barI);

    // Happiness Score Pill
    int happy = happinessSystem.cityHappiness;
    sf::Color happyCol = (happy >= 75) ? sf::Color(46, 204, 113) : ((happy >= 50) ? sf::Color(241, 196, 15) : sf::Color(231, 76, 60));
    std::string happyIcon = (happy >= 75) ? ":D " : ((happy >= 50) ? ":) " : ":( ");

    sf::RectangleShape happyBadge(sf::Vector2f(135.f, 30.f));
    happyBadge.setPosition(770.f, 9.f);
    happyBadge.setFillColor(sf::Color(25, 32, 45));
    happyBadge.setOutlineColor(happyCol);
    happyBadge.setOutlineThickness(1.5f);
    _window.draw(happyBadge);

    sf::Text happyText;
    happyText.setFont(font);
    happyText.setCharacterSize(13);
    happyText.setFillColor(happyCol);
    happyText.setString(happyIcon + std::to_string(happy) + "% HAPPY");
    happyText.setPosition(780.f, 15.f);
    _window.draw(happyText);

    // FPS badge
    sf::Text fpsText;
    fpsText.setFont(font);
    fpsText.setCharacterSize(13);
    fpsText.setFillColor(sf::Color(140, 155, 170));
    fpsText.setString(std::to_string(calculatedFPS) + " FPS");
    fpsText.setPosition(static_cast<float>(WINDOW_W) - 85.f, 15.f);
    _window.draw(fpsText);

    // -------------------------------------------------------------
    // 2. TheoTown Building Inspection & Query Card (Inspect Mode)
    // -------------------------------------------------------------
    if (toolbar.getActiveTool() == TOOL_NONE && tileX >= 0 && tileX < tileMapW && tileY >= 0 && tileY < tileMapH) {
        if (entities[tileX][tileY] != NULL) {
            Zone* z = dynamic_cast<Zone*>(entities[tileX][tileY]);
            Zone* mz = z;
            if (z != NULL && z->isSubTile && z->rootX >= 0 && z->rootY >= 0) {
                mz = dynamic_cast<Zone*>(entities[z->rootX][z->rootY]);
            }

            if (mz != NULL) {
                float cardW = 340.f;
                float cardH = 220.f;
                float cardX = static_cast<float>(WINDOW_W) - cardW - 20.f;
                float cardY = 60.f;

                sf::RectangleShape card(sf::Vector2f(cardW, cardH));
                card.setPosition(cardX, cardY);
                card.setFillColor(sf::Color(16, 22, 30, 235));
                card.setOutlineColor(sf::Color(52, 152, 219, 200));
                card.setOutlineThickness(2.f);
                _window.draw(card);

                std::string headerTitle = mz->getName();
                std::string subTitle = mz->isMultiTile ? "2x2 Building Complex" : "1x1 Structure";
                std::string tierDesc = "Tier " + std::to_string(mz->tier);

                if (mz->getName() == "ResidentalZone") {
                    headerTitle = "RESIDENTIAL COMPLEX";
                    tierDesc += " * " + dynamic_cast<ResidentalZone*>(mz)->getVariantName();
                } else if (mz->getName() == "CommericalZone") {
                    headerTitle = "COMMERCIAL DISTRICT";
                    tierDesc += " * " + dynamic_cast<CommericalZone*>(mz)->getVariantName();
                } else if (mz->getName() == "IndustrialZone") {
                    headerTitle = "INDUSTRIAL FACILITY";
                    tierDesc += " * " + dynamic_cast<IndustrialZone*>(mz)->getVariantName();
                } else if (mz->getName() == "FarmZone") {
                    headerTitle = "AGRICULTURAL FARM";
                    tierDesc = dynamic_cast<FarmZone*>(mz)->getVariantName();
                }

                sf::Text cardTitle;
                cardTitle.setFont(font);
                cardTitle.setCharacterSize(14);
                cardTitle.setFillColor(sf::Color(52, 152, 219));
                cardTitle.setString(headerTitle);
                cardTitle.setPosition(cardX + 16.f, cardY + 12.f);
                _window.draw(cardTitle);

                sf::Text cardSub;
                cardSub.setFont(font);
                cardSub.setCharacterSize(11);
                cardSub.setFillColor(sf::Color(140, 160, 180));
                cardSub.setString(subTitle);
                cardSub.setPosition(cardX + 16.f, cardY + 32.f);
                _window.draw(cardSub);

                // Divider line
                sf::RectangleShape div(sf::Vector2f(cardW - 32.f, 1.f));
                div.setPosition(cardX + 16.f, cardY + 50.f);
                div.setFillColor(sf::Color(45, 60, 80));
                _window.draw(div);

                std::string infoBody =
                    "Designation: " + tierDesc + "\n"
                    "Footprint:   " + (mz->isMultiTile ? "2x2 Tiles" : "1x1 Tile") + "\n"
                    "Capacity:    " + std::to_string(mz->capacity) + " | Occupants: " + std::to_string(mz->population) + "\n"
                    "Tax Yield:   $" + std::to_string(mz->zoneIncome) + " / cycle\n"
                    "Road Status: " + (mz->isActive ? "Connected [Active]" : "No Road Access") + "\n"
                    "Local Happy: " + std::to_string(mz->overallHappiness) + "%\n"
                    "Services:    Edu: " + std::to_string(townStats[2]) + "% | Health: " + std::to_string(townStats[4]) + "%";

                sf::Text cardBody;
                cardBody.setFont(font);
                cardBody.setCharacterSize(11);
                cardBody.setFillColor(sf::Color(220, 230, 240));
                cardBody.setString(infoBody);
                cardBody.setPosition(cardX + 16.f, cardY + 58.f);
                _window.draw(cardBody);
            }
        }
    }

    // -------------------------------------------------------------
    // 3. TheoTown Bottom Floating Dock Toolbar
    // -------------------------------------------------------------
    sf::Vector2i mousePixel = sf::Mouse::getPosition(_window);
    sf::Vector2f mousePixelF(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));
    toolbar.draw(_window, font, mousePixelF);
}

void Play_Scene::refreshUI() {
    int totalSec = static_cast<int>(gameTime.getElapsedTime().asSeconds());
    yr = totalSec / 1800;
    mon = (totalSec - (yr * 1800)) / 155;
    day = (totalSec - (yr * 1800) - (mon * 155)) / 5;
}

void Play_Scene::populateCity() {
    for (int i = 0; i < 3; i++) {
        int k = townCapacity[i] - townPopulation[i];
        if (townDemand[i] <= 0 || k <= 0) {
            continue;
        }

        int populationInc = (k > 10) ? (std::rand() % (k / 10) + 1) : (std::rand() % k + 1);
        townPopulation[i] += populationInc;
        townDemand[i] -= populationInc;
    }
}

void Play_Scene::buildTestWorld() {
    auto setRoad = [&](int x, int y) {
        if (x >= 0 && x < tileMapW && y >= 0 && y < tileMapH) {
            delete entities[x][y];
            entities[x][y] = new DirtRoad(x, y);
        }
    };

    auto set2x2 = [&](int rx, int ry, BuildTool tool, int tier = 1, int variant = 1) {
        if (rx < 0 || rx + 1 >= tileMapW || ry < 0 || ry + 1 >= tileMapH) return;
        for (int dx = 0; dx <= 1; dx++) {
            for (int dy = 0; dy <= 1; dy++) {
                delete entities[rx + dx][ry + dy];
                entities[rx + dx][ry + dy] = NULL;
            }
        }
        Zone* mz = NULL;
        std::string baseTex = "grass";
        std::string typeName = "";
        if (tool == TOOL_RES_COTTAGES || tool == TOOL_RES_APARTMENTS || tool == TOOL_RES_TOWER || tool == TOOL_RESIDENTIAL) {
            mz = new ResidentalZone(rx, ry, tier, variant);
            baseTex = "zone_residental";
            typeName = "ResidentalZone";
        } else if (tool == TOOL_COM_GROCERY || tool == TOOL_COM_PLAZA || tool == TOOL_COM_TOWER || tool == TOOL_COMMERCIAL) {
            mz = new CommericalZone(rx, ry, tier, variant);
            baseTex = "zone_commercial";
            typeName = "CommericalZone";
        } else if (tool == TOOL_IND_WORKSHOP || tool == TOOL_IND_FACTORY || tool == TOOL_IND_TECH || tool == TOOL_INDUSTRIAL) {
            mz = new IndustrialZone(rx, ry, tier, variant);
            baseTex = "zone_industrial";
            typeName = "IndustrialZone";
        } else if (tool == TOOL_FARM_BARN || tool == TOOL_FARM_WINDMILL || tool == TOOL_FARM) {
            mz = new FarmZone(rx, ry, variant);
            baseTex = "zone_farm";
            typeName = "FarmZone";
        } else if (tool == TOOL_SCHOOL) {
            mz = new SchoolZone(rx, ry);
            typeName = "SchoolZone";
        } else if (tool == TOOL_CHURCH) {
            mz = new ChurchZone(rx, ry);
            typeName = "ChurchZone";
        } else if (tool == TOOL_PARK) {
            mz = new ParkZone(rx, ry);
            typeName = "ParkZone";
        } else if (tool == TOOL_HARBOUR) {
            mz = new HarbourZone(rx, ry);
            typeName = "HarbourZone";
        }

        if (mz != NULL) {
            entities[rx][ry] = mz;
            entities[rx + 1][ry]     = new SubZone(rx + 1, ry,     rx, ry, typeName, baseTex);
            entities[rx][ry + 1]     = new SubZone(rx,     ry + 1, rx, ry, typeName, baseTex);
            entities[rx + 1][ry + 1] = new SubZone(rx + 1, ry + 1, rx, ry, typeName, baseTex);
        }
    };

    // 1. 1x1 Roads Network
    // North Avenue (y = 3)
    for (int x = 1; x <= 18; x++) setRoad(x, 3);
    // Central Boulevard (y = 6)
    for (int x = 1; x <= 18; x++) setRoad(x, 6);
    // South Shore Road (y = 9)
    for (int x = 1; x <= 18; x++) setRoad(x, 9);
    // West Avenue Link
    for (int y = 3; y <= 9; y++) setRoad(1, y);
    // East Avenue Link
    for (int y = 3; y <= 9; y++) setRoad(18, y);

    // 2. Row 1 (y = 1): Residential Sector — ALL BUILDINGS SIDE BY SIDE!
    set2x2(2,  1, TOOL_RES_COTTAGES,   1, 1); // T1 Suburban Cottages v1
    set2x2(4,  1, TOOL_RES_COTTAGES,   1, 2); // T1 Bungalow Homestead v2
    set2x2(6,  1, TOOL_RES_APARTMENTS, 2, 1); // T2 Brick Brownstone Apartments v1
    set2x2(8,  1, TOOL_RES_APARTMENTS, 2, 2); // T2 Urban Condominiums v2
    set2x2(10, 1, TOOL_RES_TOWER,      3, 1); // T3 Highrise Luxury Tower v1
    set2x2(12, 1, TOOL_RES_TOWER,      3, 2); // T3 Twin Highrise Plaza v2
    set2x2(14, 1, TOOL_PARK);                 // Grand Nature Park
    set2x2(16, 1, TOOL_CHURCH);               // Community Cathedral

    // 3. Row 2 (y = 4): Commercial Downtown — ALL BUILDINGS SIDE BY SIDE!
    set2x2(2,  4, TOOL_COM_GROCERY, 1, 1); // T1 Corner Grocery & Bakery v1
    set2x2(4,  4, TOOL_COM_GROCERY, 1, 2); // T1 Local Diner & Mini Mart v2
    set2x2(6,  4, TOOL_COM_PLAZA,   2, 1); // T2 Shopping Plaza Arcade v1
    set2x2(8,  4, TOOL_COM_PLAZA,   2, 2); // T2 Department Store Arcade v2
    set2x2(10, 4, TOOL_COM_TOWER,   3, 1); // T3 Corporate Banking Highrise v1
    set2x2(12, 4, TOOL_COM_TOWER,   3, 2); // T3 International Financial Center v2
    set2x2(14, 4, TOOL_SCHOOL);             // Elementary School
    set2x2(16, 4, TOOL_PARK);               // Downtown Plaza Park

    // 4. Row 3 (y = 7): Agriculture & Industry — ALL BUILDINGS SIDE BY SIDE!
    set2x2(2,  7, TOOL_FARM_BARN,     1, 1); // Barn & Wheat Silo v1
    set2x2(4,  7, TOOL_FARM_WINDMILL, 1, 2); // Windmill Homestead v2
    set2x2(6,  7, TOOL_IND_WORKSHOP,   1, 1); // T1 Scrap Workshop & Foundry v1
    set2x2(8,  7, TOOL_IND_WORKSHOP,   1, 2); // T1 Warehouse & Logistics Depot v2
    set2x2(10, 7, TOOL_IND_FACTORY,    2, 1); // T2 Manufacturing Plant v1
    set2x2(12, 7, TOOL_IND_FACTORY,    2, 2); // T2 Heavy Machine Assembly v2
    set2x2(14, 7, TOOL_IND_TECH,       3, 1); // T3 Microchip Clean-Fab Hub v1
    set2x2(16, 7, TOOL_IND_TECH,       3, 2); // T3 Robotics Automation Facility v2

    // 5. Row 4 (y = 10): Shoreline Harbours & Water — ALL HARBOURS SIDE BY SIDE!
    for (int x = 3; x < tileMapW; x++) {
        for (int y = 12; y < tileMapH; y++) {
            delete entities[x][y];
            entities[x][y] = new WaterTile(x, y);
        }
    }
    set2x2(4,  10, TOOL_HARBOUR); // Trade Harbour 1
    set2x2(6,  10, TOOL_HARBOUR); // Trade Harbour 2
    set2x2(8,  10, TOOL_HARBOUR); // Trade Harbour 3
    set2x2(10, 10, TOOL_HARBOUR); // Trade Harbour 4
    set2x2(12, 10, TOOL_HARBOUR); // Trade Harbour 5
    set2x2(14, 10, TOOL_HARBOUR); // Trade Harbour 6

    // 8. Connect and autotile all roads
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            if (entities[i][j] != NULL && entities[i][j]->getName() == "DirtRoad") {
                DirtRoad* r = dynamic_cast<DirtRoad*>(entities[i][j]);
                if (r != NULL) r->check();
            }
        }
    }

    // Run initial happiness simulation pass to establish coverage
    happinessSystem.updateSimulation();
}
