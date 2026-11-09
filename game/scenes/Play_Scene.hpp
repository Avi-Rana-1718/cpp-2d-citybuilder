#pragma once
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <ctime>
#include <random>
#include "../../src/Scene.hpp"
#include "../../src/_global.hpp"

#include "../entities/Player.hpp"
#include "../entities/_Zone.hpp"
#include "../entities/ResidentalZone.hpp"
#include "../entities/CommericalZone.hpp"
#include "../entities/IndustrialZone.hpp"
#include "../entities/FarmZone.hpp"
#include "../entities/HarbourZone.hpp"
#include "../entities/SchoolZone.hpp"
#include "../entities/ChurchZone.hpp"
#include "../entities/ParkZone.hpp"
#include "../entities/SubZone.hpp"
#include "../entities/roads/DirtRoad.hpp"
#include "../world/WorldData.hpp"
#include "../world/WorldGen.hpp"
#include "../world/GroundRenderer.hpp"
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

    float lastActionTime = 0.f;
    float cameraSpeed = 800.f;
    float currentZoom = 1.0f;
    float simAccumulator = 0.f;

    // Drag-to-multibuild state
    bool isDragging = false;
    bool wasLeftPressed = false;
    sf::Vector2i dragStart{-1, -1};
    sf::Vector2i dragCurrent{-1, -1};

    // TheoTown Top Bar & Info Card State
    int gameSpeed = 1; // 0: Paused, 1: Normal, 2: Fast
    int lastDailyIncome = 45;

    // TheoTown Placement Validation & Notification
    mutable std::string lastPlacementError = "";
    float placementErrorTimer = 0.f;

    // World overlay: 0 None, 1 Fertility, 2 Groundwater, 3 Deposits
    int overlayMode = 0;
    bool wasOverlayKeyPressed = false;

public:
    Toolbar toolbar;
    HappinessSystem happinessSystem;
    GroundRenderer ground; // natural terrain, drawn from world[][] in one call

    ~Play_Scene();
    void init() override;
    void update(float deltaTime) override;

    bool canPlaceTile(int x, int y, BuildTool tool, std::string* outReason = nullptr) const;
    int getToolCost(BuildTool tool) const;
    int getBulldozeCost(int x, int y) const;
    std::string getOverlayName() const;
    int getOverlayMode() const { return overlayMode; }
    void setOverlayMode(int mode) { overlayMode = (mode >= 0 && mode < 4) ? mode : 0; }
    sf::FloatRect viewPanelBounds() const;
    sf::FloatRect viewButtonBounds(int mode) const;
    bool handleViewPanelClick(const sf::Vector2f& point);
    void placeTile(int x, int y, BuildTool tool);
    void refreshRoadsAround(int x, int y);
    void refreshBuildingsAround(int x, int y);
    int buildBenchCity();
    void runSimulationTick();

private:
    void handleCameraMovement(float deltaTime);
    void handleToolHotkeys();
    bool handleTileHover(const sf::Vector2i& mousePixel, bool mouseOverUI);
    void handleGestures(bool isHoveringTile, bool mouseOverUI);

    void renderWorld(float deltaTime);
    void renderOverlay(float viewL, float viewR, float viewT, float viewB);
    void renderViewPanel(const sf::Font& font, const sf::Vector2f& mouse);
    void renderDragPreview();
    void renderHUD(float deltaTime);

    void refreshUI();
    void populateCity();

    std::vector<sf::Vector2i> computeDragTiles() const;
    void executeMultibuild();
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
    money = 50000;
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

    // Procedural world: seed from --seed, or random when unset
    if (worldSeed == 0) {
        std::random_device rd;
        worldSeed = rd() ^ static_cast<uint32_t>(std::time(nullptr));
        if (worldSeed == 0) worldSeed = 1;
    }
    generateWorld(worldSeed);
    materializeWorld(); // no structures yet: the whole map is natural ground
    ground.markDirty();
    std::cout << "\nWorld generated with seed: " << worldSeed
              << " (spawn " << worldSpawnX << ", " << worldSpawnY << ")\n";
    if (benchCity) {
        int built = buildBenchCity();
        std::cout << "\nBench city: " << built << " buildings\n";
    }
    happinessSystem.updateSimulation();

    // Initialize TheoTown bottom dock
    toolbar.init(static_cast<float>(WINDOW_W), static_cast<float>(WINDOW_H));

    // Center camera on the guaranteed spawn area
    currentZoom = std::max(0.4f, std::min(3.2f, startZoom));
    view.setSize(sf::Vector2f(static_cast<float>(WINDOW_W) * currentZoom, static_cast<float>(WINDOW_H) * currentZoom));
    float spawnPx = static_cast<float>(tileSizeX * worldSpawnX - worldSpawnX * (tileSizeX / 2)) + tileSizeX / 2.f;
    float spawnPy = static_cast<float>(tileSizeY * worldSpawnY + worldSpawnX * (tileSizeY / 2)) + tileSizeY / 2.f;
    view.setCenter(sf::Vector2f(spawnPx, spawnPy));
    _window.setView(view);
}

void Play_Scene::update(float deltaTime) {
    this->_deltaTime = deltaTime;
    if (placementErrorTimer > 0.f) {
        placementErrorTimer -= deltaTime;
    }

    // 1. Process Input
    // sf::Keyboard / sf::Mouse report global device state, so typing or
    // clicking in another application would otherwise drive the game.
    const bool focused = _window.hasFocus();
    if (focused) {
        handleCameraMovement(deltaTime);
        handleToolHotkeys();
    }

    sf::Vector2i mousePixel = sf::Mouse::getPosition(_window);
    sf::Vector2f mousePixelF(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));

    // Check if mouse is interacting with UI elements
    bool overViewPanel = viewPanelBounds().contains(mousePixelF);
    bool mouseOverUI = !focused || toolbar.contains(mousePixelF) || overViewPanel || (mousePixelF.y <= 50.f);

    bool isLeftPressed = focused && sf::Mouse::isButtonPressed(sf::Mouse::Left);
    if (isLeftPressed && !wasLeftPressed && mouseOverUI) {
        if (overViewPanel) {
            handleViewPanelClick(mousePixelF);
        } else {
            toolbar.handleClick(mousePixelF);
        }
    }

    bool isHoveringTile = handleTileHover(mousePixel, mouseOverUI);
    if (focused) {
        handleGestures(isHoveringTile, mouseOverUI);
    }

    refreshUI();

    // 2. Render Passes
    _window.clear(sf::Color(15, 20, 25));
    renderWorld(deltaTime);
    renderHUD(deltaTime);
    _window.display();

    // 3. Economy & Simulation Tick
    simAccumulator += deltaTime;
    if (simAccumulator >= 1.0f) {
        runSimulationTick();
        simAccumulator = 0.f;
    }

    if (std::rand() % 1000 == 7 && static_cast<int>(gameTime.getElapsedTime().asSeconds()) % 7 == 0) {
        populateCity();
    }
}

void Play_Scene::handleCameraMovement(float deltaTime) {
    float curSpeed = cameraSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
        curSpeed *= 2.2f;
    }

    sf::Vector2f camMove(0.f, 0.f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
        camMove.y -= curSpeed * deltaTime;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        camMove.y += curSpeed * deltaTime;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        camMove.x -= curSpeed * deltaTime;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        camMove.x += curSpeed * deltaTime;
    }

    if (camMove.x != 0.f || camMove.y != 0.f) {
        view.move(camMove);
    }

    // Zoom controls: Q / PageUp / Equal to zoom in, E / PageDown / Dash to zoom out
    bool zoomChanged = false;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q) || sf::Keyboard::isKeyPressed(sf::Keyboard::PageUp) || sf::Keyboard::isKeyPressed(sf::Keyboard::Equal)) {
        currentZoom = std::max(0.4f, currentZoom - 0.9f * deltaTime);
        zoomChanged = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::E) || sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown) || sf::Keyboard::isKeyPressed(sf::Keyboard::Dash)) {
        currentZoom = std::min(3.2f, currentZoom + 0.9f * deltaTime);
        zoomChanged = true;
    }

    if (zoomChanged) {
        view.setSize(static_cast<float>(WINDOW_W) * currentZoom, static_cast<float>(WINDOW_H) * currentZoom);
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
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && !sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) && !sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
        // Only trigger water tool if shift is not pressed (to allow WASD fast pan)
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
    // O cycles the map view (edge-triggered)
    bool overlayKey = sf::Keyboard::isKeyPressed(sf::Keyboard::O);
    if (overlayKey && !wasOverlayKeyPressed) {
        overlayMode = (overlayMode + 1) % 4;
    }
    wasOverlayKeyPressed = overlayKey;
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

    // Fast O(1) inverse isometric coordinate estimate
    int estI = static_cast<int>(std::round((mouseWorld.x - 32.f) / 32.f));
    int estJ = static_cast<int>(std::round((mouseWorld.y - 16.f * estI - 16.f) / 32.f));

    int minI = std::max(0, estI - 2);
    int maxI = std::min(tileMapW - 1, estI + 2);
    int minJ = std::max(0, estJ - 2);
    int maxJ = std::min(tileMapH - 1, estJ + 2);

    for (int i = minI; i <= maxI; i++) {
        for (int j = minJ; j <= maxJ; j++) {
            sf::Vector2f tp = tileScreenPos(i, j);
            float px = tp.x;
            float py = tp.y;

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
            sf::Vector2f tp = tileScreenPos(tileX, tileY);
            player->position->x = tp.x;
            player->position->y = tp.y;
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
        int dx = dragCurrent.x - dragStart.x;
        int dy = dragCurrent.y - dragStart.y;

        // Check if dragging along the edge-sharing Axis 2 (SW/NE: dx == -dy)
        if (dx != 0 && dy != 0 && (dx == -dy || std::abs(dx + dy) <= std::max(1, std::min(std::abs(dx), std::abs(dy)) / 3))) {
            int steps = std::max(std::abs(dx), std::abs(dy));
            int stepX = (dx > 0) ? 1 : -1;
            int stepY = (dy > 0) ? 1 : -1;
            if (stepX == stepY) stepY = -stepX;

            int cx = dragStart.x;
            int cy = dragStart.y;
            for (int s = 0; s <= steps; s++) {
                tiles.push_back({cx, cy});
                if (cx == dragCurrent.x && cy == dragCurrent.y) break;
                cx += stepX;
                cy += stepY;
            }
        } else {
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

int Play_Scene::getToolCost(BuildTool tool) const {
    switch (tool) {
        case TOOL_RES_COTTAGES:   return 100;
        case TOOL_RES_APARTMENTS: return 250;
        case TOOL_RES_TOWER:      return 600;
        case TOOL_COM_GROCERY:    return 120;
        case TOOL_COM_PLAZA:      return 280;
        case TOOL_COM_TOWER:      return 750;
        case TOOL_IND_WORKSHOP:   return 150;
        case TOOL_IND_FACTORY:    return 350;
        case TOOL_IND_TECH:       return 800;
        case TOOL_FARM_BARN:      return 70;
        case TOOL_FARM_WINDMILL:  return 90;
        case TOOL_ROAD:           return 10;
        case TOOL_HARBOUR:        return 250;
        case TOOL_WATER:          return 20;
        case TOOL_SCHOOL:         return 200;
        case TOOL_CHURCH:         return 180;
        case TOOL_PARK:           return 80;
        case TOOL_BULLDOZE:       return 5;
        default: return 0;
    }
}

int Play_Scene::getBulldozeCost(int x, int y) const {
    const std::string kind = tileKind(x, y);
    if (kind == "Forest") return 15;
    if (kind == "Rock") return 60;
    return getToolCost(TOOL_BULLDOZE);
}

std::string Play_Scene::getOverlayName() const {
    switch (overlayMode) {
        case 1: return "Fertility";
        case 2: return "Groundwater";
        case 3: return "Deposits";
        default: return "Normal";
    }
}

// Map View panel: fixed screen-space buttons under the top bar (left side)
sf::FloatRect Play_Scene::viewPanelBounds() const {
    return sf::FloatRect(12.f, 60.f, 180.f, 196.f);
}

sf::FloatRect Play_Scene::viewButtonBounds(int mode) const {
    sf::FloatRect panel = viewPanelBounds();
    return sf::FloatRect(panel.left + 10.f, panel.top + 30.f + static_cast<float>(mode) * 30.f, panel.width - 20.f, 26.f);
}

bool Play_Scene::handleViewPanelClick(const sf::Vector2f& point) {
    for (int m = 0; m < 4; m++) {
        if (viewButtonBounds(m).contains(point)) {
            setOverlayMode(m);
            return true;
        }
    }
    return false;
}

bool Play_Scene::canPlaceTile(int x, int y, BuildTool tool, std::string* outReason) const {
    if (tool == TOOL_NONE) return false;

    int cost = (tool == TOOL_BULLDOZE) ? getBulldozeCost(x, y) : getToolCost(tool);
    if (money < cost) {
        if (outReason) *outReason = "Insufficient Funds ($" + std::to_string(cost) + " needed)";
        return false;
    }

    // Natural obstacles that must be bulldozed before building
    auto terrainBlockReason = [](const std::string& name) -> std::string {
        if (name == "Forest") return "Forest - Clear First (Bulldoze)";
        if (name == "Rock") return "Rocky Terrain - Clear First (Bulldoze)";
        return "";
    };

    // 1. Bulldoze Tool
    if (tool == TOOL_BULLDOZE) {
        if (x < 0 || x >= tileMapW || y < 0 || y >= tileMapH) return false;
        if (isClearGroundName(tileKind(x, y))) {
            if (outReason) *outReason = "Already Clear Ground";
            return false;
        }
        return true;
    }

    // 2. Road Tool (1x1)
    if (tool == TOOL_ROAD) {
        if (x < 0 || x >= tileMapW || y < 0 || y >= tileMapH) return false;
        std::string name = tileKind(x, y);
        if (name == "DirtRoad") {
            if (outReason) *outReason = "Road Already Built";
            return false;
        }
        if (name == "Water") {
            if (outReason) *outReason = "Cannot Build Road on Water";
            return false;
        }
        if (!terrainBlockReason(name).empty()) {
            if (outReason) *outReason = terrainBlockReason(name);
            return false;
        }
        if (!isClearGroundName(name)) {
            if (outReason) *outReason = "Blocked by Structure — Bulldoze First";
            return false;
        }
        return true;
    }

    // 3. Water Tool (1x1)
    if (tool == TOOL_WATER) {
        if (x < 0 || x >= tileMapW || y < 0 || y >= tileMapH) return false;
        std::string name = tileKind(x, y);
        if (name == "Water") {
            if (outReason) *outReason = "Already Water";
            return false;
        }
        if (!terrainBlockReason(name).empty()) {
            if (outReason) *outReason = terrainBlockReason(name);
            return false;
        }
        if (!isClearGroundName(name)) {
            if (outReason) *outReason = "Area Occupied — Bulldoze First";
            return false;
        }
        return true;
    }

    // 4. 2x2 Structures (Residential, Commercial, Industrial, Farm, School, Church, Park, Harbour)
    // True 2x2 Isometric Diamond: (x, y), (x + 1, y), (x - 1, y + 1), (x, y + 1)
    if (x < 1 || x + 1 >= tileMapW || y < 0 || y + 1 >= tileMapH) {
        if (outReason) *outReason = "Out of Map Bounds";
        return false;
    }

    const sf::Vector2i diamondOffsets[4] = {{0, 0}, {1, 0}, {-1, 1}, {0, 1}};

    // Check all 4 footprint tiles for obstacles
    for (int k = 0; k < 4; k++) {
        int cx = x + diamondOffsets[k].x;
        int cy = y + diamondOffsets[k].y;
        if (cx < 0 || cx >= tileMapW || cy < 0 || cy >= tileMapH) return false;
        std::string name = tileKind(cx, cy);
        if (name == "DirtRoad") {
            if (outReason) *outReason = "Blocked by Road!";
            return false;
        }
        if (name == "Water") {
            if (outReason) *outReason = "Blocked by Water!";
            return false;
        }
        if (!terrainBlockReason(name).empty()) {
            if (outReason) *outReason = terrainBlockReason(name);
            return false;
        }
        if (!isClearGroundName(name)) {
            if (outReason) *outReason = "Lot Occupied — Bulldoze First!";
            return false;
        }
    }

    // Access: road / water on a tile sharing an edge with the footprint
    bool touchesRoad = hasAdjacentRoad(x, y);
    bool touchesWater = hasAdjacentWater(x, y);

    if (tool == TOOL_HARBOUR) {
        if (!touchesWater) {
            if (outReason) *outReason = "Harbour Must Touch Shoreline Water!";
            return false;
        }
        if (!touchesRoad) {
            if (outReason) *outReason = "Harbour Requires Road Connection!";
            return false;
        }
        return true;
    }

    // All other 2x2 buildings require road connection
    if (!touchesRoad) {
        if (outReason) *outReason = "Requires Road Connection!";
        return false;
    }

    return true;
}

void Play_Scene::placeTile(int x, int y, BuildTool tool) {
    if (x < 0 || x >= tileMapW || y < 0 || y >= tileMapH) return;
    if (tool == TOOL_NONE) return;

    std::string reason = "";
    if (!canPlaceTile(x, y, tool, &reason)) {
        lastPlacementError = reason;
        placementErrorTimer = 2.5f;
        return;
    }

    int cost = (tool == TOOL_BULLDOZE) ? getBulldozeCost(x, y) : getToolCost(tool);

    auto demolishAt = [&](int tx, int ty) {
        if (tx < 0 || tx >= tileMapW || ty < 0 || ty >= tileMapH) return;
        if (entities[tx][ty] == NULL) {
            // Clearing natural ground (forest, rock, water) turns it into grassland
            if (isNaturalObstacleName(tileKind(tx, ty))) {
                world[tx][ty].terrain = Terrain::Grass;
                ground.markDirty();
            }
            return;
        }
        Zone* z = dynamic_cast<Zone*>(entities[tx][ty]);
        if (z != NULL && z->isMultiTile) {
            int rx = z->isSubTile ? z->rootX : tx;
            int ry = z->isSubTile ? z->rootY : ty;
            const sf::Vector2i dOffsets[4] = {{0, 0}, {1, 0}, {-1, 1}, {0, 1}};
            for (int k = 0; k < 4; k++) {
                int cx = rx + dOffsets[k].x;
                int cy = ry + dOffsets[k].y;
                if (cx >= 0 && cx < tileMapW && cy >= 0 && cy < tileMapH) {
                    delete entities[cx][cy];
                    entities[cx][cy] = NULL;
                }
            }
        } else {
            // Structure removed: the natural ground underneath shows again
            delete entities[tx][ty];
            entities[tx][ty] = NULL;
        }
    };

    if (tool == TOOL_BULLDOZE) {
        bool wasRoad = (tileKind(x, y) == "DirtRoad");
        demolishAt(x, y);
        if (wasRoad) refreshRoadsAround(x, y);
        refreshBuildingsAround(x, y); // a removed road or water tile can disconnect neighbours
        money -= cost;
        return;
    }

    if (tool == TOOL_ROAD) {
        delete entities[x][y];
        entities[x][y] = new DirtRoad(x, y);
        refreshRoadsAround(x, y); // the new road and every road touching it
        refreshBuildingsAround(x, y);
        money -= cost;
        return;
    }

    if (tool == TOOL_WATER) {
        delete entities[x][y];
        entities[x][y] = NULL;
        world[x][y].terrain = Terrain::Water;
        ground.markDirty();
        refreshBuildingsAround(x, y); // harbours need water on their border
        money -= cost;
        return;
    }

    // 2x2 Structures: clear the 4 grass tiles (True 2x2 Diamond)
    const sf::Vector2i dOffsets[4] = {{0, 0}, {1, 0}, {-1, 1}, {0, 1}};
    for (int k = 0; k < 4; k++) {
        int cx = x + dOffsets[k].x;
        int cy = y + dOffsets[k].y;
        delete entities[cx][cy];
        entities[cx][cy] = NULL;
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
    entities[x - 1][y + 1] = new SubZone(x - 1, y + 1, x, y, typeName, zoneBase);
    entities[x][y + 1]     = new SubZone(x,     y + 1, x, y, typeName, zoneBase);
    masterZone->checkValid();
}

void Play_Scene::executeMultibuild() {
    BuildTool tool = toolbar.getActiveTool();
    if (tool == TOOL_NONE) return;

    if (tool == TOOL_ROAD || tool == TOOL_WATER || tool == TOOL_BULLDOZE) {
        auto tiles = computeDragTiles();
        for (const auto& pt : tiles) {
            placeTile(pt.x, pt.y, tool);
        }
    } else {
        int gx = isDragging ? dragCurrent.x : tileX;
        int gy = isDragging ? dragCurrent.y : tileY;
        placeTile(gx, gy, tool);
    }
}

// Once per second: every structure's slow step (taxes, tiers, sub-tile
// state), then citizen happiness, which reads the results.
void Play_Scene::runSimulationTick() {
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            if (entities[i][j] == NULL) continue;
            Zone* z = dynamic_cast<Zone*>(entities[i][j]);
            if (z != NULL) z->simTick();
        }
    }
    happinessSystem.updateSimulation();
}

// Fills the map with a dense grid city for benchmarking (--bench-city).
// Road rows run east-west every 3 rows; each 2x2 building sits on the row
// below a road so the road shares its north-east edge. Natural obstacles
// in the way are cleared first. Returns the number of buildings placed.
int Play_Scene::buildBenchCity() {
    const int savedMoney = money;
    money = 100000000;

    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            if (isNaturalObstacleName(tileKind(i, j))) world[i][j].terrain = Terrain::Grass;
        }
    }
    ground.markDirty();

    for (int y = 1; y + 2 < tileMapH; y += 3) {
        for (int x = 0; x < tileMapW; x++) placeTile(x, y, TOOL_ROAD);
    }

    const BuildTool types[6] = {TOOL_RES_COTTAGES, TOOL_COM_GROCERY, TOOL_IND_WORKSHOP,
                                TOOL_RES_APARTMENTS, TOOL_FARM_BARN, TOOL_SCHOOL};
    int built = 0;
    int k = 0;
    for (int y = 1; y + 2 < tileMapH; y += 3) {
        for (int x = 1; x + 1 < tileMapW; x += 2) {
            BuildTool t = types[k++ % 6];
            if (canPlaceTile(x, y + 1, t)) {
                placeTile(x, y + 1, t);
                built++;
            }
        }
    }

    money = savedMoney;
    return built;
}

// Re-checks road access for every 2x2 building whose border could include
// (x, y). Border tiles sit at most 2 tiles from the building's root.
void Play_Scene::refreshBuildingsAround(int x, int y) {
    for (int i = x - 2; i <= x + 2; i++) {
        for (int j = y - 2; j <= y + 2; j++) {
            if (!inWorldBounds(i, j) || entities[i][j] == NULL) continue;
            Zone* z = dynamic_cast<Zone*>(entities[i][j]);
            if (z != NULL && z->isMultiTile && !z->isSubTile) z->checkValid();
        }
    }
}

// Re-runs road autotiling for (x, y) and its 8 neighbours, which covers
// every direction DirtRoad::check() looks at.
void Play_Scene::refreshRoadsAround(int x, int y) {
    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
            if (!inWorldBounds(i, j) || entities[i][j] == NULL) continue;
            DirtRoad* road = dynamic_cast<DirtRoad*>(entities[i][j]);
            if (road != NULL) road->check();
        }
    }
}

void Play_Scene::renderWorld(float deltaTime) {
    _window.setView(view);

    // Background
    for (size_t i = 0; i < bg.size(); i++) {
        if (bg[i] != NULL) bg[i]->update(deltaTime);
    }

    // No per-frame entity update: map entities have no per-frame logic.
    // Their simulation runs in runSimulationTick() once per second.

    // Viewport frustum culling bounds
    sf::Vector2f vCenter = view.getCenter();
    sf::Vector2f vSize = view.getSize();
    float viewL = vCenter.x - vSize.x / 2.f - 64.f;
    float viewR = vCenter.x + vSize.x / 2.f + 64.f;
    float viewT = vCenter.y - vSize.y / 2.f - 200.f; // Skyscraper height margin
    float viewB = vCenter.y + vSize.y / 2.f + 64.f;

    const float tallMargin = 140.f; // buildings and trees rise above their tile
    auto isVisible = [&](float px, float py) -> bool {
        return (px + tileSizeX >= viewL && px <= viewR &&
                py + tileSizeY + tallMargin >= viewT && py <= viewB);
    };

    // Only tiles inside the camera's band are visited by the draw passes
    const VisibleTiles vis = visibleTileRange(viewL, viewR, viewT, viewB, tallMargin);

    // PASS 1a: Natural ground for the whole map in a single draw call
    ground.draw(_window);

    // PASS 1b: Ground layer of structures (roads, zone bases)
    for (int i = vis.iMin; i <= vis.iMax; i++) {
        for (int j = vis.jMin(i); j <= vis.jMax(i); j++) {
            if (entities[i][j] != NULL && entities[i][j]->position != NULL) {
                float px = entities[i][j]->position->x;
                float py = entities[i][j]->position->y;
                if (!isVisible(px, py)) continue;

                Zone* z = dynamic_cast<Zone*>(entities[i][j]);
                if (z != NULL) {
                    z->drawGround(_window);
                } else {
                    entities[i][j]->draw();
                }
            }
        }
    }

    // PASS 1.5: Resource / soil overlay
    if (overlayMode != 0) {
        renderOverlay(viewL, viewR, viewT, viewB);
    }

    // PASS 2: Render 3D Isometric Buildings in Isometric Depth Order (i + j),
    // walking only the depths and tiles inside the visible band
    const int jLo = vis.jLowest();
    const int jHi = vis.jHighest();
    for (int depth = vis.iMin + jLo; depth <= vis.iMax + jHi; depth++) {
        int iFrom = std::max(vis.iMin, depth - jHi);
        int iTo = std::min(vis.iMax, depth - jLo);
        for (int i = iFrom; i <= iTo; i++) {
            int j = depth - i;
            if (j >= vis.jMin(i) && j <= vis.jMax(i)) {
                if (entities[i][j] != NULL && entities[i][j]->position != NULL) {
                    Zone* z = dynamic_cast<Zone*>(entities[i][j]);
                    if (z != NULL && !z->isSubTile) {
                        float px = z->position->x;
                        float py = z->position->y;
                        if (isVisible(px, py)) {
                            z->drawBuilding(_window);
                        }
                    }
                } else if (entities[i][j] == NULL) {
                    Terrain t = world[i][j].terrain;
                    if (t == Terrain::Forest || t == Terrain::Rock) {
                        sf::Vector2f tp = tileScreenPos(i, j);
                        if (isVisible(tp.x, tp.y)) ground.drawTall(_window, i, j);
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

void Play_Scene::renderOverlay(float viewL, float viewR, float viewT, float viewB) {
    sf::ConvexShape diamond(4);
    diamond.setPoint(0, sf::Vector2f(32.f, 0.f));
    diamond.setPoint(1, sf::Vector2f(64.f, 16.f));
    diamond.setPoint(2, sf::Vector2f(32.f, 32.f));
    diamond.setPoint(3, sf::Vector2f(0.f, 16.f));

    const VisibleTiles vis = visibleTileRange(viewL, viewR, viewT, viewB, 0.f);
    for (int i = vis.iMin; i <= vis.iMax; i++) {
        for (int j = vis.jMin(i); j <= vis.jMax(i); j++) {
            sf::Vector2f tp = tileScreenPos(i, j);
            float px = tp.x;
            float py = tp.y;
            if (px + tileSizeX < viewL || px > viewR || py + tileSizeY < viewT || py > viewB) continue;

            const TileData& td = world[i][j];
            if (td.terrain == Terrain::Water) continue;

            sf::Color col(0, 0, 0, 0);
            if (overlayMode == 1) {
                // Fertility: red (barren) -> yellow -> green (rich)
                float f = static_cast<float>(td.fertility) / 100.f;
                sf::Uint8 r = static_cast<sf::Uint8>(f < 0.5f ? 210 : 210 - (f - 0.5f) * 2.f * 170.f);
                sf::Uint8 g = static_cast<sf::Uint8>(f < 0.5f ? 60 + f * 2.f * 150.f : 210);
                col = sf::Color(r, g, 50, 120);
            } else if (overlayMode == 2) {
                // Groundwater: deeper blue = more water underground
                sf::Uint8 a = static_cast<sf::Uint8>(30 + td.groundwater * 1.7f);
                col = sf::Color(40, 130, 230, a);
            } else if (overlayMode == 3) {
                if (td.deposit == Deposit::None) {
                    col = sf::Color(10, 12, 18, 90); // dim empty land so deposits stand out
                } else {
                    sf::Uint8 a = static_cast<sf::Uint8>(90 + td.depositAmount * 1.5f);
                    if (td.deposit == Deposit::Coal) col = sf::Color(25, 25, 30, a);
                    else if (td.deposit == Deposit::Iron) col = sf::Color(200, 85, 30, a);
                    else col = sf::Color(215, 215, 220, a);
                }
            }

            diamond.setPosition(px, py);
            diamond.setFillColor(col);
            _window.draw(diamond);
        }
    }
}

void Play_Scene::renderViewPanel(const sf::Font& font, const sf::Vector2f& mouse) {
    sf::FloatRect panel = viewPanelBounds();
    sf::RectangleShape bgRect(sf::Vector2f(panel.width, panel.height));
    bgRect.setPosition(panel.left, panel.top);
    bgRect.setFillColor(sf::Color(16, 22, 30, 230));
    bgRect.setOutlineColor(sf::Color(45, 60, 80, 220));
    bgRect.setOutlineThickness(1.5f);
    _window.draw(bgRect);

    sf::Text label;
    label.setFont(font);
    label.setCharacterSize(12);
    label.setStyle(sf::Text::Bold);
    label.setFillColor(sf::Color(180, 195, 210));
    label.setString("MAP VIEW [O]");
    label.setPosition(panel.left + 10.f, panel.top + 8.f);
    _window.draw(label);

    const char* names[4] = {"Normal", "Fertility", "Groundwater", "Deposits"};
    const sf::Color accents[4] = {
        sf::Color(149, 165, 166), sf::Color(120, 200, 70), sf::Color(52, 152, 219), sf::Color(220, 120, 50)
    };

    label.setStyle(sf::Text::Regular);
    for (int m = 0; m < 4; m++) {
        sf::FloatRect b = viewButtonBounds(m);
        bool active = (overlayMode == m);
        bool hovered = b.contains(mouse);

        sf::RectangleShape btn(sf::Vector2f(b.width, b.height));
        btn.setPosition(b.left, b.top);
        if (active) {
            btn.setFillColor(sf::Color(accents[m].r / 4, accents[m].g / 4, accents[m].b / 4, 255));
            btn.setOutlineColor(accents[m]);
            btn.setOutlineThickness(2.f);
        } else {
            btn.setFillColor(hovered ? sf::Color(38, 48, 62, 240) : sf::Color(25, 31, 40, 215));
            btn.setOutlineColor(sf::Color(50, 60, 75, 180));
            btn.setOutlineThickness(1.f);
        }
        _window.draw(btn);

        sf::RectangleShape swatch(sf::Vector2f(4.f, b.height - 8.f));
        swatch.setPosition(b.left + 5.f, b.top + 4.f);
        swatch.setFillColor(accents[m]);
        _window.draw(swatch);

        label.setCharacterSize(12);
        label.setFillColor(active ? sf::Color::White : sf::Color(190, 200, 210));
        label.setString(names[m]);
        label.setPosition(b.left + 16.f, b.top + 5.f);
        _window.draw(label);
    }

    // Legend for the active view
    float ly = viewButtonBounds(3).top + 34.f;
    label.setCharacterSize(10);
    label.setFillColor(sf::Color(150, 165, 180));
    auto drawKey = [&](float x, const sf::Color& c, const std::string& text) {
        sf::RectangleShape key(sf::Vector2f(10.f, 10.f));
        key.setPosition(x, ly + 2.f);
        key.setFillColor(c);
        key.setOutlineColor(sf::Color(90, 100, 115));
        key.setOutlineThickness(1.f);
        _window.draw(key);
        label.setString(text);
        label.setPosition(x + 14.f, ly);
        _window.draw(label);
    };
    if (overlayMode == 0) {
        label.setString("Terrain only");
        label.setPosition(panel.left + 10.f, ly);
        _window.draw(label);
    } else if (overlayMode == 1) {
        drawKey(panel.left + 10.f, sf::Color(210, 60, 50), "Barren");
        drawKey(panel.left + 92.f, sf::Color(40, 210, 50), "Rich");
    } else if (overlayMode == 2) {
        drawKey(panel.left + 10.f, sf::Color(40, 130, 230, 80), "Low");
        drawKey(panel.left + 92.f, sf::Color(40, 130, 230, 230), "High");
    } else {
        drawKey(panel.left + 10.f, sf::Color(25, 25, 30), "Coal");
        drawKey(panel.left + 64.f, sf::Color(200, 85, 30), "Iron");
        drawKey(panel.left + 116.f, sf::Color(215, 215, 220), "Stone");
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
        diamond1x1.setPosition(tileScreenPos(tx, ty));
        diamond1x1.setFillColor(fill);
        diamond1x1.setOutlineColor(outline);
        diamond1x1.setOutlineThickness(1.5f);
        _window.draw(diamond1x1);
    };

    if (tool == TOOL_ROAD || tool == TOOL_WATER) {
        std::vector<sf::Vector2i> previewTiles;
        if (isDragging) {
            previewTiles = computeDragTiles();
        } else if (tileX >= 0 && tileX < tileMapW && tileY >= 0 && tileY < tileMapH) {
            previewTiles.push_back({tileX, tileY});
        }

        for (const auto& pt : previewTiles) {
            int gx = pt.x, gy = pt.y;
            if (gx < 0 || gx >= tileMapW || gy < 0 || gy >= tileMapH) continue;
            sf::Vector2f tp = tileScreenPos(gx, gy);
            float px = tp.x;
            float py = tp.y;

            std::string reason = "";
            bool ok = canPlaceTile(gx, gy, tool, &reason);
            sf::Color fillCol = ok ? sf::Color(46, 204, 113, 85) : sf::Color(231, 76, 60, 110);
            sf::Color outCol = ok ? sf::Color(46, 204, 113, 230) : sf::Color(231, 76, 60, 245);
            highlightTile(gx, gy, fillCol, outCol);

            std::string tkey = (tool == TOOL_ROAD) ? "road" : "water";
            if (m_textures.count(tkey)) {
                ghostSprite.setTexture(m_textures[tkey]);
                ghostSprite.setOrigin(0.f, 0.f);
                ghostSprite.setScale(2.f, 2.f);
                ghostSprite.setColor(ok ? sf::Color(255, 255, 255, 195) : sf::Color(255, 120, 120, 160));
                ghostSprite.setPosition(px, py);
                _window.draw(ghostSprite);
            }
        }
        return;
    }

    if (tool == TOOL_BULLDOZE) {
        std::vector<sf::Vector2i> previewTiles;
        if (isDragging) {
            previewTiles = computeDragTiles();
        } else if (tileX >= 0 && tileX < tileMapW && tileY >= 0 && tileY < tileMapH) {
            previewTiles.push_back({tileX, tileY});
        }

        for (const auto& pt : previewTiles) {
            int gx = pt.x, gy = pt.y;
            if (gx < 0 || gx >= tileMapW || gy < 0 || gy >= tileMapH) continue;
            {
                Zone* z = entities[gx][gy] ? dynamic_cast<Zone*>(entities[gx][gy]) : NULL;
                if (z != NULL && z->isMultiTile) {
                    int rx = z->isSubTile ? z->rootX : gx;
                    int ry = z->isSubTile ? z->rootY : gy;
                    const sf::Vector2i dOffsets[4] = {{0, 0}, {1, 0}, {-1, 1}, {0, 1}};
                    for (int k = 0; k < 4; k++) {
                        highlightTile(rx + dOffsets[k].x, ry + dOffsets[k].y, sf::Color(231, 76, 60, 130), sf::Color(231, 76, 60, 245));
                    }
                    continue;
                }
                bool canBulldoze = !isClearGroundName(tileKind(gx, gy));
                sf::Color fillCol = canBulldoze ? sf::Color(231, 76, 60, 130) : sf::Color(120, 120, 120, 60);
                sf::Color outCol = canBulldoze ? sf::Color(231, 76, 60, 245) : sf::Color(140, 140, 140, 150);
                highlightTile(gx, gy, fillCol, outCol);
            }
        }
        return;
    }

    // 2x2 Building Tools: Show individual 2:1 isometric diamond highlights and ghost building (True 2x2 Diamond)
    int gx = isDragging ? dragCurrent.x : tileX;
    int gy = isDragging ? dragCurrent.y : tileY;

    if (gx >= 1 && gx + 1 < tileMapW && gy >= 0 && gy + 1 < tileMapH) {
        {
            sf::Vector2f tp = tileScreenPos(gx, gy);
            float px = tp.x;
            float py = tp.y;

            std::string reason = "";
            bool ok = canPlaceTile(gx, gy, tool, &reason);
            sf::Color fillCol = ok ? sf::Color(46, 204, 113, 85) : sf::Color(231, 76, 60, 110);
            sf::Color outCol = ok ? sf::Color(46, 204, 113, 230) : sf::Color(231, 76, 60, 245);

            // Highlight all 4 constituent 2:1 isometric tile diamonds (True 2x2 Diamond)
            const sf::Vector2i dOffsets[4] = {{0, 0}, {1, 0}, {-1, 1}, {0, 1}};
            for (int k = 0; k < 4; k++) {
                highlightTile(gx + dOffsets[k].x, gy + dOffsets[k].y, fillCol, outCol);
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
                ghostSprite.setColor(ok ? sf::Color(255, 255, 255, 195) : sf::Color(255, 120, 120, 150));
                ghostSprite.setPosition(px - 16.f, py);
                _window.draw(ghostSprite);
            }
        }
    }
}

void Play_Scene::renderHUD(float deltaTime) {
    _window.setView(_window.getDefaultView());

    const sf::Font& font = m_fonts["../assets/Hack.ttf"];

    // -------------------------------------------------------------
    // 0. Placement Error Banner
    // -------------------------------------------------------------
    if (placementErrorTimer > 0.f && !lastPlacementError.empty()) {
        float bannerW = 460.f;
        float bannerH = 34.f;
        float bannerX = (static_cast<float>(WINDOW_W) - bannerW) / 2.f;
        float bannerY = 56.f;

        sf::RectangleShape banner(sf::Vector2f(bannerW, bannerH));
        banner.setPosition(bannerX, bannerY);
        banner.setFillColor(sf::Color(38, 14, 18, 235));
        banner.setOutlineColor(sf::Color(231, 76, 60));
        banner.setOutlineThickness(1.5f);
        _window.draw(banner);

        sf::Text alertText;
        alertText.setFont(font);
        alertText.setCharacterSize(13);
        alertText.setFillColor(sf::Color(255, 190, 190));
        alertText.setString("[!] Cannot Build: " + lastPlacementError);
        alertText.setPosition(bannerX + 16.f, bannerY + 8.f);
        _window.draw(alertText);
    }

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

    // World overlay & seed
    sf::Text overlayText;
    overlayText.setFont(font);
    overlayText.setCharacterSize(12);
    overlayText.setFillColor(overlayMode == 0 ? sf::Color(140, 155, 170) : sf::Color(26, 188, 156));
    overlayText.setString("View [O]: " + getOverlayName());
    overlayText.setPosition(930.f, 7.f);
    _window.draw(overlayText);

    sf::Text seedText;
    seedText.setFont(font);
    seedText.setCharacterSize(11);
    seedText.setFillColor(sf::Color(120, 135, 150));
    seedText.setString("Seed: " + std::to_string(worldSeed));
    seedText.setPosition(930.f, 26.f);
    _window.draw(seedText);

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
        {
            bool natural = (entities[tileX][tileY] == NULL);
            Zone* z = natural ? NULL : dynamic_cast<Zone*>(entities[tileX][tileY]);
            Zone* mz = z;
            if (z != NULL && z->isSubTile && z->rootX >= 0 && z->rootY >= 0) {
                mz = dynamic_cast<Zone*>(entities[z->rootX][z->rootY]);
            }

            if (mz != NULL || natural) {
                float cardW = 360.f;
                float cardH = 240.f;
                float cardX = static_cast<float>(WINDOW_W) - cardW - 20.f;
                float cardY = 60.f;

                sf::RectangleShape card(sf::Vector2f(cardW, cardH));
                card.setPosition(cardX, cardY);
                card.setFillColor(sf::Color(16, 22, 30, 235));
                card.setOutlineColor(sf::Color(52, 152, 219, 200));
                card.setOutlineThickness(2.f);
                _window.draw(card);

                std::string headerTitle;
                std::string subTitle;
                std::string tierDesc;

                if (natural) {
                    headerTitle = isClearGroundName(tileKind(tileX, tileY)) ? "OPEN LAND" : "NATURAL FEATURE";
                    subTitle = "Natural Terrain";
                } else {
                    headerTitle = mz->getName();
                    subTitle = mz->isMultiTile ? "2x2 Building Complex" : "1x1 Structure";
                    tierDesc = "Tier " + std::to_string(mz->tier);
                }

                if (natural) {
                    // no building details
                } else if (mz->getName() == "ResidentalZone") {
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
                    FarmZone* fz = dynamic_cast<FarmZone*>(mz);
                    char mulBuf[16];
                    std::snprintf(mulBuf, sizeof(mulBuf), "%.2f", fz->fertilityMul);
                    tierDesc = fz->getVariantName() + " * Soil x" + mulBuf;
                }

                const TileData& land = world[tileX][tileY];
                std::string landLine = "Land:        " + terrainName(land.terrain) + " | Soil: " + std::to_string(land.fertility) + "%\n";
                landLine += "Underground: Water " + std::to_string(land.groundwater) + "% | " + depositName(land.deposit);
                if (land.deposit != Deposit::None) landLine += " (" + std::to_string(land.depositAmount) + "%)";

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

                std::string infoBody = natural ? landLine :
                    "Designation: " + tierDesc + "\n"
                    "Footprint:   " + (mz->isMultiTile ? "2x2 Tiles" : "1x1 Tile") + "\n"
                    "Capacity:    " + std::to_string(mz->capacity) + " | Occupants: " + std::to_string(mz->population) + "\n"
                    "Tax Yield:   $" + std::to_string(mz->zoneIncome) + " / cycle\n"
                    "Road Status: " + (mz->isActive ? "Connected [Active]" : "No Road Access") + "\n"
                    "Local Happy: " + std::to_string(mz->overallHappiness) + "%\n"
                    "Services:    Edu: " + std::to_string(townStats[2]) + "% | Health: " + std::to_string(townStats[4]) + "%\n" +
                    landLine;

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
    renderViewPanel(font, mousePixelF);
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
