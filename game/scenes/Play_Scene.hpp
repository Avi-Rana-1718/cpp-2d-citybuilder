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
#include "../simulation/GameClock.hpp"
#include "../simulation/Economy.hpp"
#include "../ui/Window.hpp"
#include "../ui/UiKit.hpp"
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
    int lastDailyIncome = 0;   // tax credited on the most recent game day
    // Budget panel and economy UI state
    bool budgetOpen = false;
    int activeSlider = -1;           // sector * 3 + wealth while dragging a tax slider
    int hoveredBucket = -1;          // budget row under the mouse (for the demand hint)
    bool budgetMouseWasDown = false;
    bool wasBudgetKey = false;
    bool wasNewCityKey = false;
    bool toastIsError = true;

    // Edge-triggered time hotkeys
    bool wasPauseKey = false;
    bool wasSlowerKey = false;
    bool wasFasterKey = false;

    // TheoTown Placement Validation & Notification
    mutable std::string lastPlacementError = "";
    float placementErrorTimer = 0.f;
    sf::FloatRect inspectCardRect; // last drawn inspect card (empty when hidden)

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
    void onResize(unsigned width, unsigned height) override;

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
    int buildBenchTown();
    void runSimulationTick();

    // ---- Time ----
    // Advances game time by real seconds (scaled by speed): new days pay
    // taxes and grow population, simulation ticks run per game second.
    void advanceTime(float realSeconds);
    void runNewDay();
    int getLastDailyIncome() const { return lastDailyIncome; }

    // ---- Economy ----
    EconomySystem economy;
    void startNewCity(uint32_t seed);
    void showToast(const std::string& msg, bool isError);

    // Budget panel (open with B or by clicking the money chip)
    bool isBudgetOpen() const { return budgetOpen; }
    void setBudgetOpen(bool open) { budgetOpen = open; activeSlider = -1; }
    sf::FloatRect moneyChipBounds() const { return sf::FloatRect(150.f + uikit::topBarShift(), 6.f, 200.f, 36.f); }
    sf::FloatRect budgetBounds() const;
    sf::FloatRect budgetCloseBounds() const;
    sf::FloatRect taxSliderBounds(int sector, int wealth) const;
    sf::FloatRect loanButtonBounds() const;
    sf::FloatRect newCityButtonBounds() const;
    int taxFromSliderX(int sector, int wealth, float x) const;
    // Mouse on the budget panel: press / drag / release. Returns true when it used the input.
    bool handleBudgetPointer(const sf::Vector2f& p, bool down, bool justPressed);
    sf::FloatRect speedButtonBounds(int speedIndex) const;
    bool handleSpeedClick(const sf::Vector2f& point);

private:
    void handleCameraMovement(float deltaTime);
    void handleToolHotkeys();
    bool handleTileHover(const sf::Vector2i& mousePixel, bool mouseOverUI);
    void handleGestures(bool isHoveringTile, bool mouseOverUI);

    void renderWorld(float deltaTime);
    void renderOverlay(float viewL, float viewR, float viewT, float viewB);
    void renderViewPanel(const sf::Font& font, const sf::Vector2f& mouse);
    void renderTopBar(const sf::Vector2f& mouse);
    void renderInspectCard();
    void renderToast();
    void renderDragPreview();
    void renderHUD(float deltaTime);

    void refreshUI();
    void renderBudget(const sf::Vector2f& mouse);
    void renderGameOver(const sf::Vector2f& mouse);

    std::vector<sf::Vector2i> computeDragTiles() const;
    void executeMultibuild();
};

Play_Scene::~Play_Scene() {
    delete player;
    player = NULL;
}

void Play_Scene::init() {
    std::cout << "Scene created with name: Play_Scene\n";

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
    startNewCity(worldSeed);
    if (benchCity) {
        int built = buildBenchCity();
        std::cout << "\nBench city: " << built << " buildings\n";
    }
    if (benchTown) {
        int built = buildBenchTown();
        std::cout << "\nBench town: " << built << " buildings\n";
    }
    happinessSystem.updateSimulation();

    // Initialize TheoTown bottom dock
    toolbar.init(uikit::SW(), uikit::SH());

    // Screenshot helpers (see --menu / --inspect / --view)
    if (startMenu >= 1 && startMenu <= 5) toolbar.openFlyout(static_cast<ToolbarCategory>(startMenu));
    if (inWorldBounds(startInspectX, startInspectY)) { tileX = startInspectX; tileY = startInspectY; }
    setOverlayMode(startView);
    if (startBudget) setBudgetOpen(true);

    // Balance testing: simulate N days instantly and print the trend (--fast-forward N)
    if (fastForwardDays > 0) {
        for (int d = 1; d <= fastForwardDays && !economy.bankrupt; d++) {
            advanceTime(GameClock::SECONDS_PER_DAY);
            if (d % 30 == 0 || d == fastForwardDays) {
                std::printf("day %4d  money %8d  net/day %6d  pop R %d/%d/%d  jobs C %d/%d/%d I %d/%d/%d  "
                            "demand R %+.0f/%+.0f/%+.0f C %+.0f/%+.0f/%+.0f I %+.0f/%+.0f/%+.0f  happy %d  debt %d\n",
                            d, money, lastDailyIncome,
                            economy.occupants[0][0], economy.occupants[0][1], economy.occupants[0][2],
                            economy.occupants[1][0], economy.occupants[1][1], economy.occupants[1][2],
                            economy.occupants[2][0], economy.occupants[2][1], economy.occupants[2][2],
                            economy.demand[0][0], economy.demand[0][1], economy.demand[0][2],
                            economy.demand[1][0], economy.demand[1][1], economy.demand[1][2],
                            economy.demand[2][0], economy.demand[2][1], economy.demand[2][2],
                            happinessSystem.cityHappiness, economy.daysInDebt);
            }
        }
        if (economy.bankrupt) std::printf("BANKRUPT\n");
        std::fflush(stdout);
    }
}

// Generates a fresh world and resets money, economy and taxes; the camera
// moves to the new spawn. Used at start-up and after bankruptcy.
void Play_Scene::startNewCity(uint32_t seed) {
    if (seed == 0) {
        std::random_device rd;
        seed = rd() ^ static_cast<uint32_t>(std::time(nullptr));
        if (seed == 0) seed = 1;
    }
    worldSeed = seed;
    materializeWorld();   // delete every structure first
    generateWorld(worldSeed);
    ground.markDirty();
    ground.setSeasonLook(seasonLookAt(gameClock.getSeason(), gameClock.seasonProgress()));
    std::cout << "\nWorld generated with seed: " << worldSeed
              << " (spawn " << worldSpawnX << ", " << worldSpawnY << ")\n";

    money = STARTING_MONEY;
    lastDailyIncome = 0;
    simAccumulator = 0.f;
    economy.reset();
    resetTaxRates();
    for (int i = 0; i < 3; i++) { townPopulation[i] = 0; townCapacity[i] = 0; townDemand[i] = 0; }
    happinessSystem = HappinessSystem();
    happinessSystem.updateSimulation();
    setBudgetOpen(false);
    placementErrorTimer = 0.f;

    currentZoom = std::max(0.4f, std::min(3.2f, startZoom));
    view.setSize(sf::Vector2f(uikit::SW() * currentZoom, uikit::SH() * currentZoom));
    sf::Vector2f spawn = tileScreenPos(worldSpawnX, worldSpawnY);
    view.setCenter(sf::Vector2f(spawn.x + tileSizeX / 2.f, spawn.y + tileSizeY / 2.f));
    _window.setView(view);
}

// Window resized: the camera shows more / less of the map (no stretching)
// and the HUD re-anchors to the new edges.
void Play_Scene::onResize(unsigned width, unsigned height) {
    screenW = static_cast<int>(width);
    screenH = static_cast<int>(height);
    view.setSize(uikit::SW() * currentZoom, uikit::SH() * currentZoom);
    toolbar.init(uikit::SW(), uikit::SH());
}

void Play_Scene::showToast(const std::string& msg, bool isError) {
    lastPlacementError = msg;
    toastIsError = isError;
    placementErrorTimer = isError ? 2.5f : 4.f;
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
        if (!economy.bankrupt) handleToolHotkeys();

        // B toggles the budget; N founds a new city after bankruptcy
        bool budgetKey = sf::Keyboard::isKeyPressed(sf::Keyboard::B);
        if (budgetKey && !wasBudgetKey && !economy.bankrupt) setBudgetOpen(!budgetOpen);
        wasBudgetKey = budgetKey;
        bool newCityKey = sf::Keyboard::isKeyPressed(sf::Keyboard::N);
        if (newCityKey && !wasNewCityKey && economy.bankrupt) {
            gameClock = GameClock();
            startNewCity(0);
            toolbar.setActiveTool(TOOL_NONE);
        }
        wasNewCityKey = newCityKey;
    }

    // Window pixels for the world (mapped through the camera later), HUD
    // coordinates for the interface (the HUD is stretched with the window)
    sf::Vector2i mousePixel = sf::Mouse::getPosition(_window);
    sf::Vector2f mousePixelF = uikit::hudFromPixel(mousePixel, _window.getSize());

    // Check if mouse is interacting with UI elements
    bool overViewPanel = viewPanelBounds().contains(mousePixelF);
    bool overBudget = budgetOpen && budgetBounds().contains(mousePixelF);
    bool mouseOverUI = !focused || economy.bankrupt || toolbar.contains(mousePixelF) || overViewPanel ||
                       (mousePixelF.y <= 50.f) || inspectCardRect.contains(mousePixelF) || overBudget;

    bool isLeftPressed = focused && sf::Mouse::isButtonPressed(sf::Mouse::Left);
    bool justPressed = isLeftPressed && !budgetMouseWasDown;
    budgetMouseWasDown = isLeftPressed;

    if (economy.bankrupt) {
        if (justPressed && newCityButtonBounds().contains(mousePixelF)) {
            gameClock = GameClock();
            startNewCity(0);
            toolbar.setActiveTool(TOOL_NONE);
        }
    } else if (budgetOpen && handleBudgetPointer(mousePixelF, isLeftPressed, justPressed)) {
        // budget panel used the click / drag
    } else if (isLeftPressed && !wasLeftPressed && mouseOverUI) {
        if (overViewPanel) {
            handleViewPanelClick(mousePixelF);
        } else if (mousePixelF.y <= 50.f) {
            if (moneyChipBounds().contains(mousePixelF)) setBudgetOpen(!budgetOpen);
            else handleSpeedClick(mousePixelF);
        } else {
            toolbar.handleClick(mousePixelF);
        }
    }

    bool isHoveringTile = handleTileHover(mousePixel, mouseOverUI);
    if (focused && !economy.bankrupt) {
        handleGestures(isHoveringTile, mouseOverUI);
    } else {
        wasLeftPressed = isLeftPressed;
    }

    // 2. Render Passes
    _window.clear(sf::Color(15, 20, 25));
    renderWorld(deltaTime);
    renderHUD(deltaTime);
    _window.display();

    // 3. Game time: days, economy and simulation (scaled by speed, frozen when paused)
    advanceTime(deltaTime);
}

void Play_Scene::advanceTime(float realSeconds) {
    if (economy.bankrupt) { refreshUI(); return; } // the city is frozen on the game-over screen
    int newDays = gameClock.advance(realSeconds);
    for (int d = 0; d < newDays; d++) {
        runNewDay();
    }

    // One simulation tick per game second; at 4x that is 4 per real second.
    // Cap catch-up so a long stall cannot trigger a burst of ticks.
    simAccumulator += gameClock.scaledDelta(realSeconds);
    int ticks = 0;
    while (simAccumulator >= 1.f && ticks < 4) {
        runSimulationTick();
        simAccumulator -= 1.f;
        ticks++;
    }
    if (simAccumulator >= 1.f) simAccumulator = 0.f;

    ground.setSeasonLook(seasonLookAt(gameClock.getSeason(), gameClock.seasonProgress()));
    refreshUI();
}

void Play_Scene::runNewDay() {
    DayReport rep = economy.runDay(happinessSystem);
    lastDailyIncome = rep.net;

    // Debt warnings: the city may stay below $0 for at most 180 days
    int d = economy.daysInDebt;
    if (d == 1) showToast("Treasury in debt - bankrupt after 6 months in the red", true);
    else if (d == 90) showToast("3 months of debt left - raise taxes or take a loan", true);
    else if (d == 150) showToast("1 month left before bankruptcy!", true);

    if (economy.bankrupt) {
        gameClock.setSpeedIndex(0);
        setBudgetOpen(false);
        placementErrorTimer = 0.f; // the game-over screen replaces any warning
        toolbar.setActiveTool(TOOL_NONE);
    }
}

// Pause / 1x / 2x / 4x buttons, right side of the top bar
sf::FloatRect Play_Scene::speedButtonBounds(int speedIndex) const {
    return sf::FloatRect(966.f + uikit::topBarShift() + static_cast<float>(speedIndex) * 38.f, 10.f, 34.f, 28.f);
}

bool Play_Scene::handleSpeedClick(const sf::Vector2f& point) {
    for (int i = 0; i < GameClock::SPEED_COUNT; i++) {
        if (speedButtonBounds(i).contains(point)) {
            gameClock.setSpeedIndex(i);
            return true;
        }
    }
    return false;
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
        view.setSize(uikit::SW() * currentZoom, uikit::SH() * currentZoom);
    }
}

void Play_Scene::handleToolHotkeys() {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
        toolbar.setActiveTool(TOOL_NONE);
        setBudgetOpen(false);
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
    // Time: P pause/resume, [ slower, ] faster (edge-triggered)
    bool pauseKey = sf::Keyboard::isKeyPressed(sf::Keyboard::P);
    bool slowerKey = sf::Keyboard::isKeyPressed(sf::Keyboard::LBracket);
    bool fasterKey = sf::Keyboard::isKeyPressed(sf::Keyboard::RBracket);
    if (pauseKey && !wasPauseKey) gameClock.togglePause();
    if (slowerKey && !wasSlowerKey) gameClock.slower();
    if (fasterKey && !wasFasterKey) gameClock.faster();
    wasPauseKey = pauseKey;
    wasSlowerKey = slowerKey;
    wasFasterKey = fasterKey;

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
    return toolCost(tool);
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
    return sf::FloatRect(uikit::SW() - 212.f, 58.f, 200.f, 84.f);
}

sf::FloatRect Play_Scene::viewButtonBounds(int mode) const {
    sf::FloatRect panel = viewPanelBounds();
    return sf::FloatRect(panel.left + 8.f + static_cast<float>(mode) * 48.f, panel.top + 8.f, 40.f, 40.f);
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
    if (economy.bankrupt) {
        if (outReason) *outReason = "City is bankrupt";
        return false;
    }

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
        showToast("Can't build here: " + reason, true);
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

// A small starter town around the spawn (--bench-town): 2 road rows of
// ~19 tiles with ~16 buildings (mostly homes, a few shops / workshops, a
// farm, a school and a park). Used to check early-game economy balance.
int Play_Scene::buildBenchTown() {
    const int savedMoney = money;
    money = 100000000;
    int x0 = std::max(2, worldSpawnX - 9), x1 = std::min(tileMapW - 3, worldSpawnX + 9);
    int y0 = std::max(1, worldSpawnY - 4);
    for (int i = x0 - 2; i <= x1 + 2; i++)
        for (int j = y0; j <= y0 + 6; j++)
            if (inWorldBounds(i, j) && isNaturalObstacleName(tileKind(i, j))) world[i][j].terrain = Terrain::Grass;
    ground.markDirty();

    for (int y = y0; y <= y0 + 3; y += 3)
        for (int x = x0; x <= x1; x++) placeTile(x, y, TOOL_ROAD);

    const BuildTool plan[] = {
        TOOL_RES_COTTAGES, TOOL_RES_COTTAGES, TOOL_COM_GROCERY, TOOL_RES_COTTAGES, TOOL_IND_WORKSHOP,
        TOOL_RES_COTTAGES, TOOL_RES_COTTAGES, TOOL_SCHOOL, TOOL_RES_COTTAGES, TOOL_COM_GROCERY,
        TOOL_RES_COTTAGES, TOOL_IND_WORKSHOP, TOOL_RES_COTTAGES, TOOL_FARM_BARN, TOOL_RES_COTTAGES,
        TOOL_PARK, TOOL_RES_COTTAGES, TOOL_COM_GROCERY, TOOL_RES_COTTAGES, TOOL_IND_WORKSHOP};
    const int planSize = static_cast<int>(sizeof(plan) / sizeof(plan[0]));
    int built = 0, k = 0;
    for (int y = y0; y <= y0 + 3 && k < planSize; y += 3) {
        for (int x = x0 + 1; x + 1 <= x1 && k < planSize; x += 2) {
            if (canPlaceTile(x, y + 1, plan[k])) { placeTile(x, y + 1, plan[k]); built++; }
            k++;
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
                    // Building lots follow the season like natural grass; roads keep their colour
                    if (z->sprite != NULL && z->getName() != "DirtRoad") {
                        z->sprite->setColor(ground.getSeasonLook().grass);
                    }
                    z->drawGround(_window);
                } else {
                    entities[i][j]->draw();
                }
            }
        }
    }

    // PASS 1c: Winter snow over land, lots and roads
    ground.drawSnow(_window);

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
    (void)font;
    using namespace uikit;
    sf::FloatRect pb = viewPanelBounds();
    panel(_window, pb, 12.f);

    const char* icons[4] = {"view_normal", "view_fertility", "view_water", "view_deposits"};
    const char* names[4] = {"Normal", "Fertility", "Groundwater", "Deposits"};
    const sf::Color tints[4] = {sf::Color(200, 210, 220), sf::Color(120, 215, 90),
                                sf::Color(80, 170, 240), sf::Color(240, 150, 70)};

    int hovered = -1;
    for (int m = 0; m < 4; m++) {
        sf::FloatRect b = viewButtonBounds(m);
        bool active = (overlayMode == m);
        if (b.contains(mouse)) hovered = m;
        sf::Color fill = active ? withAlpha(tints[m], 60) : (hovered == m ? PANEL_HOVER : PANEL_RAISED);
        roundedRect(_window, b, 10.f, fill, active ? tints[m] : BORDER, active ? 2.f : 1.f);
        icon(_window, icons[m], b.left + 4.f, b.top + 4.f, 32.f, tints[m]);
    }

    // Caption + legend for the active view (hover shows the hovered view's name)
    float ly = pb.top + 56.f;
    if (hovered >= 0 && hovered != overlayMode) {
        textMid(_window, std::string("View: ") + names[hovered] + "  [O]", pb.left + 10.f, ly, 20.f, 11, TEXT_DIM);
        return;
    }
    auto key = [&](float x, const sf::Color& c, const std::string& label) {
        roundedRect(_window, sf::FloatRect(x, ly + 5.f, 10.f, 10.f), 3.f, c, BORDER, 1.f);
        return 14.f + text(_window, label, x + 14.f, ly + 5.f, 10, TEXT_DIM) + 10.f;
    };
    float x = pb.left + 10.f;
    switch (overlayMode) {
        case 1: x += key(x, sf::Color(210, 60, 50), "Barren"); key(x, sf::Color(40, 210, 50), "Rich soil"); break;
        case 2: x += key(x, sf::Color(40, 130, 230, 90), "Dry"); key(x, sf::Color(40, 130, 230, 240), "Wet ground"); break;
        case 3: x += key(x, sf::Color(30, 30, 36), "Coal"); x += key(x, sf::Color(200, 85, 30), "Iron");
                key(x, sf::Color(215, 215, 220), "Stone"); break;
        default: textMid(_window, "Map view  [O]", pb.left + 10.f, ly, 20.f, 11, TEXT_DIM); break;
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

// Daily tax with one decimal, e.g. "2.5"
static std::string dailyTaxText(const Zone* z) {
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%.1f", z ? z->dailyTax() : 0.f);
    return buf;
}

void Play_Scene::renderHUD(float deltaTime) {
    (void)deltaTime;
    // HUD view matches the window 1:1, so nothing is stretched
    _window.setView(sf::View(sf::FloatRect(0.f, 0.f, uikit::SW(), uikit::SH())));
    const sf::Font& font = m_fonts["../assets/Hack.ttf"];

    sf::Vector2f mouse = uikit::hudFromPixel(sf::Mouse::getPosition(_window), _window.getSize());

    renderTopBar(mouse);
    renderViewPanel(font, mouse);
    renderInspectCard();
    toolbar.draw(_window, font, mouse);
    if (budgetOpen) renderBudget(mouse);
    renderToast();
    if (economy.bankrupt) renderGameOver(mouse);

    // Small diagnostics, bottom-right
    std::string diag = std::to_string(calculatedFPS) + " FPS  |  Seed " + std::to_string(worldSeed);
    float dw = uikit::textWidth(diag, 10) + 18.f;
    sf::FloatRect dr(uikit::SW() - dw - 10.f, uikit::SH() - 30.f, dw, 20.f);
    uikit::roundedRect(_window, dr, 8.f, uikit::PANEL, uikit::BORDER, 1.f);
    uikit::textMid(_window, diag, dr.left + 9.f, dr.top, dr.height, 10, uikit::TEXT_DIM);
}

// Top status bar: city, money, population + demand, happiness, date/season + speed
void Play_Scene::renderTopBar(const sf::Vector2f& mouse) {
    using namespace uikit;
    const float W = SW();
    const float sh = topBarShift();

    sf::RectangleShape bar(sf::Vector2f(W, 48.f));
    bar.setFillColor(sf::Color(12, 16, 23, 240));
    _window.draw(bar);
    sf::RectangleShape line(sf::Vector2f(W, 1.f));
    line.setPosition(0.f, 48.f);
    line.setFillColor(BORDER);
    _window.draw(line);

    auto chip = [&](float x, float w) {
        sf::FloatRect r(x, 6.f, w, 36.f);
        roundedRect(_window, r, 10.f, PANEL_RAISED, BORDER, 1.f);
        return r;
    };

    // City name (hidden on narrow windows)
    if (sh == 0.f) {
        text(_window, "Chime City", 16.f, 9.f, 15, ACCENT, true);
        text(_window, "Year " + std::to_string(gameClock.getYear()), 16.f, 29.f, 10, TEXT_DIM);
    }

    // Treasury (click for the budget)
    sf::FloatRect m = moneyChipBounds();
    bool moneyHover = m.contains(mouse);
    roundedRect(_window, m, 10.f, moneyHover || budgetOpen ? PANEL_HOVER : PANEL_RAISED,
                budgetOpen ? ACCENT : BORDER, budgetOpen ? 2.f : 1.f);
    icon(_window, "coin", m.left + 8.f, m.top + 6.f, 24.f);
    text(_window, formatMoney(money), m.left + 40.f, m.top + 4.f, 15, money < 0 ? BAD : TEXT, true);
    if (economy.daysInDebt > 0) {
        text(_window, "Debt! " + std::to_string(economy.daysUntilBankrupt()) + " days left", m.left + 40.f, m.top + 22.f, 10, BAD, true);
    } else {
        sf::Color flow = lastDailyIncome > 0 ? GOOD : (lastDailyIncome < 0 ? BAD : TEXT_DIM);
        std::string sign = lastDailyIncome < 0 ? "-" : "+";
        text(_window, sign + formatMoney(std::abs(lastDailyIncome)) + " / day   [B]", m.left + 40.f, m.top + 22.f, 10, flow);
    }

    // Population + demand by income level
    sf::FloatRect p = chip(360.f + sh, 190.f);
    icon(_window, "people", p.left + 8.f, p.top + 6.f, 24.f);
    int pop = townPopulation[0] + townPopulation[1] + townPopulation[2];
    text(_window, withCommas(pop), p.left + 40.f, p.top + 4.f, 15, TEXT, true);
    text(_window, "citizens", p.left + 40.f, p.top + 22.f, 10, TEXT_DIM);
    // Demand: R C I groups, each $ $$ $$$ (up = wanted, down red = surplus)
    const sf::Color rci[3] = {GOOD, ACCENT, sf::Color(235, 140, 50)};
    float bx = p.left + 98.f;
    float mid = p.top + 18.f;
    for (int sct = 0; sct < 3; sct++) {
        for (int w = 0; w < 3; w++) {
            float d = economy.demand[sct][w] / 100.f;
            float x = bx + static_cast<float>(sct) * 30.f + static_cast<float>(w) * 8.f;
            sf::RectangleShape track(sf::Vector2f(6.f, 26.f));
            track.setPosition(x, mid - 13.f);
            track.setFillColor(sf::Color(10, 13, 19, 220));
            _window.draw(track);
            float h = std::max(1.f, 13.f * std::fabs(d));
            sf::RectangleShape barShape(sf::Vector2f(6.f, h));
            barShape.setPosition(x, d >= 0.f ? mid - h : mid);
            barShape.setFillColor(d >= 0.f ? rci[sct] : BAD);
            _window.draw(barShape);
        }
        text(_window, sectorLetter(sct), bx + static_cast<float>(sct) * 30.f + 7.f, p.top + 30.f, 8, TEXT_DIM, true);
    }

    // Happiness
    int happy = happinessSystem.cityHappiness;
    sf::Color hc = happy >= 75 ? GOOD : (happy >= 50 ? WARN : BAD);
    sf::FloatRect h = chip(560.f + sh, 130.f);
    icon(_window, "smile", h.left + 8.f, h.top + 6.f, 24.f, hc);
    text(_window, std::to_string(happy) + "%", h.left + 40.f, h.top + 4.f, 15, hc, true);
    text(_window, "happiness", h.left + 40.f, h.top + 22.f, 10, TEXT_DIM);

    // Date, season and speed controls
    Season season = gameClock.getSeason();
    const char* seasonIcons[4] = {"season_spring", "season_summer", "season_autumn", "season_winter"};
    const sf::Color seasonColors[4] = {sf::Color(120, 220, 110), sf::Color(250, 205, 70),
                                       sf::Color(235, 140, 60), sf::Color(170, 205, 245)};
    sf::FloatRect d = chip(700.f + sh, 420.f);
    icon(_window, seasonIcons[static_cast<int>(season)], d.left + 8.f, d.top + 6.f, 24.f);
    text(_window, gameClock.dateString(), d.left + 40.f, d.top + 4.f, 14, TEXT, true);
    std::string sub = GameClock::seasonName(season);
    if (gameClock.isPaused()) sub += "  -  PAUSED";
    text(_window, sub, d.left + 40.f, d.top + 22.f, 10, gameClock.isPaused() ? WARN : seasonColors[static_cast<int>(season)], true);

    const char* speedIcons[GameClock::SPEED_COUNT] = {"speed_pause", "speed_1", "speed_2", "speed_4"};
    for (int i = 0; i < GameClock::SPEED_COUNT; i++) {
        sf::FloatRect b = speedButtonBounds(i);
        bool active = gameClock.getSpeedIndex() == i;
        bool hovered = b.contains(mouse);
        sf::Color fill = active ? sf::Color(30, 80, 120) : (hovered ? PANEL_HOVER : sf::Color(18, 23, 32));
        roundedRect(_window, b, 7.f, fill, active ? ACCENT : BORDER, active ? 2.f : 1.f);
        icon(_window, speedIcons[i], b.left + 5.f, b.top + 2.f, 24.f, active ? sf::Color::White : TEXT_DIM);
    }
}

// Placement error toast, bottom-centre, fading out
void Play_Scene::renderToast() {
    using namespace uikit;
    if (placementErrorTimer <= 0.f || lastPlacementError.empty()) return;
    float alpha = std::min(1.f, placementErrorTimer / 0.5f); // fade over the last half second
    const std::string& msg = lastPlacementError;
    float w = textWidth(msg, 13, true) + 64.f;
    sf::FloatRect r((SW() - w) / 2.f, SH() - 84.f, w, 40.f);
    sf::Uint8 a = static_cast<sf::Uint8>(235.f * alpha);
    roundedRect(_window, sf::FloatRect(r.left + 2.f, r.top + 3.f, r.width, r.height), 12.f, withAlpha(SHADOW, static_cast<sf::Uint8>(90.f * alpha)));
    sf::Color edge = toastIsError ? BAD : ACCENT;
    roundedRect(_window, r, 12.f, toastIsError ? sf::Color(48, 20, 22, a) : sf::Color(18, 34, 52, a), withAlpha(edge, a), 1.5f);
    icon(_window, "warning", r.left + 12.f, r.top + 8.f, 24.f, sf::Color(255, 255, 255, a));
    textMid(_window, msg, r.left + 44.f, r.top, r.height, 13, withAlpha(sf::Color(255, 215, 210), a), true);
}

// Inspect card (Inspect mode): building or natural tile under the cursor
void Play_Scene::renderInspectCard() {
    using namespace uikit;
    inspectCardRect = sf::FloatRect();
    if (toolbar.getActiveTool() != TOOL_NONE) return;
    if (tileX < 0 || tileX >= tileMapW || tileY < 0 || tileY >= tileMapH) return;

    bool natural = (entities[tileX][tileY] == NULL);
    Zone* z = natural ? NULL : dynamic_cast<Zone*>(entities[tileX][tileY]);
    Zone* mz = z;
    if (z != NULL && z->isSubTile && z->rootX >= 0 && z->rootY >= 0) {
        mz = dynamic_cast<Zone*>(entities[z->rootX][z->rootY]);
    }
    if (!natural && mz == NULL) return;

    const std::string kind = natural ? tileKind(tileX, tileY) : mz->getName();
    const bool isRoad = (kind == "DirtRoad");
    const bool isBuilding = !natural && !isRoad;
    const bool isZone = kind == "ResidentalZone" || kind == "CommericalZone" || kind == "IndustrialZone";

    // Title, subtitle, accent and thumbnail
    std::string title, subtitle;
    sf::Color accent = ACCENT;
    if (natural) {
        title = terrainName(world[tileX][tileY].terrain);
        subtitle = isClearGroundName(kind) ? "Open land - buildable" : "Bulldoze to clear";
        accent = sf::Color(120, 200, 110);
    } else if (isRoad) {
        title = "Dirt Road"; subtitle = "Upkeep $1 / day"; accent = sf::Color(190, 196, 204);
    } else if (kind == "ResidentalZone") {
        title = "Residential " + wealthSigns(mz->wealth()); subtitle = dynamic_cast<ResidentalZone*>(mz)->getVariantName(); accent = GOOD;
    } else if (kind == "CommericalZone") {
        title = "Commercial " + wealthSigns(mz->wealth()); subtitle = dynamic_cast<CommericalZone*>(mz)->getVariantName(); accent = ACCENT;
    } else if (kind == "IndustrialZone") {
        title = "Industrial " + wealthSigns(mz->wealth()); subtitle = dynamic_cast<IndustrialZone*>(mz)->getVariantName(); accent = sf::Color(235, 140, 50);
    } else if (kind == "FarmZone") {
        title = "Farm"; subtitle = dynamic_cast<FarmZone*>(mz)->getVariantName(); accent = sf::Color(150, 205, 90);
    } else if (kind == "SchoolZone") {
        title = "Elementary School"; subtitle = "Education - radius 5"; accent = BAD;
    } else if (kind == "ChurchZone") {
        title = "Community Church"; subtitle = "Faith - radius 4"; accent = sf::Color(170, 120, 220);
    } else if (kind == "ParkZone") {
        title = "Nature Park"; subtitle = "Health & fun - radius 4"; accent = GOOD;
    } else if (kind == "HarbourZone") {
        title = "Trade Harbour"; subtitle = "Sea trade port"; accent = ACCENT;
    } else {
        title = kind;
    }

    // Height: header + building rows + land section
    const float W = 320.f;
    float hgt = 82.f;                       // header
    if (isBuilding) hgt += 34.f;            // status + tax row
    if (isZone) hgt += 2 * 30.f;            // occupancy + happiness bars
    if (kind == "FarmZone") hgt += 22.f;    // soil / season line
    hgt += 112.f;                           // land section
    sf::FloatRect card(SW() - W - 12.f, 152.f, W, hgt);
    inspectCardRect = card;
    panel(_window, card, 12.f);
    roundedRect(_window, sf::FloatRect(card.left, card.top, card.width, 4.f), 2.f, accent);

    // Header: thumbnail well + title
    sf::FloatRect well(card.left + 12.f, card.top + 14.f, 64.f, 56.f);
    roundedRect(_window, well, 9.f, sf::Color(10, 13, 19, 230));
    sf::FloatRect inner(well.left + 4.f, well.top + 4.f, well.width - 8.f, well.height - 8.f);
    if (isBuilding && mz->buildingSprite != NULL) {
        thumbnailTex(_window, mz->buildingSprite->getTexture(), inner);
    } else if (isRoad) {
        thumbnail(_window, "road", inner);
    } else if (natural) {
        Terrain t = world[tileX][tileY].terrain;
        const char* key = t == Terrain::Water ? "water" : t == Terrain::Sand ? "sand" : t == Terrain::Forest ? "forest"
                        : t == Terrain::Rock ? "rock" : t == Terrain::ForestPath ? "forest_path" : "grass";
        thumbnail(_window, key, inner);
    }

    float tx = well.left + well.width + 12.f;
    const float maxTitleW = card.left + card.width - 14.f - tx;
    text(_window, fitText(title, 15, maxTitleW, true), tx, card.top + 18.f, 15, TEXT, true);
    text(_window, fitText(subtitle, 11, maxTitleW), tx, card.top + 38.f, 11, TEXT_DIM);
    if (isZone) {
        for (int s = 0; s < 3; s++) {
            icon(_window, "star", tx + static_cast<float>(s) * 18.f, card.top + 54.f, 16.f,
                 s < mz->tier ? sf::Color::White : sf::Color(255, 255, 255, 45));
        }
    } else if (isBuilding || isRoad) {
        text(_window, isRoad ? "1x1 tile" : "2x2 building", tx, card.top + 56.f, 10, TEXT_DIM);
    }

    float y = card.top + 82.f;
    const float lx = card.left + 14.f, rw = card.width - 28.f;

    auto barRow = [&](const std::string& label, const std::string& value, float frac, const sf::Color& c) {
        text(_window, label, lx, y, 11, TEXT_DIM);
        textRight(_window, value, lx + rw, y, 11, TEXT, true);
        bar(_window, sf::FloatRect(lx, y + 16.f, rw, 7.f), frac, c);
        y += 30.f;
    };

    if (isBuilding) {
        bool connected = mz->isActive;
        sf::Color sc = connected ? GOOD : BAD;
        std::string status = connected ? "Road connected" : "No road access";
        if (kind == "ParkZone") status = "Always open";
        float sw = textWidth(status, 11, true) + 20.f;
        roundedRect(_window, sf::FloatRect(lx, y, sw, 22.f), 11.f, withAlpha(sc, 45), sc, 1.f);
        textMid(_window, status, lx + 10.f, y, 22.f, 11, sc, true);
        if (mz->dailyUpkeep() > 0) {
            std::string cost = "-$" + std::to_string(mz->dailyUpkeep()) + " / day";
            textMid(_window, cost, lx + rw - textWidth(cost, 12, true), y, 22.f, 12, BAD, true);
        } else {
            std::string tax = "$" + dailyTaxText(mz) + " / day";
            textMid(_window, tax, lx + rw - textWidth(tax, 12, true), y, 22.f, 12, mz->dailyTax() > 0.f ? GOOD : TEXT_DIM, true);
        }
        y += 34.f;
    }
    if (isZone) {
        float occ = mz->capacity > 0 ? static_cast<float>(mz->population) / static_cast<float>(mz->capacity) : 0.f;
        barRow("Occupancy", std::to_string(mz->population) + " / " + std::to_string(mz->capacity), occ, ACCENT);
        int hap = mz->overallHappiness;
        barRow("Happiness", std::to_string(hap) + "%", static_cast<float>(hap) / 100.f,
               hap >= 75 ? GOOD : (hap >= 50 ? WARN : BAD));
    }
    if (kind == "FarmZone") {
        FarmZone* fz = dynamic_cast<FarmZone*>(mz);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Soil x%.2f  -  %s x%.1f", fz->fertilityMul,
                      GameClock::seasonName(gameClock.getSeason()).c_str(), seasonalFarmMultiplier(gameClock.getSeason()));
        text(_window, buf, lx, y, 11, sf::Color(170, 220, 120), true);
        y += 22.f;
    }

    // Land section
    sf::RectangleShape div(sf::Vector2f(rw, 1.f));
    div.setPosition(lx, y);
    div.setFillColor(BORDER);
    _window.draw(div);
    y += 8.f;
    const TileData& land = world[tileX][tileY];
    text(_window, "LAND", lx, y, 10, TEXT_DIM, true);
    textRight(_window, terrainName(land.terrain), lx + rw, y, 10, TEXT_DIM);
    y += 18.f;
    barRow("Soil fertility", std::to_string(land.fertility) + "%", static_cast<float>(land.fertility) / 100.f,
           sf::Color(120, 200, 80));
    barRow("Groundwater", std::to_string(land.groundwater) + "%", static_cast<float>(land.groundwater) / 100.f,
           sf::Color(70, 150, 230));
    if (land.deposit == Deposit::None) {
        text(_window, "Deposit", lx, y, 11, TEXT_DIM);
        textRight(_window, "None", lx + rw, y, 11, TEXT_DIM, true);
    } else {
        sf::Color dc = land.deposit == Deposit::Coal ? sf::Color(150, 150, 160)
                     : land.deposit == Deposit::Iron ? sf::Color(220, 110, 50) : sf::Color(220, 220, 225);
        barRow(depositName(land.deposit), std::to_string(land.depositAmount) + "%",
               static_cast<float>(land.depositAmount) / 100.f, dc);
    }
}

// -------------------------------------------------------------
// Budget panel: tax sliders + demand per income level, expenses, loans
// -------------------------------------------------------------
sf::FloatRect Play_Scene::budgetBounds() const {
    // Centred; never under the top bar
    return sf::FloatRect((uikit::SW() - 780.f) / 2.f, std::max(56.f, (uikit::SH() - 600.f) / 2.f), 780.f, 600.f);
}

sf::FloatRect Play_Scene::budgetCloseBounds() const {
    sf::FloatRect b = budgetBounds();
    return sf::FloatRect(b.left + b.width - 44.f, b.top + 12.f, 30.f, 30.f);
}

// Row layout: 3 sector groups (header 24 px) of 3 income rows (34 px)
static float budgetRowTop(const sf::FloatRect& b, int sector, int wealth) {
    return b.top + 92.f + static_cast<float>(sector) * (24.f + 3.f * 34.f) + 24.f + static_cast<float>(wealth) * 34.f;
}

sf::FloatRect Play_Scene::taxSliderBounds(int sector, int wealth) const {
    sf::FloatRect b = budgetBounds();
    return sf::FloatRect(b.left + 150.f, budgetRowTop(b, sector, wealth) + 13.f, 170.f, 8.f);
}

sf::FloatRect Play_Scene::loanButtonBounds() const {
    sf::FloatRect b = budgetBounds();
    return sf::FloatRect(b.left + 510.f, b.top + 470.f, 250.f, 34.f);
}

sf::FloatRect Play_Scene::newCityButtonBounds() const {
    return sf::FloatRect(uikit::SW() / 2.f - 110.f, uikit::SH() / 2.f + 60.f, 220.f, 40.f);
}

int Play_Scene::taxFromSliderX(int sector, int wealth, float x) const {
    sf::FloatRect t = taxSliderBounds(sector, wealth);
    float f = std::max(0.f, std::min(1.f, (x - t.left) / t.width));
    return TAX_MIN + static_cast<int>(std::round(f * static_cast<float>(TAX_MAX - TAX_MIN)));
}

bool Play_Scene::handleBudgetPointer(const sf::Vector2f& p, bool down, bool justPressed) {
    if (!budgetOpen) return false;
    sf::FloatRect b = budgetBounds();

    // Which row is hovered (for the demand hint)
    hoveredBucket = -1;
    for (int sct = 0; sct < SECTOR_COUNT; sct++)
        for (int w = 0; w < WEALTH_COUNT; w++) {
            float top = budgetRowTop(b, sct, w);
            if (p.x >= b.left + 16.f && p.x <= b.left + 490.f && p.y >= top && p.y < top + 34.f) hoveredBucket = sct * 3 + w;
        }

    if (justPressed) {
        if (budgetCloseBounds().contains(p)) { setBudgetOpen(false); return true; }
        if (loanButtonBounds().contains(p)) {
            if (economy.takeLoan()) showToast("Loan received: +$10,000 (repaid daily over a year)", false);
            else showToast("Loan limit reached (" + std::to_string(LOAN_MAX_ACTIVE) + " active loans)", true);
            return true;
        }
        for (int sct = 0; sct < SECTOR_COUNT; sct++) {
            for (int w = 0; w < WEALTH_COUNT; w++) {
                sf::FloatRect t = taxSliderBounds(sct, w);
                sf::FloatRect hit(t.left - 8.f, t.top - 10.f, t.width + 16.f, t.height + 20.f);
                if (hit.contains(p)) {
                    activeSlider = sct * 3 + w;
                    taxRates[sct][w] = taxFromSliderX(sct, w, p.x);
                    return true;
                }
            }
        }
        return b.contains(p); // clicks on the panel never reach the map
    }
    if (down && activeSlider >= 0) {
        int sct = activeSlider / 3, w = activeSlider % 3;
        taxRates[sct][w] = taxFromSliderX(sct, w, p.x);
        return true;
    }
    if (!down) activeSlider = -1;
    return false;
}

void Play_Scene::renderBudget(const sf::Vector2f& mouse) {
    using namespace uikit;
    // keep the hover hint current even without clicks
    handleBudgetPointer(mouse, activeSlider >= 0, false);

    sf::FloatRect b = budgetBounds();
    panel(_window, b, 14.f);
    text(_window, "City Budget", b.left + 20.f, b.top + 16.f, 18, TEXT, true);
    text(_window, "Daily figures  -  drag a slider to change that tax rate", b.left + 20.f, b.top + 42.f, 11, TEXT_DIM);

    sf::FloatRect xb = budgetCloseBounds();
    bool xHover = xb.contains(mouse);
    roundedRect(_window, xb, 8.f, xHover ? sf::Color(120, 40, 36) : PANEL_RAISED, BORDER, 1.f);
    textMid(_window, "x", xb.left + 10.f, xb.top - 1.f, xb.height, 15, xHover ? sf::Color::White : TEXT_DIM, true);

    // ---- left: taxes and demand ----
    const sf::Color rci[3] = {GOOD, ACCENT, sf::Color(235, 140, 50)};
    float lx = b.left + 20.f;
    text(_window, "TAX & DEMAND", lx, b.top + 72.f, 11, TEXT_DIM, true);
    text(_window, "rate", lx + 336.f, b.top + 72.f, 9, TEXT_DIM);
    text(_window, "income", lx + 402.f, b.top + 72.f, 9, TEXT_DIM);
    for (int sct = 0; sct < SECTOR_COUNT; sct++) {
        float gy = budgetRowTop(b, sct, 0) - 24.f;
        roundedRect(_window, sf::FloatRect(lx, gy + 6.f, 4.f, 14.f), 2.f, rci[sct]);
        text(_window, sectorName(sct), lx + 10.f, gy + 6.f, 12, TEXT, true);

        for (int w = 0; w < WEALTH_COUNT; w++) {
            float ry = budgetRowTop(b, sct, w);
            if (hoveredBucket == sct * 3 + w) {
                roundedRect(_window, sf::FloatRect(lx - 6.f, ry, 478.f, 32.f), 8.f, sf::Color(255, 255, 255, 12));
            }
            text(_window, wealthSigns(w), lx + 10.f, ry + 10.f, 12, rci[sct], true);

            // Demand bar centred on zero
            float d = economy.demand[sct][w] / 100.f;
            sf::FloatRect db(lx + 36.f, ry + 12.f, 44.f, 10.f);
            roundedRect(_window, db, 3.f, sf::Color(10, 13, 19, 220));
            float half = db.width / 2.f;
            float wbar = std::max(1.f, half * std::fabs(d));
            sf::RectangleShape fill(sf::Vector2f(wbar, db.height - 2.f));
            fill.setPosition(d >= 0.f ? db.left + half : db.left + half - wbar, db.top + 1.f);
            fill.setFillColor(d >= 0.f ? rci[sct] : BAD);
            _window.draw(fill);
            sf::RectangleShape zero(sf::Vector2f(1.f, db.height + 4.f));
            zero.setPosition(db.left + half, db.top - 2.f);
            zero.setFillColor(TEXT_DIM);
            _window.draw(zero);
            char dbuf[8];
            int dval = static_cast<int>(std::lround(economy.demand[sct][w]));
            if (dval == 0) std::snprintf(dbuf, sizeof(dbuf), "0");
            else std::snprintf(dbuf, sizeof(dbuf), "%+d", dval);
            textRight(_window, dbuf, lx + 120.f, ry + 10.f, 10, d >= 0.f ? rci[sct] : BAD, true);

            // Tax slider
            sf::FloatRect t = taxSliderBounds(sct, w);
            int rate = taxRates[sct][w];
            float f = static_cast<float>(rate - TAX_MIN) / static_cast<float>(TAX_MAX - TAX_MIN);
            sf::Color rateCol = rate >= 13 ? BAD : (rate <= 5 ? GOOD : TEXT);
            bar(_window, t, f, withAlpha(rci[sct], 170));
            float kx = t.left + t.width * f;
            bool active = activeSlider == sct * 3 + w;
            sf::CircleShape knob(active ? 9.f : 7.f);
            knob.setOrigin(knob.getRadius(), knob.getRadius());
            knob.setPosition(kx, t.top + t.height / 2.f);
            knob.setFillColor(active ? sf::Color::White : sf::Color(220, 228, 238));
            knob.setOutlineColor(rci[sct]);
            knob.setOutlineThickness(2.f);
            _window.draw(knob);
            text(_window, std::to_string(rate) + "%", lx + 336.f, ry + 9.f, 12, rateCol, true);

            float inc = economy.lastDay.tax[sct][w];
            char buf[24];
            std::snprintf(buf, sizeof(buf), "$%.0f", inc);
            textRight(_window, buf, lx + 470.f, ry + 9.f, 12, inc > 0.5f ? GOOD : TEXT_DIM, true);
        }
    }

    // Demand hint for the hovered row
    std::string hint = "Hover a row to see what limits its demand";
    if (hoveredBucket >= 0) {
        int sct = hoveredBucket / 3, w = hoveredBucket % 3;
        char buf[96];
        std::snprintf(buf, sizeof(buf), "%s %s: demand %+.0f  -  %s", sectorLetter(sct).c_str(), wealthSigns(w).c_str(),
                      economy.demand[sct][w], economy.limitingFactor(sct, w).c_str());
        hint = buf;
    }
    text(_window, hint, lx, b.top + b.height - 30.f, 11, hoveredBucket >= 0 ? WARN : TEXT_DIM, hoveredBucket >= 0);

    // ---- right: expenses, summary, loans ----
    float rx = b.left + 510.f, rw = 250.f;
    sf::RectangleShape divider(sf::Vector2f(1.f, b.height - 110.f));
    divider.setPosition(rx - 16.f, b.top + 72.f);
    divider.setFillColor(BORDER);
    _window.draw(divider);

    const DayReport& rep = economy.lastDay;
    float y = b.top + 72.f;
    text(_window, "DAILY EXPENSES", rx, y, 11, TEXT_DIM, true);
    y += 22.f;
    auto row = [&](const std::string& label, const std::string& value, const sf::Color& c) {
        text(_window, label, rx, y, 12, TEXT_DIM);
        textRight(_window, value, rx + rw, y, 12, c, true);
        y += 22.f;
    };
    auto dollars = [](float v) { return "$" + withCommas(static_cast<long long>(std::round(v))); };
    row("Roads (" + std::to_string(rep.roadTiles) + " tiles)", dollars(static_cast<float>(rep.upkeepRoads)), TEXT);
    row("Schools", dollars(static_cast<float>(rep.upkeepSchools)), TEXT);
    row("Churches", dollars(static_cast<float>(rep.upkeepChurches)), TEXT);
    row("Parks", dollars(static_cast<float>(rep.upkeepParks)), TEXT);
    row("Harbours", dollars(static_cast<float>(rep.upkeepHarbours)), TEXT);
    row("Loan repayments", dollars(rep.loanPayments), TEXT);

    y += 8.f;
    text(_window, "SUMMARY", rx, y, 11, TEXT_DIM, true);
    y += 22.f;
    row("Tax income", "+" + dollars(rep.taxTotal), GOOD);
    row("Expenses", "-" + dollars(rep.expenseTotal), BAD);
    float net = rep.taxTotal - rep.expenseTotal;
    text(_window, "Net per day", rx, y, 13, TEXT, true);
    textRight(_window, (net >= 0.f ? "+" : "-") + dollars(std::fabs(net)), rx + rw, y, 13, net >= 0.f ? GOOD : BAD, true);
    y += 30.f;

    text(_window, "LOANS", rx, y, 11, TEXT_DIM, true);
    y += 22.f;
    row("Active", std::to_string(economy.loans.size()) + " / " + std::to_string(LOAN_MAX_ACTIVE), TEXT);
    row("Still owed", dollars(economy.loanBalance()), economy.loans.empty() ? TEXT_DIM : WARN);

    sf::FloatRect lb = loanButtonBounds();
    bool canLoan = economy.canTakeLoan();
    bool lHover = lb.contains(mouse) && canLoan;
    roundedRect(_window, lb, 10.f, canLoan ? (lHover ? sf::Color(40, 90, 130) : sf::Color(28, 64, 96)) : PANEL_RAISED,
                canLoan ? ACCENT : BORDER, 1.f);
    std::string loanLabel = canLoan ? "Take $10,000 loan (10%)" : "Loan limit reached";
    textMid(_window, loanLabel, lb.left + (lb.width - textWidth(loanLabel, 12, true)) / 2.f, lb.top, lb.height, 12,
            canLoan ? TEXT : TEXT_DIM, true);
    text(_window, "Repaid daily over 1 year", rx, lb.top + lb.height + 6.f, 10, TEXT_DIM);

    // Debt warning strip
    if (economy.daysInDebt > 0) {
        sf::FloatRect w(b.left + 16.f, b.top + b.height - 70.f, b.width - 32.f, 30.f);
        roundedRect(_window, w, 8.f, sf::Color(70, 22, 24, 235), BAD, 1.f);
        std::string msg = "In debt for " + std::to_string(economy.daysInDebt) + " days  -  bankrupt in " +
                          std::to_string(economy.daysUntilBankrupt()) + " days unless the treasury recovers";
        icon(_window, "warning", w.left + 8.f, w.top + 5.f, 20.f);
        textMid(_window, msg, w.left + 36.f, w.top, w.height, 11, sf::Color(255, 210, 205), true);
    }
}

void Play_Scene::renderGameOver(const sf::Vector2f& mouse) {
    using namespace uikit;
    sf::RectangleShape dim(sf::Vector2f(SW(), SH()));
    dim.setFillColor(sf::Color(5, 7, 10, 190));
    _window.draw(dim);

    sf::FloatRect b(SW() / 2.f - 260.f, SH() / 2.f - 140.f, 520.f, 270.f);
    panel(_window, b, 16.f, PANEL, BAD);
    icon(_window, "warning", b.left + b.width / 2.f - 24.f, b.top + 20.f, 48.f);
    std::string title = "City Bankrupt";
    text(_window, title, b.left + (b.width - textWidth(title, 22, true)) / 2.f, b.top + 80.f, 22, BAD, true);
    std::string l1 = "The treasury stayed in debt for 6 months.";
    std::string l2 = "Reached " + gameClock.dateString() + " with " +
                     withCommas(townPopulation[0] + townPopulation[1] + townPopulation[2]) + " citizens.";
    text(_window, l1, b.left + (b.width - textWidth(l1, 12)) / 2.f, b.top + 118.f, 12, TEXT);
    text(_window, l2, b.left + (b.width - textWidth(l2, 12)) / 2.f, b.top + 140.f, 12, TEXT_DIM);

    sf::FloatRect nb = newCityButtonBounds();
    bool hover = nb.contains(mouse);
    roundedRect(_window, nb, 10.f, hover ? sf::Color(40, 90, 130) : sf::Color(28, 64, 96), ACCENT, 1.f);
    std::string label = "Found a new city  [N]";
    textMid(_window, label, nb.left + (nb.width - textWidth(label, 13, true)) / 2.f, nb.top, nb.height, 13, TEXT, true);
}

void Play_Scene::refreshUI() {
    yr = gameClock.getYear();
    mon = gameClock.getMonth();
    day = gameClock.getDay();
}
