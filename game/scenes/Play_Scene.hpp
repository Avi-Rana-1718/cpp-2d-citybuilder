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
#include "../entities/UtilityZone.hpp"
#include "../entities/ServiceZone.hpp"
#include <deque>
#include <sstream>
#include <fstream>
#include <cstring>
#include "../entities/roads/RoadTile.hpp"
#include "../world/WorldData.hpp"
#include "../world/WorldGen.hpp"
#include "../world/GroundRenderer.hpp"
#include "../world/RoadNetwork.hpp"
#include "../world/CarRenderer.hpp"
#include "../simulation/HappinessSystem.hpp"
#include "../simulation/GameClock.hpp"
#include "../simulation/Economy.hpp"
#include "../simulation/UtilitySystem.hpp"
#include "../simulation/ServiceSystem.hpp"
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

    // World overlay: 0 None, 1 Fertility, 2 Groundwater, 3 Deposits, 4 Traffic, 5 Power, 6 Water
    int overlayMode = 0;
    bool utilitiesDirty = true;   // a building or road changed: recompute who has power / water
    int shortageToastDay = -100;  // last day a utility shortage toast was shown
    BuildTool lastTool = TOOL_NONE;   // to switch the map view with power / water tools
    bool spawningZoned = false;       // placeTile is growing a building on a zone (free, not fixed)

    // Message log (L): shortages, fires, debt, saves
    struct Notice { long day; std::string text; bool error; };
    bool logOpen = false;
    bool wasLogKey = false, wasSaveKey = false, wasLoadKey = false, wasUndoKey = false;
    int viewBeforeUtilityTool = 0;
    std::vector<int> carHead, carNext; // per-frame car buckets for the depth pass
    bool wasOverlayKeyPressed = false;

public:
    Toolbar toolbar;
    HappinessSystem happinessSystem;
    GroundRenderer ground; // natural terrain, drawn from world[][] in one call
    CarRenderer cars;      // visual traffic (TrafficSystem decides where cars go)

    ~Play_Scene();
    void init() override;
    void update(float deltaTime) override;
    void onResize(unsigned width, unsigned height) override;

    bool canPlaceTile(int x, int y, BuildTool tool, std::string* outReason = nullptr) const;
    int getToolCost(BuildTool tool) const;
    int getBulldozeCost(int x, int y) const;
    // What placing a tool on (x, y) costs: road upgrades pay the difference
    int getPlacementCost(int x, int y, BuildTool tool) const;
    std::string getOverlayName() const;
    int getOverlayMode() const { return overlayMode; }
    static const int OVERLAY_COUNT = 8; // Normal, Fertility, Groundwater, Deposits, Traffic, Power, Water, Land value
    void setOverlayMode(int mode) { overlayMode = (mode >= 0 && mode < OVERLAY_COUNT) ? mode : 0; }
    sf::FloatRect viewPanelBounds() const;
    sf::FloatRect viewButtonBounds(int mode) const;
    bool handleViewPanelClick(const sf::Vector2f& point);
    void placeTile(int x, int y, BuildTool tool);
    void refreshRoadsAround(int x, int y);
    // A road tile appeared, changed or vanished at (x, y)
    void onRoadsChanged(int x, int y);

    // ---- Road drags (shared by the preview and the build) ----
    enum RoadAction { ROAD_BUILD, ROAD_UPGRADE, ROAD_BRIDGE, ROAD_SKIP, ROAD_FAIL };
    struct RoadStep {
        sf::Vector2i p;
        RoadAction action = ROAD_FAIL;
        int axis = -1;       // bridge axis (see RoadTile)
        int cost = 0;
        std::string reason;
    };
    std::vector<RoadStep> planRoadPath(const std::vector<sf::Vector2i>& tiles, BuildTool tool) const;
    // Lays a road path: land roads, upgrades and bridges. Returns the first failure reason ("" if none).
    std::string buildRoadPath(const std::vector<sf::Vector2i>& tiles, BuildTool tool);
    void refreshBuildingsAround(int x, int y);
    int buildBenchCity();
    int buildBenchTown();
    void connectBenchRoads(int x0, int x1, int y0);
    void buildStarterShowcase();
    // Grows tier-1 buildings on zoned lots with road access while demand lasts (daily)
    int growZones();
    // Places a utility on the first buildable lot next to the road row y (x from x0 stepping dx)
    bool placeUtilityNear(BuildTool tool, int x0, int x1, int y);
    // Places a 2x2 on the free road-side lot closest to (cx, cy) within r tiles
    bool placeNearest(BuildTool tool, int cx, int cy, int r);
    // Places a utility away from the road at root (rx, ry) and runs a line
    // south-west from its border until it meets a road
    bool placeLinkedUtility(BuildTool tool, int rx, int ry, BuildTool lineTool);
    void markUtilitiesDirty() { utilitiesDirty = true; }

    // ---- Save / load (SaveGame.hpp) ----
    bool saveCity(const std::string& path);
    bool loadCity(const std::string& path);
    bool writeCity(std::ostream& out);
    bool readCity(std::istream& in, bool keepTime);
    std::string loadError;   // why the last load failed
    bool loadFail(const std::string& why) { loadError = why; return false; }
    Zone* createStructure(const std::string& name, int x, int y, int tier, int variant, int extra);
    void onClose() override;
    bool autosaveAllowed() const { return !headless && benchSeconds <= 0.f && fastForwardDays <= 0; }

    // ---- Undo (Ctrl+Z): city snapshots taken before each build action ----
    // A build step stores a city snapshot; a tax step only the old tax rates
    struct UndoStep { std::string state; int spent; bool taxOnly = false; int tax[SECTOR_COUNT][WEALTH_COUNT] = {}; };
    int taxBeforeDrag[SECTOR_COUNT][WEALTH_COUNT] = {};
    std::vector<UndoStep> undoStack;
    static const size_t UNDO_LIMIT = 10;
    bool undoLast();

    // ---- Messages ----
    std::deque<Notice> notices;
    void notify(const std::string& msg, bool isError);
    bool isLogOpen() const { return logOpen; }
    sf::FloatRect logChipBounds() const { return sf::FloatRect(uikit::SW() - 150.f, uikit::SH() - 58.f, 140.f, 24.f); }
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
    void renderUtilityIcons(Zone* z);
    void drawPylon(int x, int y);
    void renderUtilityLines(float viewL, float viewR, float viewT, float viewB, uint8_t line);
    void syncViewWithTool();
    void renderToast();
    void renderDragPreview();
    void renderHUD(float deltaTime);

    void refreshUI();
    void renderBudget(const sf::Vector2f& mouse);
    void renderGameOver(const sf::Vector2f& mouse);

    std::vector<sf::Vector2i> computeDragTiles() const;
    void executeMultibuild();
    void runMultibuild(BuildTool tool);
    void renderLog(const sf::Vector2f& mouse);
    void drawFire(Zone* z);
    void runServicesDay();
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
    if (benchTown || starterCity) {
        int built = buildBenchTown();
        std::cout << "\nBench town: " << built << " buildings\n";
    }
    if (starterCity) buildStarterShowcase();
    placementErrorTimer = 0.f; // no build-time toasts from the scripted towns
    if (!loadPath.empty()) {
        if (loadCity(loadPath)) {
            notify("Loaded " + loadPath, false);
        } else {
            notify("Could not load " + loadPath + " (" + loadError + ")", true);
            std::fprintf(stderr, "load failed: %s\n", loadError.c_str());
        }
    }
    utilities.refreshService();
    services.update();
    services.computeLandValue(happinessSystem.parkMap, happinessSystem.educationMap, happinessSystem.commerceMap,
                              happinessSystem.pollutionMap, traffic.congestion);
    utilitiesDirty = false;
    traffic.update();
    happinessSystem.updateSimulation();

    // Initialize TheoTown bottom dock
    toolbar.init(uikit::SW(), uikit::SH());

    // Screenshot helpers (see --menu / --inspect / --view)
    if (startMenu >= 1 && startMenu <= 7) toolbar.openFlyout(static_cast<ToolbarCategory>(startMenu));
    if (inWorldBounds(startInspectX, startInspectY)) { tileX = startInspectX; tileY = startInspectY; }
    setOverlayMode(startView);
    if (startBudget) setBudgetOpen(true);
    if (startLog) logOpen = true;

    // Balance testing: simulate N days instantly and print the trend (--fast-forward N)
    if (fastForwardDays > 0) {
        for (int d = 1; d <= fastForwardDays && !economy.bankrupt; d++) {
            advanceTime(GameClock::SECONDS_PER_DAY);
            if (d % 30 == 0 || d == fastForwardDays) {
                std::printf("day %4d  money %8d  net/day %6d  pop R %d/%d/%d  jobs C %d/%d/%d I %d/%d/%d  "
                            "demand R %+.0f/%+.0f/%+.0f C %+.0f/%+.0f/%+.0f I %+.0f/%+.0f/%+.0f  happy %d  debt %d  traffic %.2f  "
                            "power %.0f/%.0f  water %.0f/%.0f (untreated %.0f, stored %.0f)  unserved %d\n",
                            d, money, lastDailyIncome,
                            economy.occupants[0][0], economy.occupants[0][1], economy.occupants[0][2],
                            economy.occupants[1][0], economy.occupants[1][1], economy.occupants[1][2],
                            economy.occupants[2][0], economy.occupants[2][1], economy.occupants[2][2],
                            economy.demand[0][0], economy.demand[0][1], economy.demand[0][2],
                            economy.demand[1][0], economy.demand[1][1], economy.demand[1][2],
                            economy.demand[2][0], economy.demand[2][1], economy.demand[2][2],
                            happinessSystem.cityHappiness, economy.daysInDebt, traffic.cityCongestion,
                            utilities.powerDelivered, utilities.powerDemand, utilities.waterDelivered, utilities.waterDemand,
                            utilities.waterUntreated, utilities.waterStored,
                            utilities.grace ? 0 : std::max(utilities.unpowered, utilities.unwatered));
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
    for (auto& col : world) for (TileData& t : col) { t.lines = 0; t.zone = 0; }
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
    roadNet.clear();
    traffic.reset();
    cars.clear();
    utilities.reset();
    services.reset();
    utilitiesDirty = true;
    shortageToastDay = -100;
    undoStack.clear();
    notices.clear();
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
    bool overLog = logChipBounds().contains(mousePixelF) || (logOpen && sf::FloatRect(uikit::SW() - 432.f, uikit::SH() - 420.f, 420.f, 356.f).contains(mousePixelF));
    bool mouseOverUI = !focused || economy.bankrupt || toolbar.contains(mousePixelF) || overViewPanel || overLog ||
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
        if (logChipBounds().contains(mousePixelF)) {
            logOpen = !logOpen;
        } else if (overLog) {
            // clicks on the open log do nothing
        } else if (overViewPanel) {
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

    syncViewWithTool();

    // 2. Render Passes
    _window.clear(sf::Color(15, 20, 25));
    renderWorld(deltaTime);
    renderHUD(deltaTime);
    _window.display();

    // 3. Game time: days, economy and simulation (scaled by speed, frozen when paused)
    advanceTime(deltaTime);
}

// Power tools show the Power view, water tools the Water view (that is
// where lines and pipes are visible); leaving them restores the old view
void Play_Scene::syncViewWithTool() {
    BuildTool tool = toolbar.getActiveTool();
    if (tool == lastTool) return;
    auto viewFor = [](BuildTool t) {
        if (t == TOOL_POWER_LINE || (isUtilityTool(t) && isPowerUtility(utilityToolKind(t)))) return 5;
        if (t == TOOL_WATER_PIPE || isUtilityTool(t)) return 6;
        return -1;
    };
    int before = viewFor(lastTool), after = viewFor(tool);
    if (after >= 0) {
        if (before < 0) viewBeforeUtilityTool = overlayMode;
        overlayMode = after;
    } else if (before >= 0) {
        overlayMode = viewBeforeUtilityTool;
    }
    lastTool = tool;
}

void Play_Scene::advanceTime(float realSeconds) {
    if (economy.bankrupt) { refreshUI(); return; } // the city is frozen on the game-over screen
    if (utilitiesDirty) {
        utilities.refreshService();
        services.update();
        utilitiesDirty = false;
    }
    int newDays = gameClock.advance(realSeconds);
    for (int d = 0; d < newDays; d++) {
        runNewDay();
    }

    // One simulation tick per game second; at 4x that is 4 per real second.
    // Cap catch-up so a long stall cannot trigger a burst of ticks.
    const float gameSeconds = gameClock.scaledDelta(realSeconds);
    cars.update(gameSeconds);
    simAccumulator += gameSeconds;
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

// Fires, mines, garbage and land value, once a day
void Play_Scene::runServicesDay() {
    services.update();
    if (services.runDay(gameClock.getTotalDays(), townPopulation[SECTOR_R])) utilitiesDirty = true;
    for (const auto& e : services.events) {
        std::string what = e.name.substr(0, e.name.size() > 4 ? e.name.size() - 4 : e.name.size());   // "ResidentalZone" -> "Residental"
        if (what == "Residental") what = "Home";
        else if (what == "Commerical") what = "Shop";
        else if (what == "Industrial") what = "Factory";
        std::string where = " at " + std::to_string(e.x) + "," + std::to_string(e.y);
        if (e.kind == 3) { notify("Landfill" + where + " is full - build another, an incinerator or recycling", true); continue; }
        if (e.kind == 0) notify("Fire! " + what + where + " is burning - fire stations put fires out", true);
        else if (e.kind == 1) notify("Fire fighters put out the fire" + where, false);
        else notify(what + where + " burnt down", true);
    }
    services.computeLandValue(happinessSystem.parkMap, happinessSystem.educationMap, happinessSystem.commerceMap,
                              happinessSystem.pollutionMap, traffic.congestion);
}

void Play_Scene::notify(const std::string& msg, bool isError) {
    showToast(msg, isError);
    notices.push_front({gameClock.getTotalDays(), msg, isError});
    while (notices.size() > 40) notices.pop_back();
}

void Play_Scene::onClose() {
    if (autosaveAllowed() && !economy.bankrupt) saveCity("autosave.sav");
}

bool Play_Scene::undoLast() {
    if (undoStack.empty()) {
        showToast("Nothing to undo", true);
        return false;
    }
    UndoStep step = undoStack.back();
    undoStack.pop_back();
    if (step.taxOnly) {
        for (int s = 0; s < SECTOR_COUNT; s++) for (int w = 0; w < WEALTH_COUNT; w++) taxRates[s][w] = step.tax[s][w];
        showToast("Tax change undone", false);
        return true;
    }
    const int keep = money;
    std::istringstream in(step.state);
    if (!readCity(in, true)) {
        showToast("Undo failed", true);
        return false;
    }
    money = keep + step.spent;   // refund what the action cost
    utilitiesDirty = true;
    showToast("Undone (+" + uikit::formatMoney(step.spent) + ")", false);
    return true;
}

void Play_Scene::runNewDay() {
    growZones();      // new buildings on zoned land (from yesterday's demand)
    traffic.update(); // commutes on today's roads feed happiness and demand
    DayReport rep = economy.runDay(happinessSystem);
    lastDailyIncome = rep.net;

    // Debt warnings: the city may stay below $0 for at most 180 days
    int d = economy.daysInDebt;
    // Utility shortages: at most one warning a week
    const int today = static_cast<int>(gameClock.getTotalDays());
    if (!utilities.grace && today - shortageToastDay >= 7) {
        char buf[128];
        buf[0] = 0;
        if (utilities.unpowered > 0)
            std::snprintf(buf, sizeof(buf), "Power shortage: %d buildings dark (%.0f / %.0f power) - build plants or transformers",
                          utilities.unpowered, utilities.powerDelivered, utilities.powerDemand);
        else if (utilities.unwatered > 0)
            std::snprintf(buf, sizeof(buf), "Water shortage: %d buildings dry (%.0f / %.0f) - build wells or pumps",
                          utilities.unwatered, utilities.waterDelivered, utilities.waterDemand);
        else if (utilities.untreatedShare > 0.3f)
            std::snprintf(buf, sizeof(buf), "Untreated tap water is making people ill - build a treatment plant");
        else if (!utilities.sewageGrace && utilities.unsewered > 0)
            std::snprintf(buf, sizeof(buf), "%d buildings have no sewers - build a sewage outflow or treatment plant", utilities.unsewered);
        else if (!services.grace && services.unservedGarbage > 0)
            std::snprintf(buf, sizeof(buf), "Garbage piling up at %d buildings - build a landfill, incinerator or recycling", services.unservedGarbage);
        if (buf[0]) { notify(buf, true); shortageToastDay = today; }
    }

    if (d == 1) notify("Treasury in debt - bankrupt after 6 months in the red", true);
    else if (d == 90) notify("3 months of debt left - raise taxes or take a loan", true);
    else if (d == 150) notify("1 month left before bankruptcy!", true);

    runServicesDay();

    // Autosave once a month (not in benchmarks / tests)
    if (autosaveAllowed() && today > 0 && today % GameClock::DAYS_PER_MONTH == 0) saveCity("autosave.sav");

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
        toolbar.setActiveTool(TOOL_ZONE_R_LOW);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::C)) {
        toolbar.setActiveTool(TOOL_ZONE_C_LOW);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::I)) {
        toolbar.setActiveTool(TOOL_ZONE_I_LOW);
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
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::U)) {
        toolbar.setActiveTool(TOOL_POWER_COAL);
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

    // F5 save, F9 load (city.sav, else the autosave), Ctrl+Z / Z undo, L messages
    bool saveKey = sf::Keyboard::isKeyPressed(sf::Keyboard::F5);
    bool loadKey = sf::Keyboard::isKeyPressed(sf::Keyboard::F9);
    bool undoKey = sf::Keyboard::isKeyPressed(sf::Keyboard::Z);
    bool logKey = sf::Keyboard::isKeyPressed(sf::Keyboard::L);
    if (saveKey && !wasSaveKey) {
        if (saveCity("city.sav")) notify("City saved (city.sav) - F9 loads it", false);
        else notify("Could not save city.sav", true);
    }
    if (loadKey && !wasLoadKey) {
        std::ifstream probe("city.sav");
        const std::string path = probe ? "city.sav" : "autosave.sav";
        probe.close();
        if (loadCity(path)) { toolbar.setActiveTool(TOOL_NONE); notify("Loaded " + path, false); }
        else showToast("No save to load (F5 saves)", true);
    }
    if (undoKey && !wasUndoKey) undoLast();
    if (logKey && !wasLogKey) logOpen = !logOpen;
    wasSaveKey = saveKey;
    wasLoadKey = loadKey;
    wasUndoKey = undoKey;
    wasLogKey = logKey;

    // O cycles the map view (edge-triggered)
    bool overlayKey = sf::Keyboard::isKeyPressed(sf::Keyboard::O);
    if (overlayKey && !wasOverlayKeyPressed) {
        overlayMode = (overlayMode + 1) % OVERLAY_COUNT;
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

    if (isRoadTool(toolbar.getActiveTool()) || isLineTool(toolbar.getActiveTool())) {
        // Edge-connected L (see roadDragPath)
        for (const auto& p : roadDragPath(dragStart, dragCurrent))
            if (inWorldBounds(p.x, p.y)) tiles.push_back(p);
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

int Play_Scene::getPlacementCost(int x, int y, BuildTool tool) const {
    if (tool == TOOL_BULLDOZE) return getBulldozeCost(x, y);
    if (isRoadTool(tool)) {
        if (RoadTile* r = RoadTile::at(x, y)) {
            int diff = std::max(0, ROAD_COST[roadToolTier(tool)] - ROAD_COST[r->tier]);
            return r->isBridge() ? diff * BRIDGE_COST_MULT : diff;
        }
    }
    return getToolCost(tool);
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
        case 4: return "Traffic";
        case 5: return "Power";
        case 6: return "Water";
        case 7: return "Land value";
        default: return "Normal";
    }
}

// Map View panel: fixed screen-space buttons under the top bar (left side)
sf::FloatRect Play_Scene::viewPanelBounds() const {
    return sf::FloatRect(uikit::SW() - 404.f, 58.f, 392.f, 84.f);
}

sf::FloatRect Play_Scene::viewButtonBounds(int mode) const {
    sf::FloatRect panel = viewPanelBounds();
    return sf::FloatRect(panel.left + 8.f + static_cast<float>(mode) * 48.f, panel.top + 8.f, 40.f, 40.f);
}

bool Play_Scene::handleViewPanelClick(const sf::Vector2f& point) {
    for (int m = 0; m < OVERLAY_COUNT; m++) {
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

    int cost = spawningZoned ? 0 : getPlacementCost(x, y, tool);
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
        if (isClearGroundName(tileKind(x, y)) && world[x][y].lines == 0 && world[x][y].zone == 0) {
            if (outReason) *outReason = "Already Clear Ground";
            return false;
        }
        return true;
    }

    // Zones (rectangle drag): mark land for buildings to grow on
    if (isZoneTool(tool) || tool == TOOL_DEZONE) {
        if (!inWorldBounds(x, y)) return false;
        if (tool == TOOL_DEZONE) {
            if (world[x][y].zone == 0) {
                if (outReason) *outReason = "Already Unzoned";
                return false;
            }
            return true;
        }
        const std::string name = tileKind(x, y);
        if (world[x][y].zone == zoneToolValue(tool)) {
            if (outReason) *outReason = "Already Zoned";
            return false;
        }
        if (name == "Water" || RoadTile::at(x, y) != NULL) {
            if (outReason) *outReason = "Already a road or water - cannot zone";
            return false;
        }
        if (!terrainBlockReason(name).empty()) {
            if (outReason) *outReason = terrainBlockReason(name);
            return false;
        }
        return true;
    }

    // Power lines (overground) and pipes (underground): 1x1, dragged
    if (isLineTool(tool)) {
        if (!inWorldBounds(x, y)) return false;
        const uint8_t bit = lineToolBit(tool);
        const std::string name = tileKind(x, y);
        if (world[x][y].lines & bit) {
            if (outReason) *outReason = tool == TOOL_POWER_LINE ? "Already a Power Line" : "Already a Pipe";
            return false;
        }
        if (RoadTile::at(x, y) != NULL) {
            if (outReason) *outReason = "Roads Already Carry Power and Water";
            return false;
        }
        if (name == "Water") {
            if (outReason) *outReason = "Lines Cannot Cross Water - use a road bridge";
            return false;
        }
        if (!terrainBlockReason(name).empty()) {
            if (outReason) *outReason = terrainBlockReason(name);
            return false;
        }
        // Pipes run under buildings; power lines need open ground
        if (tool == TOOL_POWER_LINE && !isClearGroundName(name)) {
            if (outReason) *outReason = "Blocked by Building - Bulldoze First";
            return false;
        }
        return true;
    }

    // 2. Road Tools (1x1): build on clear ground, or upgrade a lower-tier road
    if (isRoadTool(tool)) {
        if (x < 0 || x >= tileMapW || y < 0 || y >= tileMapH) return false;
        std::string name = tileKind(x, y);
        if (RoadTile* r = RoadTile::at(x, y)) {
            if (r->tier >= roadToolTier(tool)) {
                if (outReason) *outReason = "Road Already Built";
                return false;
            }
            return true; // upgrade
        }
        if (name == "Water") {
            if (outReason) *outReason = "Cannot Build Road on Water - drag straight across for a bridge (max " +
                                        std::to_string(BRIDGE_MAX_SPAN) + ")";
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

    // Roundabout: replaces a 3- or 4-way junction on land
    if (tool == TOOL_ROUNDABOUT) {
        RoadTile* r = RoadTile::at(x, y);
        if (r == NULL) {
            if (outReason) *outReason = "Roundabouts Go on a Road Junction";
            return false;
        }
        if (r->roundabout) {
            if (outReason) *outReason = "Already a Roundabout";
            return false;
        }
        int links = 0;
        for (int d = 0; d < 4; d++) links += (r->mask >> d) & 1;
        if (r->isBridge() || links < 3) {
            if (outReason) *outReason = "Roundabout Needs a 3- or 4-way Junction";
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
        if (isRoadName(name)) {
            if (outReason) *outReason = "Blocked by Road!";
            return false;
        }
        if (name == "Water") {
            if (outReason) *outReason = "Blocked by Water!";
            return false;
        }
        if (hasLine(cx, cy, LINE_POWER)) {
            if (outReason) *outReason = "Power Line in the Way - Bulldoze First";
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

    // Sewage outflows empty into a lake, river or the sea
    if (tool == TOOL_SEWAGE_OUTFLOW && !touchesWater) {
        if (outReason) *outReason = "Sewage Outflow Must Touch Water";
        return false;
    }
    // Power & water buildings connect through roads or lines, so a road is optional
    if (isUtilityTool(tool)) return true;

    // Mines dig into a coal / iron / stone deposit under the footprint
    if (tool == TOOL_MINE) {
        bool deposit = false;
        for (int k = 0; k < 4; k++) {
            int cx = x + diamondOffsets[k].x, cy = y + diamondOffsets[k].y;
            if (world[cx][cy].deposit != Deposit::None && world[cx][cy].depositAmount > 0) deposit = true;
        }
        if (!deposit) {
            if (outReason) *outReason = "Mines Need a Coal, Iron or Stone Deposit (Deposits view)";
            return false;
        }
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

    int cost = spawningZoned ? 0 : getPlacementCost(x, y, tool);

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
        bool wasRoad = isRoadName(tileKind(x, y));
        utilitiesDirty = true;
        // Bare ground with lines: the power line goes first, then the pipe, then the zone
        if (entities[x][y] == NULL && (world[x][y].lines != 0 || world[x][y].zone != 0) && isClearGroundName(tileKind(x, y))) {
            uint8_t& l = world[x][y].lines;
            if (l != 0) l &= (l & LINE_POWER) ? static_cast<uint8_t>(~LINE_POWER) : static_cast<uint8_t>(~LINE_PIPE);
            else world[x][y].zone = 0;
            money -= cost;
            return;
        }
        demolishAt(x, y);
        if (wasRoad) onRoadsChanged(x, y);
        else refreshBuildingsAround(x, y); // a removed water tile can disconnect a harbour
        money -= cost;
        return;
    }

    if (isRoadTool(tool)) {
        if (RoadTile* r = RoadTile::at(x, y)) {
            r->setTier(roadToolTier(tool)); // upgrade in place
        } else {
            delete entities[x][y];
            entities[x][y] = new RoadTile(x, y, roadToolTier(tool));
        }
        onRoadsChanged(x, y);
        money -= cost;
        return;
    }

    if (tool == TOOL_ROUNDABOUT) {
        RoadTile* r = RoadTile::at(x, y);
        r->roundabout = true;
        r->check();
        money -= cost;
        return;
    }

    if (isZoneTool(tool) || tool == TOOL_DEZONE) {
        world[x][y].zone = zoneToolValue(tool);   // 0 for dezone; buildings stay
        money -= cost;
        return;
    }

    if (isLineTool(tool)) {
        world[x][y].lines |= lineToolBit(tool);
        utilitiesDirty = true;
        money -= cost;
        return;
    }

    if (tool == TOOL_WATER) {
        delete entities[x][y];
        entities[x][y] = NULL;
        world[x][y].lines = 0;   // flooding removes lines and pipes
        utilitiesDirty = true;
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
            if (isServiceTool(tool)) {
                masterZone = new ServiceZone(x, y, serviceToolKind(tool));
                zoneBase = "grass";
                typeName = serviceEntityName(serviceToolKind(tool));
                break;
            }
            if (!isUtilityTool(tool)) return;
            masterZone = new UtilityZone(x, y, utilityToolKind(tool));
            zoneBase = "grass";
            typeName = "UtilityZone";
            break;
    }

    // Placed by hand: keeps its tier. Grown on a zone: growZones sets the rules.
    masterZone->fixedTier = !spawningZoned;
    entities[x][y] = masterZone;
    entities[x + 1][y]     = new SubZone(x + 1, y,     x, y, typeName, zoneBase);
    entities[x - 1][y + 1] = new SubZone(x - 1, y + 1, x, y, typeName, zoneBase);
    entities[x][y + 1]     = new SubZone(x,     y + 1, x, y, typeName, zoneBase);
    masterZone->checkValid();
    utilitiesDirty = true;

    // Tell the player at once when a new utility cannot work yet
    if (UtilityZone* u = dynamic_cast<UtilityZone*>(masterZone)) {
        utilities.refreshService();
        utilitiesDirty = false;
        if (!u->linked)
            showToast(std::string(utilityTitle(u->kind)) + " is not connected - join it to a road or " +
                      (isPowerUtility(u->kind) ? "power line" : "water pipe"), false);
        else if (u->noTransformer)
            showToast("Power plants need a Transformer on their grid to deliver power", false);
    }
}

void Play_Scene::executeMultibuild() {
    BuildTool tool = toolbar.getActiveTool();
    if (tool == TOOL_NONE) return;
    // Snapshot for undo; kept only if the action changed something
    std::ostringstream before;
    writeCity(before);
    const int moneyBefore = money;
    runMultibuild(tool);
    std::ostringstream after;
    writeCity(after);
    if (after.str() != before.str()) {
        undoStack.push_back({before.str(), moneyBefore - money});
        if (undoStack.size() > UNDO_LIMIT) undoStack.erase(undoStack.begin());
    }
}

void Play_Scene::runMultibuild(BuildTool tool) {

    if (isRoadTool(tool)) {
        std::string reason = buildRoadPath(computeDragTiles(), tool);
        if (!reason.empty()) showToast("Can't build here: " + reason, true);
    } else if (isLineTool(tool) || isZoneTool(tool) || tool == TOOL_DEZONE) {
        // Tiles that already carry this line / zone (or are roads, water,
        // forest for zones) are skipped quietly; only real problems toast
        std::string firstFail;
        for (const auto& pt : computeDragTiles()) {
            std::string reason;
            if (canPlaceTile(pt.x, pt.y, tool, &reason)) placeTile(pt.x, pt.y, tool);
            else if (firstFail.empty() && reason.rfind("Already", 0) != 0 && reason.rfind("Roads Already", 0) != 0 &&
                     (!isZoneTool(tool) || reason.rfind("Insufficient", 0) == 0))
                firstFail = reason;
        }
        if (!firstFail.empty()) showToast("Can't build here: " + firstFail, true);
    } else if (tool == TOOL_WATER || tool == TOOL_BULLDOZE) {
        // One toast for the first tile that fails, not one per tile
        std::string firstFail;
        for (const auto& pt : computeDragTiles()) {
            std::string reason;
            if (canPlaceTile(pt.x, pt.y, tool, &reason)) placeTile(pt.x, pt.y, tool);
            else if (firstFail.empty()) firstFail = reason;
        }
        if (!firstFail.empty()) showToast("Can't build here: " + firstFail, true);
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
    connectBenchRoads(x0, x1, y0);

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
    if (!starterCity) {
        // Enough power and water for the town to pass UTILITY_GRACE_POP
        for (BuildTool t : {TOOL_TRANSFORMER, TOOL_POWER_WIND, TOOL_POWER_WIND, TOOL_POWER_WIND,
                            TOOL_WATER_WELL, TOOL_WATER_WELL, TOOL_WATER_WELL, TOOL_WATER_WELL, TOOL_LANDFILL})
            placeUtilityNear(t, x0, x1, y0);
    }
    money = savedMoney;
    return built;
}

// Bench town roads: joins the two rows east of the town with a NE-SW link,
// then runs a road out to the closest map edge (cheapest of four straight
// routes, clearing forest / rock and bridging short water on the way).
void Play_Scene::connectBenchRoads(int x0, int x1, int y0) {
    int ex = std::min(tileMapW - 1, x1 + 3);
    std::vector<sf::Vector2i> link;
    for (int x = x1 + 1; x <= ex; x++) link.push_back({x, y0});
    for (int k = 1; k <= 3 && x1 + 3 - k >= 0; k++) link.push_back({x1 + 3 - k, y0 + k});
    for (const auto& p : link)
        if (inWorldBounds(p.x, p.y) && isNaturalObstacleName(tileKind(p.x, p.y)) && tileKind(p.x, p.y) != "Water")
            world[p.x][p.y].terrain = Terrain::Grass;
    ground.markDirty();
    buildRoadPath(link, TOOL_ROAD);

    // Candidate straight routes to an edge: west, east, north-east, south-west
    std::vector<std::vector<sf::Vector2i>> routes(4);
    for (int x = x0; x >= 0; x--) routes[0].push_back({x, y0});
    for (int x = ex; x < tileMapW; x++) routes[1].push_back({x, y0});
    for (int x = ex, y = y0; x < tileMapW && y >= 0; x++, y--) routes[2].push_back({x, y});
    for (int x = x0, y = y0 + 3; x >= 0 && y < tileMapH; x--, y++) routes[3].push_back({x, y});
    auto score = [&](const std::vector<sf::Vector2i>& r) {
        int water = 0, s = 0;
        for (const auto& p : r) {
            std::string k = tileKind(p.x, p.y);
            if (k == "Water") { water++; s += 20; }
            else s += 1;
        }
        bool endsInWater = !r.empty() && tileKind(r.back().x, r.back().y) == "Water";
        return (water > BRIDGE_MAX_SPAN || endsInWater) ? 1 << 30 : s;
    };
    int best = 0;
    for (int r = 1; r < 4; r++)
        if (score(routes[r]) < score(routes[best])) best = r;
    for (const auto& p : routes[best]) {
        std::string k = tileKind(p.x, p.y);
        if (k == "Forest" || k == "Rock") world[p.x][p.y].terrain = Terrain::Grass;
    }
    ground.markDirty();
    buildRoadPath(routes[best], TOOL_ROAD);
}

// --starter: dresses the bench town up to show every road feature, then
// runs 60 days so homes fill and traffic / cars appear. The main street
// becomes asphalt, the east link an avenue, and a short NE spur off the
// middle of the main street makes a 3-way junction that gets a roundabout.
void Play_Scene::buildStarterShowcase() {
    const int x0 = std::max(2, worldSpawnX - 9), x1 = std::min(tileMapW - 3, worldSpawnX + 9);
    const int y0 = std::max(1, worldSpawnY - 4);
    const int savedMoney = money;
    money = 100000000;

    std::vector<sf::Vector2i> mainStreet, link;
    for (int x = x0; x <= x1; x++) mainStreet.push_back({x, y0});
    buildRoadPath(mainStreet, TOOL_ROAD_ASPHALT);
    const int ex = std::min(tileMapW - 1, x1 + 3);
    for (int x = x1 + 1; x <= ex; x++) link.push_back({x, y0});
    for (int k = 1; k <= 3; k++) link.push_back({x1 + 3 - k, y0 + k});
    buildRoadPath(link, TOOL_ROAD_AVENUE);

    // NE spur from the middle of the main street, then a roundabout on the junction
    const int mx = (x0 + x1) / 2;
    std::vector<sf::Vector2i> spur;
    for (int k = 1; k <= 8 && inWorldBounds(mx + k, y0 - k); k++) spur.push_back({mx + k, y0 - k});
    for (const auto& p : spur)
        if (isNaturalObstacleName(tileKind(p.x, p.y)) && tileKind(p.x, p.y) != "Water") world[p.x][p.y].terrain = Terrain::Grass;
    ground.markDirty();
    buildRoadPath(spur, TOOL_ROAD_ASPHALT);
    if (canPlaceTile(mx, y0, TOOL_ROUNDABOUT)) placeTile(mx, y0, TOOL_ROUNDABOUT);

    // Off the road: a wind farm on a power line and a well on a pipe
    placeLinkedUtility(TOOL_POWER_WIND, x0 + 7, y0 - 7, TOOL_POWER_LINE);
    placeLinkedUtility(TOOL_WATER_WELL, x1 - 2, y0 - 11, TOOL_WATER_PIPE);
    // On the road: coal plant + transformer, an underground pump with a
    // treatment plant, and a water tower
    for (BuildTool t : {TOOL_TRANSFORMER, TOOL_TRANSFORMER, TOOL_POWER_COAL, TOOL_WATER_PUMP, TOOL_WATER_TREATMENT, TOOL_WATER_TOWER,
                        TOOL_FIRE_STATION, TOOL_POLICE})
        placeUtilityNear(t, x0, x1, y0);
    // Clinic, landfill and bus depot along the spur
    for (BuildTool t : {TOOL_CLINIC, TOOL_BUS_DEPOT, TOOL_LANDFILL, TOOL_RECYCLING, TOOL_SEWAGE_PLANT, TOOL_INCINERATOR})
        placeNearest(t, mx + 4, y0 - 4, 12);

    // Zones along the spur: medium-density homes on its north-west side,
    // low-density shops on its south-east side (spur tiles have x + y = mx + y0)
    for (int x = mx - 4; x <= mx + 11; x++) {
        for (int y = y0 - 11; y <= y0 - 3; y++) {
            if (!inWorldBounds(x, y) || x + y == mx + y0) continue;
            const std::string k = tileKind(x, y);
            if (k == "Forest" || k == "Rock") world[x][y].terrain = Terrain::Grass;
            BuildTool t = (x + y < mx + y0) ? TOOL_ZONE_R_MED : TOOL_ZONE_C_LOW;
            if (canPlaceTile(x, y, t)) placeTile(x, y, t);
        }
    }
    ground.markDirty();

    // A showcase, not a balanced start: the full road and utility set costs
    // far more upkeep than 18 buildings pay, so it gets a sandbox treasury
    (void)savedMoney;
    money = STARTER_SANDBOX_MONEY;
    utilities.refreshService();
    traffic.update();
    for (int d = 0; d < 60 && !economy.bankrupt; d++) advanceTime(GameClock::SECONDS_PER_DAY);
    std::printf("Starter city: pop %d, money %d, traffic %.2f, roundabout at %d,%d\n",
                townPopulation[0], money, traffic.cityCongestion, mx, y0);
    for (int i = 0; i < tileMapW; i++)   // positions for --inspect screenshots
        for (int j = 0; j < tileMapH; j++)
            if (UtilityZone* u = dynamic_cast<UtilityZone*>(entities[i][j])) {
                if (!u->isSubTile) std::printf("  %s at %d,%d\n", utilityTitle(u->kind), i, j);
            } else if (ServiceZone* sv = dynamic_cast<ServiceZone*>(entities[i][j])) {
                if (!sv->isSubTile) std::printf("  %s at %d,%d\n", serviceTitle(sv->kind), i, j);
            }
}

// Puts a utility on the north side of road row y (roots (x, y - 2), which
// border the row). Water sources take the spot with the most groundwater,
// everything else the westernmost free lot. Clears forest / rock first.
bool Play_Scene::placeUtilityNear(BuildTool tool, int x0, int x1, int y) {
    const int kind = utilityToolKind(tool);
    const bool wantsWater = kind == UTIL_WELL || kind == UTIL_PUMP;
    int bestX = -1;
    float bestScore = -1e9f;
    for (int x = x0 + 1; x <= x1; x++) {
        const int ry = y - 2;
        bool clear = inWorldBounds(x - 1, ry) && inWorldBounds(x + 1, ry + 1);
        for (const auto& o : FOOTPRINT_2X2) {
            if (!clear) break;
            const std::string k = tileKind(x + o[0], ry + o[1]);
            clear = (isClearGroundName(k) || k == "Forest" || k == "Rock") && !hasLine(x + o[0], ry + o[1], LINE_POWER);
        }
        if (!clear) continue;
        // Wells / pumps: most groundwater; transformers: middle of the row; the rest: west first
        float score = wantsWater ? UtilityZone::groundwaterAround(x, ry, kind == UTIL_WELL ? WELL_RADIUS : PUMP_RADIUS)
                    : kind == UTIL_TRANSFORMER ? -static_cast<float>(std::abs(2 * x - x0 - x1))
                    : -static_cast<float>(x);
        if (score > bestScore) { bestScore = score; bestX = x; }
    }
    if (bestX < 0) return false;
    for (const auto& o : FOOTPRINT_2X2) {
        int tx = bestX + o[0], ty = y - 2 + o[1];
        if (isNaturalObstacleName(tileKind(tx, ty)) && tileKind(tx, ty) != "Water") world[tx][ty].terrain = Terrain::Grass;
    }
    ground.markDirty();
    if (!canPlaceTile(bestX, y - 2, tool)) return false;
    placeTile(bestX, y - 2, tool);
    return true;
}

bool Play_Scene::placeNearest(BuildTool tool, int cx, int cy, int r) {
    int best = 1 << 30, bx = -1, by = -1;
    for (int x = cx - r; x <= cx + r; x++)
        for (int y = cy - r; y <= cy + r; y++) {
            int d = (x - cx) * (x - cx) + (y - cy) * (y - cy);
            if (d < best && inWorldBounds(x, y) && canPlaceTile(x, y, tool)) { best = d; bx = x; by = y; }
        }
    if (bx < 0) return false;
    placeTile(bx, by, tool);
    return true;
}

bool Play_Scene::placeLinkedUtility(BuildTool tool, int rx, int ry, BuildTool lineTool) {
    for (const auto& o : FOOTPRINT_2X2) {
        int tx = rx + o[0], ty = ry + o[1];
        if (!inWorldBounds(tx, ty)) return false;
        if (isNaturalObstacleName(tileKind(tx, ty)) && tileKind(tx, ty) != "Water") world[tx][ty].terrain = Terrain::Grass;
    }
    ground.markDirty();
    if (!canPlaceTile(rx, ry, tool)) return false;
    placeTile(rx, ry, tool);

    // Shortest edge-connected line from the building's border to any road
    // (BFS; forest / rock on the way is cleared, water and blocked tiles avoided)
    const uint8_t bit = lineToolBit(lineTool);
    auto passable = [&](int x, int y) {
        if (!inWorldBounds(x, y) || hasLine(x, y, bit)) return inWorldBounds(x, y);
        const std::string k = tileKind(x, y);
        if (k == "Water") return false;
        if (k == "Forest" || k == "Rock") return true;
        return canPlaceTile(x, y, lineTool);
    };
    static sf::Vector2i from[tileMapW][tileMapH];
    static bool seen[tileMapW][tileMapH];
    for (auto& col : seen) for (bool& b : col) b = false;
    std::vector<sf::Vector2i> queue;
    for (const auto& o : BORDER_2X2) {
        int bx = rx + o[0], by = ry + o[1];
        if (inWorldBounds(bx, by) && !seen[bx][by]) {
            seen[bx][by] = true;
            from[bx][by] = {-1, -1};
            queue.push_back({bx, by});
        }
    }
    for (size_t head = 0; head < queue.size(); head++) {
        sf::Vector2i p = queue[head];
        if (RoadTile::at(p.x, p.y) != NULL) {
            // Lay the line back from the road to the building
            for (sf::Vector2i c = from[p.x][p.y]; c.x >= 0; c = from[c.x][c.y]) {
                std::string k = tileKind(c.x, c.y);
                if (k == "Forest" || k == "Rock") world[c.x][c.y].terrain = Terrain::Grass;
                if (canPlaceTile(c.x, c.y, lineTool)) placeTile(c.x, c.y, lineTool);
            }
            ground.markDirty();
            return true;
        }
        if (!passable(p.x, p.y) || queue.size() > 2000) continue;
        for (const auto& d : ROAD_DIR) {
            int nx = p.x + d[0], ny = p.y + d[1];
            if (!inWorldBounds(nx, ny) || seen[nx][ny]) continue;
            seen[nx][ny] = true;
            from[nx][ny] = p;
            queue.push_back({nx, ny});
        }
    }
    return false;
}

// Daily: for each sector with low-income demand, grow up to 1 + demand / 30
// (max 3) tier-1 buildings on lots whose 4 tiles carry the same zone, are
// empty clear ground without power lines, and touch a road. A position /
// day hash skips two of three lots on the first pass so growth spreads
// out; a second pass fills the rest of the budget. Returns buildings grown.
int Play_Scene::growZones() {
    static const BuildTool byTier[SECTOR_COUNT][3] = {{TOOL_RES_COTTAGES, TOOL_RES_APARTMENTS, TOOL_RES_TOWER},
                                                      {TOOL_COM_GROCERY, TOOL_COM_PLAZA, TOOL_COM_TOWER},
                                                      {TOOL_IND_WORKSHOP, TOOL_IND_FACTORY, TOOL_IND_TECH}};
    const unsigned day = static_cast<unsigned>(gameClock.getTotalDays());
    int grown = 0;
    for (int s = 0; s < SECTOR_COUNT; s++) {
        float d = 0.f;
        for (int w = 0; w < WEALTH_COUNT; w++) d = std::max(d, economy.demand[s][w]);
        if (d <= 0.f) continue;
        int budget = std::min(ZONE_GROWTH_MAX_PER_DAY, 1 + static_cast<int>(d) / ZONE_GROWTH_DEMAND_STEP);
        for (int pass = 0; pass < 2 && budget > 0; pass++) {
            for (int x = 1; x + 1 < tileMapW && budget > 0; x++) {
                for (int y = 0; y + 1 < tileMapH && budget > 0; y++) {
                    const uint8_t zv = world[x][y].zone;
                    if (zoneSector(zv) != s) continue;
                    bool lot = true;
                    for (const auto& o : FOOTPRINT_2X2) {
                        int tx = x + o[0], ty = y + o[1];
                        if (world[tx][ty].zone != zv || entities[tx][ty] != NULL || !isClearGroundName(tileKind(tx, ty)) ||
                            hasLine(tx, ty, LINE_POWER)) { lot = false; break; }
                    }
                    if (!lot || !hasAdjacentRoad(x, y)) continue;
                    unsigned h = static_cast<unsigned>(x) * 73856093u ^ static_cast<unsigned>(y) * 19349663u ^ (day + 1u) * 83492791u;
                    if (pass == 0 && h % 3 != 0) continue;
                    // Grow the richest income level with demand that the density
                    // and the land value allow
                    int tier = 0;
                    for (int w = std::min(2, zoneMaxTier(zv) - 1); w >= 0; w--) {
                        const int lv = world[x][y].landValue;
                        bool landOk = w == 0 || (w == 1 ? lv >= LAND_VALUE_TIER2 : lv >= LAND_VALUE_TIER3);
                        if (economy.demand[s][w] > 0.f && landOk) { tier = w + 1; break; }
                    }
                    if (tier == 0) continue;

                    spawningZoned = true;
                    placeTile(x, y, byTier[s][tier - 1]);
                    spawningZoned = false;
                    Zone* z = dynamic_cast<Zone*>(entities[x][y]);
                    if (z == NULL) continue;
                    z->zoned = true;
                    z->fixedTier = false;
                    z->maxTier = zoneMaxTier(zv);
                    z->variant = 1 + static_cast<int>((h >> 4) % 2);
                    if (auto* r = dynamic_cast<ResidentalZone*>(z)) r->updateBuildingTexture();
                    else if (auto* c = dynamic_cast<CommericalZone*>(z)) c->updateBuildingTexture();
                    else if (auto* i = dynamic_cast<IndustrialZone*>(z)) i->updateBuildingTexture();
                    budget--;
                    grown++;
                }
            }
        }
    }
    return grown;
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
            RoadTile* road = dynamic_cast<RoadTile*>(entities[i][j]);
            if (road != NULL) road->check();
        }
    }
}

void Play_Scene::onRoadsChanged(int x, int y) {
    refreshRoadsAround(x, y);     // the tile and every road touching it
    refreshBuildingsAround(x, y); // access for lots on its border
    roadNet.rebuild();            // components and outside connection
    utilitiesDirty = true;        // utilities travel along roads
}

// Classifies every tile of a road drag. Land tiles build or upgrade; runs
// of water become a bridge when they are crossed in a straight line, land
// to land, over at most BRIDGE_MAX_SPAN tiles. Costs accumulate so the
// preview turns red where the money runs out.
std::vector<Play_Scene::RoadStep> Play_Scene::planRoadPath(const std::vector<sf::Vector2i>& tiles, BuildTool tool) const {
    const int tier = roadToolTier(tool);
    const int n = static_cast<int>(tiles.size());
    std::vector<RoadStep> steps(static_cast<size_t>(n));
    auto isWater = [&](int k) { return tileKind(tiles[k].x, tiles[k].y) == "Water"; };
    // Edge axis of the step from tile k to k+1: 1 = SE-NW (+-1, 0), 0 = NE-SW (+-1, -+1)
    auto stepAxis = [&](int k) { return tiles[k + 1].y == tiles[k].y ? 1 : 0; };

    for (int k = 0; k < n; k++) {
        RoadStep& st = steps[k];
        st.p = tiles[k];
        if (isWater(k)) continue; // decided per run below
        std::string reason;
        if (RoadTile* r = RoadTile::at(st.p.x, st.p.y)) {
            st.action = (r->tier >= tier) ? ROAD_SKIP : ROAD_UPGRADE;
        } else if (canPlaceTile(st.p.x, st.p.y, tool, &reason)) {
            st.action = ROAD_BUILD;
        } else {
            st.action = ROAD_FAIL;
            st.reason = reason;
        }
        if (st.action == ROAD_BUILD || st.action == ROAD_UPGRADE) st.cost = getPlacementCost(st.p.x, st.p.y, tool);
    }

    for (int s = 0; s < n; s++) {
        if (!isWater(s)) continue;
        int e = s;
        while (e + 1 < n && isWater(e + 1)) e++;
        std::string why;
        int axis = n > 1 ? stepAxis(s > 0 ? s - 1 : s) : 1;
        if (economy.bankrupt) why = "City is bankrupt";
        else if (s == 0 || e == n - 1) why = "Bridges must start and end on land";
        else if (e - s + 1 > BRIDGE_MAX_SPAN) why = "Bridge too long (max " + std::to_string(BRIDGE_MAX_SPAN) + " tiles)";
        else if (steps[s - 1].action == ROAD_FAIL || steps[e + 1].action == ROAD_FAIL) why = "Bridge needs buildable land at both ends";
        for (int k = s - 1; why.empty() && k <= e; k++) {
            if (stepAxis(k) != axis) why = "Bridges must run straight";
        }
        for (int k = s; k <= e; k++) {
            steps[k].action = why.empty() ? ROAD_BRIDGE : ROAD_FAIL;
            steps[k].axis = axis;
            steps[k].reason = why;
            if (why.empty()) steps[k].cost = ROAD_COST[tier] * BRIDGE_COST_MULT;
        }
        s = e;
    }

    int total = 0;
    for (RoadStep& st : steps) {
        if (st.action == ROAD_SKIP || st.action == ROAD_FAIL) continue;
        total += st.cost;
        if (total > money) {
            st.action = ROAD_FAIL;
            st.reason = "Insufficient Funds ($" + std::to_string(total) + " needed)";
        }
    }
    return steps;
}

std::string Play_Scene::buildRoadPath(const std::vector<sf::Vector2i>& tiles, BuildTool tool) {
    std::string firstFail;
    for (const RoadStep& st : planRoadPath(tiles, tool)) {
        const int x = st.p.x, y = st.p.y;
        if (st.action == ROAD_BUILD || st.action == ROAD_UPGRADE) {
            placeTile(x, y, tool);
        } else if (st.action == ROAD_BRIDGE) {
            delete entities[x][y];
            entities[x][y] = new RoadTile(x, y, roadToolTier(tool), st.axis);
            money -= st.cost;
            onRoadsChanged(x, y);
        } else if (st.action == ROAD_FAIL && firstFail.empty()) {
            firstFail = st.reason;
        }
    }
    return firstFail;
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
                    if (z->sprite != NULL && !isRoadName(z->getName())) {
                        z->sprite->setColor(ground.getSeasonLook().grass);
                    }
                    z->drawGround(_window);
                } else {
                    entities[i][j]->draw();
                }
            }
        }
    }

    // PASS 1b2: Painted zones that nothing has grown on yet (stronger = denser)
    {
        static const char* zoneTex[3] = {"zone_residental", "zone_commercial", "zone_industrial"};
        static const sf::Uint8 densityAlpha[3] = {110, 160, 210};
        sf::Sprite zs;
        zs.setScale(2.f, 2.f);
        for (int i = vis.iMin; i <= vis.iMax; i++) {
            for (int j = vis.jMin(i); j <= vis.jMax(i); j++) {
                const uint8_t zv = world[i][j].zone;
                if (zv == 0 || entities[i][j] != NULL) continue;
                sf::Vector2f tp = tileScreenPos(i, j);
                if (!isVisible(tp.x, tp.y)) continue;
                zs.setTexture(m_textures[zoneTex[zoneSector(zv)]], true);
                zs.setColor(sf::Color(255, 255, 255, densityAlpha[zoneDensity(zv)]));
                zs.setPosition(tp);
                _window.draw(zs);
            }
        }
    }

    // PASS 1c: Winter snow over land, lots and roads
    ground.drawSnow(_window);

    // PASS 1.5: Resource / soil overlay
    if (overlayMode != 0) {
        renderOverlay(viewL, viewR, viewT, viewB);
    }

    // Utility networks: power lines / pipes (and the roads that also carry them)
    if (overlayMode == 5) renderUtilityLines(viewL, viewR, viewT, viewB, LINE_POWER);
    if (overlayMode == 6) renderUtilityLines(viewL, viewR, viewT, viewB, LINE_PIPE);

    // PASS 2: Render 3D Isometric Buildings in Isometric Depth Order (i + j),
    // walking only the depths and tiles inside the visible band
    const int jLo = vis.jLowest();
    const int jHi = vis.jHighest();

    // Cars, bucketed by the tile they are drawn with (linked lists per tile)
    carHead.assign(static_cast<size_t>(tileMapW * tileMapH), -1);
    carNext.assign(cars.cars.size(), -1);
    for (size_t c = 0; c < cars.cars.size(); c++) {
        sf::Vector2i t = CarRenderer::drawTile(cars.cars[c]);
        if (!inWorldBounds(t.x, t.y)) continue;
        size_t k = static_cast<size_t>(t.x * tileMapH + t.y);
        carNext[c] = carHead[k];
        carHead[k] = static_cast<int>(c);
    }
    sf::Sprite carSprite;

    for (int depth = vis.iMin + jLo; depth <= vis.iMax + jHi; depth++) {
        int iFrom = std::max(vis.iMin, depth - jHi);
        int iTo = std::min(vis.iMax, depth - jLo);
        for (int i = iFrom; i <= iTo; i++) {
            int j = depth - i;
            if (j >= vis.jMin(i) && j <= vis.jMax(i)) {
                for (int c = carHead[static_cast<size_t>(i * tileMapH + j)]; c >= 0; c = carNext[static_cast<size_t>(c)]) {
                    cars.drawCar(_window, cars.cars[static_cast<size_t>(c)], carSprite);
                }
                if (entities[i][j] != NULL && entities[i][j]->position != NULL) {
                    Zone* z = dynamic_cast<Zone*>(entities[i][j]);
                    if (z != NULL && !z->isSubTile) {
                        float px = z->position->x;
                        float py = z->position->y;
                        if (isVisible(px, py)) {
                            z->drawBuilding(_window);
                            renderUtilityIcons(z);
                            if (z->fireDays > 0) drawFire(z);
                        }
                    }
                } else if (entities[i][j] == NULL) {
                    Terrain t = world[i][j].terrain;
                    if (t == Terrain::Forest || t == Terrain::Rock) {
                        sf::Vector2f tp = tileScreenPos(i, j);
                        if (isVisible(tp.x, tp.y)) ground.drawTall(_window, i, j);
                    }
                }
                if (world[i][j].lines & LINE_POWER) drawPylon(i, j);
            }
        }
    }

    // PASS 3: Drag & Build Ghost Preview & World Cursor
    renderDragPreview();

    if (player != NULL && player->isActive && !isDragging) {
        player->update(deltaTime);
    }
}

// Green below 50% of capacity, yellow at capacity, red at 150% and above
inline sf::Color congestionColor(float cong) {
    float t = std::max(0.f, std::min(1.f, (cong - 0.5f) / 1.f));
    sf::Uint8 r = static_cast<sf::Uint8>(t < 0.5f ? 60 + t * 2.f * 180.f : 240);
    sf::Uint8 g = static_cast<sf::Uint8>(t < 0.5f ? 200 : 200 - (t - 0.5f) * 2.f * 150.f);
    return sf::Color(r, g, 50, 190);
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
            sf::Color col(0, 0, 0, 0);
            if (overlayMode == 7) {
                if (td.terrain == Terrain::Water) continue;
                const int lv = td.landValue;
                col = lv >= LAND_VALUE_TIER3 ? sf::Color(70, 210, 110, 150)
                    : lv >= LAND_VALUE_TIER2 ? sf::Color(230, 200, 70, 140) : sf::Color(200, 70, 60, 130);
                col.a = static_cast<sf::Uint8>(70 + lv);
                diamond.setPosition(px, py);
                diamond.setFillColor(col);
                _window.draw(diamond);
                continue;
            }
            if (overlayMode == 5 || overlayMode == 6) {
                // Utilities: buildings green when served, red when not, amber on
                // untreated water; plants / facilities in the utility colour
                const bool power = overlayMode == 5;
                Zone* z = dynamic_cast<Zone*>(entities[i][j]);
                if (z != NULL && z->isSubTile && inWorldBounds(z->rootX, z->rootY)) z = dynamic_cast<Zone*>(entities[z->rootX][z->rootY]);
                UtilityZone* u = dynamic_cast<UtilityZone*>(z);
                if (u != NULL) {
                    if (isPowerUtility(u->kind) != power) col = sf::Color(10, 12, 18, 90);
                    else col = power ? sf::Color(250, 205, 50, 200) : sf::Color(60, 150, 240, 210);
                } else if (z != NULL && UtilitySystem::consumes(z)) {
                    bool ok = power ? utilities.powered(z->posX, z->posY) : utilities.watered(z->posX, z->posY);
                    bool rawOnly = ok && !power && utilities.untreated(z->posX, z->posY);
                    if (!z->isActive) col = sf::Color(120, 120, 130, 150);
                    else if (rawOnly) col = sf::Color(235, 165, 50, 180);
                    else col = ok ? sf::Color(60, 200, 90, 170) : sf::Color(230, 60, 50, 190);
                } else if (td.terrain == Terrain::Water) {
                    continue;
                } else if (power) {
                    col = sf::Color(10, 12, 18, 90);   // dim land so the network stands out
                } else {
                    // Water view: live groundwater reserve underneath
                    col = sf::Color(40, 130, 230, static_cast<sf::Uint8>(15 + td.groundwater * 1.3f));
                }
                diamond.setPosition(px, py);
                diamond.setFillColor(col);
                _window.draw(diamond);
                continue;
            }
            if (overlayMode == 4) {
                // Traffic: roads from green (free) through yellow to red (jammed); land dimmed
                if (RoadTile::at(i, j) != NULL) {
                    col = congestionColor(traffic.congestion[i][j]);
                } else if (td.terrain != Terrain::Water) {
                    col = sf::Color(10, 12, 18, 80);
                } else {
                    continue;
                }
                diamond.setPosition(px, py);
                diamond.setFillColor(col);
                _window.draw(diamond);
                continue;
            }
            if (td.terrain == Terrain::Water) continue;

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

    const char* icons[OVERLAY_COUNT] = {"view_normal", "view_fertility", "view_water", "view_deposits", "view_traffic",
                                        "view_power", "view_tap", "view_land"};
    const char* names[OVERLAY_COUNT] = {"Normal", "Fertility", "Groundwater", "Deposits", "Traffic", "Power", "Water", "Land value"};
    const sf::Color tints[OVERLAY_COUNT] = {sf::Color(200, 210, 220), sf::Color(120, 215, 90),
                                            sf::Color(80, 170, 240), sf::Color(240, 150, 70), sf::Color(240, 200, 60),
                                            sf::Color(250, 210, 60), sf::Color(90, 180, 245), sf::Color(120, 230, 160)};

    int hovered = -1;
    for (int m = 0; m < OVERLAY_COUNT; m++) {
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
        case 4: x += key(x, congestionColor(0.3f), "Free"); x += key(x, congestionColor(1.0f), "Busy");
                key(x, congestionColor(1.6f), "Jammed"); break;
        case 5: x += key(x, sf::Color(60, 200, 90), "Powered"); x += key(x, sf::Color(230, 60, 50), "Dark");
                key(x, sf::Color(250, 205, 50), "Plant / transformer"); break;
        case 6: x += key(x, sf::Color(60, 200, 90), "Clean"); x += key(x, sf::Color(235, 165, 50), "Untreated");
                x += key(x, sf::Color(230, 60, 50), "Dry"); key(x, sf::Color(40, 130, 230, 200), "Reserve"); break;
        case 7: x += key(x, sf::Color(200, 70, 60), "Low"); x += key(x, sf::Color(230, 200, 70), "Tier 2 (40+)");
                key(x, sf::Color(70, 210, 110), "Tier 3 (60+)"); break;
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

    toolbar.dragCostPreview = -1;
    toolbar.pillNote.clear();
    if ((tool == TOOL_WATER_WELL || tool == TOOL_WATER_PUMP) && inWorldBounds(tileX, tileY)) {
        int kind = utilityToolKind(tool);
        float gw = UtilityZone::groundwaterAround(tileX, tileY, kind == UTIL_WELL ? WELL_RADIUS : PUMP_RADIUS);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.0f water/day here (%d%% groundwater)", UTIL_OUTPUT[kind] * gw,
                      static_cast<int>(gw * 100.f + 0.5f));
        toolbar.pillNote = buf;
    } else if (tool == TOOL_POWER_COAL && inWorldBounds(tileX, tileY)) {
        bool coal = false;
        for (const auto& o : FOOTPRINT_2X2)
            if (inWorldBounds(tileX + o[0], tileY + o[1]) && world[tileX + o[0]][tileY + o[1]].deposit == Deposit::Coal) coal = true;
        toolbar.pillNote = coal ? "on coal: 500 power" : "400 power (500 on coal)";
    }

    if (isRoadTool(tool)) {
        std::vector<sf::Vector2i> previewTiles;
        if (isDragging) previewTiles = computeDragTiles();
        else if (inWorldBounds(tileX, tileY)) previewTiles.push_back({tileX, tileY});
        std::vector<RoadStep> steps = planRoadPath(previewTiles, tool);

        // Ghost frame from the roads that would exist after the drag
        auto planned = [&](int x, int y) {
            for (const auto& st : steps)
                if (st.p.x == x && st.p.y == y) return st.action != ROAD_FAIL;
            return false;
        };
        const int tier = roadToolTier(tool);
        int total = 0;
        for (const RoadStep& st : steps) {
            const int gx = st.p.x, gy = st.p.y;
            sf::Color base = st.action == ROAD_FAIL    ? sf::Color(231, 76, 60)
                           : st.action == ROAD_BRIDGE  ? sf::Color(64, 160, 230)
                           : st.action == ROAD_UPGRADE ? sf::Color(240, 196, 60)
                           : st.action == ROAD_SKIP    ? sf::Color(150, 150, 150)
                                                       : sf::Color(46, 204, 113);
            highlightTile(gx, gy, uikit::withAlpha(base, 85), uikit::withAlpha(base, 230));
            if (st.action != ROAD_FAIL && st.action != ROAD_SKIP) total += st.cost;
            if (st.action == ROAD_SKIP) continue;

            if (st.action == ROAD_BRIDGE) {
                ghostSprite.setTexture(m_textures["road_bridge"]);
                ghostSprite.setTextureRect(sf::IntRect((tier * 2 + st.axis) * 32, 0, 32, 24));
            } else {
                int mask = 0;
                for (int d = 0; d < 4; d++) {
                    int nx = gx + ROAD_DIR[d][0], ny = gy + ROAD_DIR[d][1];
                    RoadTile* n = RoadTile::at(nx, ny);
                    bool linked = planned(nx, ny) || (n != NULL && n->allowsDir(d));
                    if (linked) mask |= 1 << d;
                }
                ghostSprite.setTexture(m_textures[roadSheetKey(tier)]);
                ghostSprite.setTextureRect(sf::IntRect(mask * 32, 0, 32, 16));
            }
            ghostSprite.setOrigin(0.f, 0.f);
            ghostSprite.setScale(2.f, 2.f);
            ghostSprite.setColor(st.action != ROAD_FAIL ? sf::Color(255, 255, 255, 200) : sf::Color(255, 120, 120, 150));
            ghostSprite.setPosition(tileScreenPos(gx, gy));
            _window.draw(ghostSprite);
        }
        if (isDragging) toolbar.dragCostPreview = total;
        return;
    }

    if (isZoneTool(tool) || tool == TOOL_DEZONE) {
        std::vector<sf::Vector2i> previewTiles;
        if (isDragging) previewTiles = computeDragTiles();
        else if (inWorldBounds(tileX, tileY)) previewTiles.push_back({tileX, tileY});
        const sf::Color sectorCol[3] = {sf::Color(72, 200, 110), sf::Color(70, 160, 235), sf::Color(235, 140, 50)};
        const sf::Color zc = tool == TOOL_DEZONE ? sf::Color(220, 110, 90) : sectorCol[zoneSector(zoneToolValue(tool))];
        int total = 0;
        for (const auto& pt : previewTiles) {
            std::string reason;
            bool ok = canPlaceTile(pt.x, pt.y, tool, &reason);
            if (!ok && reason.rfind("Insufficient", 0) != 0) continue;   // skipped tiles stay unmarked
            sf::Color base = ok ? zc : sf::Color(231, 76, 60);
            highlightTile(pt.x, pt.y, uikit::withAlpha(base, 100), uikit::withAlpha(base, 220));
            if (ok) total += getToolCost(tool);
        }
        if (isDragging) toolbar.dragCostPreview = total;
        if (isZoneTool(tool)) toolbar.pillNote = std::string(zoneDensityName(zoneDensity(zoneToolValue(tool)))) + ", grows to tier " +
                                                 std::to_string(zoneMaxTier(zoneToolValue(tool)));
        return;
    }

    if (isLineTool(tool)) {
        std::vector<sf::Vector2i> previewTiles;
        if (isDragging) previewTiles = computeDragTiles();
        else if (inWorldBounds(tileX, tileY)) previewTiles.push_back({tileX, tileY});
        int total = 0;
        const sf::Color lineCol = tool == TOOL_POWER_LINE ? sf::Color(250, 210, 60) : sf::Color(70, 170, 250);
        for (const auto& pt : previewTiles) {
            std::string reason;
            bool ok = canPlaceTile(pt.x, pt.y, tool, &reason);
            bool skip = !ok && (reason.rfind("Already", 0) == 0 || reason.rfind("Roads Already", 0) == 0);
            sf::Color base = ok ? lineCol : (skip ? sf::Color(150, 150, 160) : sf::Color(231, 76, 60));
            highlightTile(pt.x, pt.y, uikit::withAlpha(base, 90), uikit::withAlpha(base, 230));
            if (ok) total += getToolCost(tool);
        }
        if (isDragging) toolbar.dragCostPreview = total;
        toolbar.pillNote = tool == TOOL_POWER_LINE ? "carries power from plants" : "carries water, runs underground";
        return;
    }

    if (tool == TOOL_ROUNDABOUT) {
        if (!inWorldBounds(tileX, tileY)) return;
        bool ok = canPlaceTile(tileX, tileY, tool);
        sf::Color base = ok ? sf::Color(46, 204, 113) : sf::Color(231, 76, 60);
        highlightTile(tileX, tileY, uikit::withAlpha(base, 85), uikit::withAlpha(base, 230));
        if (ok) {
            RoadTile* r = RoadTile::at(tileX, tileY);
            ghostSprite.setTexture(m_textures["roundabout"]);
            ghostSprite.setTextureRect(sf::IntRect(r->mask * 32, 0, 32, 16));
            ghostSprite.setScale(2.f, 2.f);
            ghostSprite.setColor(sf::Color(255, 255, 255, 210));
            ghostSprite.setPosition(tileScreenPos(tileX, tileY));
            _window.draw(ghostSprite);
        }
        return;
    }

    if (tool == TOOL_WATER) {
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

            std::string tkey = "water";
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

            // Coverage radius of police / fire / clinic / bus depot
            if (isServiceTool(tool) && SERVICE_RADIUS[serviceToolKind(tool)] > 0) {
                const int r = SERVICE_RADIUS[serviceToolKind(tool)];
                for (int dx = -r; dx <= r; dx++)
                    for (int dy = -r; dy <= r; dy++)
                        if (dx * dx + dy * dy <= r * r && inWorldBounds(gx + dx, gy + dy))
                            highlightTile(gx + dx, gy + dy, sf::Color(120, 190, 255, 35), sf::Color(120, 190, 255, 60));
            }

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
                default:
                    if (isUtilityTool(tool)) textureKey = utilityTextureKey(utilityToolKind(tool));
                    else if (isServiceTool(tool)) textureKey = serviceTextureKey(serviceToolKind(tool));
                    break;
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
    renderLog(mouse);
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

    // Power and water: delivered / needed (red when anything goes without)
    if (1130.f + sh + 2.f * 136.f <= W) {
        auto utilChip = [&](float x, const char* ic, float have, float need, bool shortage, const char* unit) {
            sf::FloatRect r = chip(x, 130.f);
            sf::Color c = utilities.grace ? TEXT_DIM : (shortage ? BAD : GOOD);
            icon(_window, ic, r.left + 8.f, r.top + 6.f, 24.f, c);
            char buf[48];
            std::snprintf(buf, sizeof(buf), "%.0f/%.0f", have, need);
            text(_window, buf, r.left + 38.f, r.top + 4.f, 13, c, true);
            text(_window, utilities.grace ? std::string("not needed yet") : std::string(unit), r.left + 38.f, r.top + 22.f, 10, TEXT_DIM);
        };
        utilChip(1130.f + sh, "view_power", utilities.powerDelivered, utilities.powerDemand, utilities.unpowered > 0, "power units");
        utilChip(1266.f + sh, "view_tap", utilities.waterDelivered, utilities.waterDemand,
                 utilities.unwatered > 0 || utilities.untreatedShare > 0.3f, "water / day");
    }

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

// Flickering flames and smoke over a burning building
void Play_Scene::drawFire(Zone* z) {
    const float t = gameTime.getElapsedTime().asSeconds();
    const float cx = z->position->x + 32.f, base = z->position->y + 24.f;
    sf::ConvexShape flame(3);
    for (int k = 0; k < 5; k++) {
        float fx = cx - 28.f + 14.f * static_cast<float>(k);
        float h = 22.f + 10.f * std::sin(t * 9.f + static_cast<float>(k) * 1.7f) + 6.f * static_cast<float>(z->fireDays);
        flame.setPoint(0, sf::Vector2f(fx - 8.f, base));
        flame.setPoint(1, sf::Vector2f(fx + 8.f, base));
        flame.setPoint(2, sf::Vector2f(fx + 2.f * std::sin(t * 7.f + static_cast<float>(k)), base - h));
        flame.setFillColor(k % 2 ? sf::Color(255, 120, 30, 220) : sf::Color(255, 205, 60, 220));
        _window.draw(flame);
    }
    sf::CircleShape smoke(9.f);
    for (int k = 0; k < 3; k++) {
        float rise = std::fmod(t * 18.f + static_cast<float>(k) * 14.f, 42.f);
        smoke.setPosition(cx - 9.f + 6.f * std::sin(t + static_cast<float>(k)), base - 40.f - rise);
        smoke.setFillColor(sf::Color(70, 70, 75, static_cast<sf::Uint8>(160.f * (1.f - rise / 42.f))));
        _window.draw(smoke);
    }
}

// A pylon with its wires to the neighbouring pylons (drawn in the depth pass)
void Play_Scene::drawPylon(int x, int y) {
    sf::Vector2f tp = tileScreenPos(x, y);
    const float cx = tp.x + 32.f, cy = tp.y + 18.f, top = cy - 38.f;
    sf::RectangleShape pole(sf::Vector2f(4.f, 38.f));
    pole.setPosition(cx - 2.f, top);
    pole.setFillColor(sf::Color(92, 96, 104));
    _window.draw(pole);
    sf::RectangleShape arm(sf::Vector2f(20.f, 3.f));
    arm.setPosition(cx - 10.f, top + 3.f);
    arm.setFillColor(sf::Color(70, 74, 82));
    _window.draw(arm);
    // Wires to the pylons NE and SE (each pair once; SW / NW draw from the other end)
    sf::VertexArray wires(sf::Lines);
    for (int d = 0; d < 2; d++) {
        int nx = x + ROAD_DIR[d][0], ny = y + ROAD_DIR[d][1];
        if (!hasLine(nx, ny, LINE_POWER)) continue;
        sf::Vector2f np = tileScreenPos(nx, ny);
        for (float off : {-9.f, 9.f}) {
            wires.append(sf::Vertex(sf::Vector2f(cx + off, top + 4.f), sf::Color(35, 35, 40)));
            wires.append(sf::Vertex(sf::Vector2f(np.x + 32.f + off, np.y + 18.f - 34.f), sf::Color(35, 35, 40)));
        }
    }
    if (wires.getVertexCount() > 0) _window.draw(wires);
}

// Power / Water view: thick strokes along every conductor, from tile
// centre towards each connected neighbour. Lines are bright, roads (which
// carry both utilities) fainter.
void Play_Scene::renderUtilityLines(float viewL, float viewR, float viewT, float viewB, uint8_t line) {
    const sf::Color bright = line == LINE_POWER ? sf::Color(255, 215, 60, 240) : sf::Color(70, 175, 255, 240);
    const sf::Color faint = line == LINE_POWER ? sf::Color(255, 215, 60, 110) : sf::Color(70, 175, 255, 110);
    sf::VertexArray quads(sf::Quads);
    auto stroke = [&](sf::Vector2f a, sf::Vector2f b, float w, sf::Color c) {
        sf::Vector2f d = b - a;
        float len = std::sqrt(d.x * d.x + d.y * d.y);
        if (len <= 0.f) return;
        sf::Vector2f n(-d.y / len * w / 2.f, d.x / len * w / 2.f);
        quads.append(sf::Vertex(a + n, c)); quads.append(sf::Vertex(b + n, c));
        quads.append(sf::Vertex(b - n, c)); quads.append(sf::Vertex(a - n, c));
    };
    const VisibleTiles vis = visibleTileRange(viewL, viewR, viewT, viewB, 0.f);
    for (int i = vis.iMin; i <= vis.iMax; i++) {
        for (int j = vis.jMin(i); j <= vis.jMax(i); j++) {
            if (!conductsUtility(i, j, line)) continue;
            const bool own = (world[i][j].lines & line) != 0;
            sf::Vector2f tp = tileScreenPos(i, j);
            sf::Vector2f c(tp.x + 32.f, tp.y + 16.f);
            bool any = false;
            for (const auto& d : ROAD_DIR) {
                int nx = i + d[0], ny = j + d[1];
                if (!conductsUtility(nx, ny, line)) continue;
                bool nOwn = (world[nx][ny].lines & line) != 0;
                sf::Vector2f mid(c.x + 16.f * static_cast<float>(d[0]), c.y + 16.f * static_cast<float>(d[1]) + 8.f * static_cast<float>(d[0]));
                stroke(c, mid, own || nOwn ? 6.f : 3.f, own || nOwn ? bright : faint);
                any = true;
            }
            if (own) {
                // node dot, and a stub to any building it feeds
                stroke(sf::Vector2f(c.x - 4.f, c.y), sf::Vector2f(c.x + 4.f, c.y), 8.f, bright);
            } else if (!any) {
                stroke(sf::Vector2f(c.x - 3.f, c.y), sf::Vector2f(c.x + 3.f, c.y), 3.f, faint);
            }
        }
    }
    if (quads.getVertexCount() > 0) _window.draw(quads);
}

// Blinking "no power" / "no water" markers over a building that lacks them,
// and a warning over a utility that is cut off or has no transformer
void Play_Scene::renderUtilityIcons(Zone* z) {
    if (UtilityZone* u = dynamic_cast<UtilityZone*>(z)) {
        if ((u->linked && !u->noTransformer) || std::fmod(gameTime.getElapsedTime().asSeconds(), 1.2f) > 0.8f) return;
        uikit::icon(_window, "warning", z->position->x + 18.f, z->position->y - 34.f, 28.f);
        return;
    }
    if (utilities.grace || !z->isActive || !utilities.isConsumer(z->posX, z->posY)) return;
    const bool power = utilities.powered(z->posX, z->posY), water = utilities.watered(z->posX, z->posY);
    if (power && water) return;
    if (std::fmod(gameTime.getElapsedTime().asSeconds(), 1.2f) > 0.8f) return;
    float x = z->position->x + 16.f, y = z->position->y - 30.f;
    if (!power && !water) x -= 14.f;
    if (!power) { uikit::icon(_window, "no_power", x, y, 28.f); x += 28.f; }
    if (!water) uikit::icon(_window, "no_water", x, y, 28.f);
}

// Messages chip (bottom-right) and, when open, the message log above it
void Play_Scene::renderLog(const sf::Vector2f& mouse) {
    using namespace uikit;
    sf::FloatRect chipR = logChipBounds();
    int errors = 0;
    for (const Notice& n : notices) if (n.error && gameClock.getTotalDays() - n.day <= 30) errors++;
    bool hover = chipR.contains(mouse);
    roundedRect(_window, chipR, 8.f, hover || logOpen ? PANEL_HOVER : PANEL, logOpen ? ACCENT : BORDER, 1.f);
    std::string label = "Messages [L]" + (errors > 0 ? "  " + std::to_string(errors) : std::string());
    textMid(_window, label, chipR.left + 10.f, chipR.top, chipR.height, 10, errors > 0 ? WARN : TEXT_DIM, errors > 0);
    if (!logOpen) return;

    sf::FloatRect p(SW() - 432.f, SH() - 420.f, 420.f, 356.f);
    panel(_window, p, 12.f);
    text(_window, "Messages", p.left + 14.f, p.top + 10.f, 14, TEXT, true);
    text(_window, "F5 save  F9 load  Z undo", p.left + p.width - 14.f - textWidth("F5 save  F9 load  Z undo", 10), p.top + 14.f, 10, TEXT_DIM);
    float y = p.top + 38.f;
    if (notices.empty()) text(_window, "Nothing yet", p.left + 14.f, y, 11, TEXT_DIM);
    for (const Notice& n : notices) {
        if (y > p.top + p.height - 22.f) break;
        GameClock c;
        c.restore(n.day, 0.f, 1);
        text(_window, c.dateString(), p.left + 14.f, y, 10, TEXT_DIM);
        text(_window, fitText(n.text, 10, p.width - 128.f), p.left + 112.f, y, 10, n.error ? sf::Color(255, 170, 150) : TEXT);
        y += 20.f;
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
    const bool isRoad = isRoadName(kind);
    RoadTile* road = isRoad ? RoadTile::at(tileX, tileY) : NULL;
    const bool isBuilding = !natural && !isRoad;
    const bool isZone = kind == "ResidentalZone" || kind == "CommericalZone" || kind == "IndustrialZone";

    // Title, subtitle, accent and thumbnail
    std::string title, subtitle;
    sf::Color accent = ACCENT;
    if (natural) {
        title = terrainName(world[tileX][tileY].terrain);
        subtitle = isClearGroundName(kind) ? "Open land - buildable" : "Bulldoze to clear";
        if (world[tileX][tileY].zone != 0) {
            const uint8_t zv = world[tileX][tileY].zone;
            title = sectorName(zoneSector(zv)) + " zone";
            subtitle = std::string(zoneDensityName(zoneDensity(zv))) + " - grows to tier " + std::to_string(zoneMaxTier(zv));
        }
        accent = sf::Color(120, 200, 110);
    } else if (isRoad) {
        const char* tierTitle[3] = {"Dirt Road", "Asphalt Road", "Avenue"};
        title = tierTitle[road->tier];
        if (road->isBridge()) title += " Bridge";
        if (road->roundabout) title += " Roundabout";
        subtitle = "Capacity " + std::to_string(static_cast<int>(TrafficSystem::effectiveCapacity(road))) + " trips / day";
        accent = road->tier == 2 ? sf::Color(226, 196, 70) : sf::Color(190, 196, 204);
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
    } else if (serviceKindOfName(kind) >= 0) {
        const ServiceZone* sv = dynamic_cast<const ServiceZone*>(mz);
        title = serviceTitle(sv->kind);
        static const char* subs[SERVICE_KIND_COUNT] = {"Safety for nearby homes", "Puts out fires nearby", "Health for nearby homes",
                                                       "Stores garbage until full", "Jobs on a deposit", "Buses for nearby homes",
                                                       "Burns garbage into power - pollutes", "Recycles garbage cleanly"};
        subtitle = subs[sv->kind];
        accent = sf::Color(150, 170, 220);
    } else if (kind == "UtilityZone") {
        const UtilityZone* u = dynamic_cast<const UtilityZone*>(mz);
        title = utilityTitle(u->kind);
        static const char* subs[UTIL_KIND_COUNT] = {"Power plant - pollutes", "Clean power", "Clean power - weak in winter",
                                                    "Steps power down for its grid", "Clean water from groundwater",
                                                    "Raw water - drains groundwater", "Cleans raw water", "Stores spare water",
                                                    "Sewage into the water - pollutes", "Cleans sewage"};
        subtitle = subs[u->kind];
        accent = isPowerUtility(u->kind) ? sf::Color(245, 200, 60) : sf::Color(64, 160, 230);
    } else {
        title = kind;
    }

    // Height: header + building rows + land section
    const float W = 320.f;
    float hgt = 82.f;                       // header
    if (isBuilding || isRoad) hgt += 34.f;  // status + tax / upkeep row
    if (isRoad) hgt += 30.f;                // traffic bar
    if (isZone) hgt += 2 * 30.f;            // occupancy + happiness bars
    if (kind == "FarmZone") hgt += 22.f;    // soil / season line
    const bool isUtility = kind == "UtilityZone";
    const bool isConsumer = isBuilding && UtilitySystem::consumes(mz);
    if (isUtility) hgt += 2 * 30.f;         // output + grid bars
    if (isConsumer) hgt += 30.f;            // power / water chips
    hgt += 142.f;                           // land section
    const bool isService = !natural && serviceKindOfName(kind) >= 0;
    if (isService) hgt += 30.f;             // service info row
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
        thumbnail(_window, road->roundabout ? "roundabout_icon"
                                            : std::string(roadSheetKey(road->tier)) + "_icon", inner);
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
        const std::string rule = mz->fixedTier ? "Placed - fixed tier"
                               : (mz->zoned ? "Zoned - up to tier " + std::to_string(mz->maxTier) : "Grows freely");
        text(_window, fitText(rule, 10, maxTitleW - 62.f), tx + 62.f, card.top + 56.f, 10,
             mz->fixedTier ? TEXT_DIM : sf::Color(120, 200, 140));
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
        bool outside = roadNet.buildingOutside(mz->posX, mz->posY);
        sf::Color sc = !connected ? BAD : (outside ? GOOD : WARN);
        std::string status = !connected ? "No road access" : (outside ? "Road connected" : "No outside link - slow growth");
        if (kind == "ParkZone") status = "Always open";
        if (mz->fireDays > 0) { status = "ON FIRE"; sc = BAD; }
        if (const UtilityZone* uz = dynamic_cast<const UtilityZone*>(mz)) {
            const bool power = isPowerUtility(uz->kind);
            if (!uz->linked) { status = power ? "Not connected - add road / power line" : "Not connected - add road / pipe"; sc = BAD; }
            else if (uz->noTransformer) { status = "No transformer on this grid"; sc = WARN; }
            else { status = power ? "On the power grid" : "On the water grid"; sc = GOOD; }
        }
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
    if (isRoad) {
        bool outside = roadNet.tileOutside(tileX, tileY);
        sf::Color sc = outside ? GOOD : WARN;
        std::string status = outside ? "Reaches the map edge" : "No outside connection";
        float sw = textWidth(status, 11, true) + 20.f;
        roundedRect(_window, sf::FloatRect(lx, y, sw, 22.f), 11.f, withAlpha(sc, 45), sc, 1.f);
        textMid(_window, status, lx + 10.f, y, 22.f, 11, sc, true);
        std::string cost = "-$" + std::to_string(road->dailyUpkeep()) + " / day";
        textMid(_window, cost, lx + rw - textWidth(cost, 12, true), y, 22.f, 12, BAD, true);
        y += 34.f;
        float cong = traffic.congestion[tileX][tileY];
        barRow("Traffic", std::to_string(static_cast<int>(traffic.load[tileX][tileY])) + " trips  (" +
                          std::to_string(static_cast<int>(cong * 100.f)) + "%)",
               std::min(1.f, cong), cong >= 1.f ? BAD : (cong >= 0.6f ? WARN : GOOD));
    }
    if (isZone) {
        float occ = mz->capacity > 0 ? static_cast<float>(mz->population) / static_cast<float>(mz->capacity) : 0.f;
        barRow("Occupancy", std::to_string(mz->population) + " / " + std::to_string(mz->capacity), occ, ACCENT);
        int hap = mz->overallHappiness;
        barRow("Happiness", std::to_string(hap) + "%", static_cast<float>(hap) / 100.f,
               hap >= 75 ? GOOD : (hap >= 50 ? WARN : BAD));
    }
    if (isConsumer) {
        // Power / water status chips
        float cx = lx;
        auto statusChip = [&](const std::string& label, const sf::Color& c) {
            float w = textWidth(label, 11, true) + 20.f;
            roundedRect(_window, sf::FloatRect(cx, y, w, 22.f), 11.f, withAlpha(c, 45), c, 1.f);
            textMid(_window, label, cx + 10.f, y, 22.f, 11, c, true);
            cx += w + 8.f;
        };
        const int rx = mz->posX, ry = mz->posY;
        if (utilities.grace) {
            statusChip("Utilities not needed yet (town < " + std::to_string(UTILITY_GRACE_POP) + ")", TEXT_DIM);
        } else {
            statusChip(utilities.powered(rx, ry) ? "Power" : "No power", utilities.powered(rx, ry) ? GOOD : BAD);
            if (!utilities.watered(rx, ry)) statusChip("No water", BAD);
            else if (utilities.untreated(rx, ry)) statusChip("Untreated", WARN);
            else statusChip("Water", GOOD);
            statusChip(services.garbageOk(rx, ry) ? "Garbage" : "No pickup", services.garbageOk(rx, ry) ? GOOD : BAD);
            if (!utilities.sewered(rx, ry)) statusChip("No sewer", BAD);
        }
        y += 30.f;
    }
    if (isUtility) {
        const UtilityZone* u = dynamic_cast<const UtilityZone*>(mz);
        const UtilityGrid* g = utilities.gridFor(u);
        char buf[64];
        const float base = UTIL_OUTPUT[u->kind];
        const sf::Color uc = isPowerUtility(u->kind) ? sf::Color(245, 200, 60) : sf::Color(64, 160, 230);
        switch (u->kind) {
            case UTIL_COAL: case UTIL_WIND: case UTIL_SOLAR:
                std::snprintf(buf, sizeof(buf), "%.0f power", u->output);
                barRow("Output", buf, std::min(1.f, u->output / (base * 1.25f)), uc);
                if (g) std::snprintf(buf, sizeof(buf), "%.0f / %.0f", g->powerUsed, std::min(g->powerSupply, g->transformerCap));
                barRow(g && g->transformerCap <= 0.f ? "Grid: no transformer!" : "Grid load", g ? buf : "No road",
                       g && g->powerSupply > 0.f ? std::min(1.f, g->powerUsed / std::max(1.f, std::min(g->powerSupply, g->transformerCap))) : 0.f,
                       g && g->transformerCap <= 0.f ? BAD : uc);
                break;
            case UTIL_TRANSFORMER:
                if (g) std::snprintf(buf, sizeof(buf), "%.0f from plants", g->powerSupply);
                barRow("Grid supply", g ? buf : "No road", g ? std::min(1.f, g->powerSupply / std::max(1.f, g->powerDemand)) : 0.f,
                       g && g->powerSupply >= g->powerDemand ? GOOD : BAD);
                if (g) std::snprintf(buf, sizeof(buf), "%.0f / %.0f", g->powerUsed, g->transformerCap);
                barRow("Transformers on grid", g ? buf : "-", g ? std::min(1.f, g->powerUsed / std::max(1.f, g->transformerCap)) : 0.f, uc);
                break;
            case UTIL_WELL: case UTIL_PUMP: {
                float gw = UtilityZone::groundwaterAround(u->posX, u->posY, u->drawRadius());
                std::snprintf(buf, sizeof(buf), "%.0f / %.0f %s", u->output, base, u->kind == UTIL_WELL ? "clean" : "raw");
                barRow("Output", buf, u->output / base, uc);
                std::snprintf(buf, sizeof(buf), "%d%% (radius %d)", static_cast<int>(gw * 100.f + 0.5f), u->drawRadius());
                barRow("Groundwater reserve", buf, gw, gw < 0.25f ? BAD : (gw < 0.45f ? WARN : GOOD));
                break;
            }
            case UTIL_TREATMENT:
                if (g) std::snprintf(buf, sizeof(buf), "%.0f / %.0f", g->treated(), g->treatCap);
                barRow("Treating on grid", g ? buf : "No road", g ? std::min(1.f, g->treated() / std::max(1.f, g->treatCap)) : 0.f, uc);
                if (g) std::snprintf(buf, sizeof(buf), "%.0f untreated", g->untreatedPool());
                barRow("Raw water left over", g ? buf : "-", g && g->raw > 0.f ? g->untreatedPool() / g->raw : 0.f,
                       g && g->untreatedPool() > 0.f ? WARN : GOOD);
                break;
            case UTIL_SEWAGE_OUTFLOW:
            case UTIL_SEWAGE_PLANT:
                if (g) std::snprintf(buf, sizeof(buf), "%.0f / %.0f", g->sewageUsed, g->sewageCap);
                barRow("Sewage on grid", g ? buf : "Not connected", g && g->sewageCap > 0.f ? std::min(1.f, g->sewageUsed / g->sewageCap) : 0.f,
                       g && g->sewageDemand > g->sewageCap ? BAD : uc);
                if (g) std::snprintf(buf, sizeof(buf), "%.0f needed", g->sewageDemand);
                barRow("Waste water", g ? buf : "-", g && g->sewageDemand > 0.f ? std::min(1.f, g->sewageCap / g->sewageDemand) : 0.f, uc);
                break;
            case UTIL_TOWER:
                std::snprintf(buf, sizeof(buf), "%.0f / %.0f", u->stored, base);
                barRow("Stored", buf, u->stored / base, uc);
                if (g) std::snprintf(buf, sizeof(buf), "%.0f / %.0f", g->cleanUsed + g->towerUsed + g->untreatedUsed, g->waterDemand);
                barRow("Grid water", g ? buf : "No road", g ? std::min(1.f, (g->cleanPool() + g->raw - g->treated()) / std::max(1.f, g->waterDemand)) : 0.f,
                       GOOD);
                break;
        }
    }
    if (isService) {
        const ServiceZone* sv = dynamic_cast<const ServiceZone*>(mz);
        char buf[80];
        switch (sv->kind) {
            case SERVICE_LANDFILL:
                std::snprintf(buf, sizeof(buf), "%.0f / %.0f  (+%.0f today)", sv->stored, LANDFILL_SPACE, sv->handled);
                barRow(sv->stored >= LANDFILL_SPACE ? "FULL - build another" : "Filled", buf, sv->stored / LANDFILL_SPACE,
                       sv->stored >= LANDFILL_SPACE ? BAD : (sv->stored > 0.8f * LANDFILL_SPACE ? WARN : sf::Color(170, 150, 110)));
                break;
            case SERVICE_INCINERATOR:
                std::snprintf(buf, sizeof(buf), "%.0f / %.0f  -> %.0f power", sv->handled, INCINERATOR_RATE, sv->handled * INCINERATOR_POWER);
                barRow("Burning today", buf, sv->handled / INCINERATOR_RATE, sf::Color(230, 120, 60));
                break;
            case SERVICE_RECYCLING:
                std::snprintf(buf, sizeof(buf), "%.0f / %.0f", sv->handled, RECYCLING_RATE);
                barRow("Recycling today", buf, sv->handled / RECYCLING_RATE, GOOD);
                break;
            case SERVICE_MINE:
                std::snprintf(buf, sizeof(buf), "%d / %d miners - deposit %d%%", sv->population, sv->capacity,
                              static_cast<int>(sv->depositRichness() * 100.f + 0.5f));
                barRow("Mine", buf, sv->depositRichness(), sf::Color(170, 150, 130));
                break;
            default:
                std::snprintf(buf, sizeof(buf), "radius %d", SERVICE_RADIUS[sv->kind]);
                barRow(sv->kind == SERVICE_BUS_DEPOT ? "30% of commuters ride the bus" : "Covers homes", buf, 1.f, ACCENT);
                break;
        }
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
    barRow("Land value", std::to_string(land.landValue) + (land.landValue >= LAND_VALUE_TIER3 ? "  (tier 3 ok)" : land.landValue >= LAND_VALUE_TIER2 ? "  (tier 2 ok)" : ""),
           static_cast<float>(land.landValue) / 100.f, sf::Color(120, 220, 150));
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
                    std::memcpy(taxBeforeDrag, taxRates, sizeof(taxBeforeDrag));   // for undo
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
    if (!down && activeSlider >= 0) {
        // Slider released: remember the old rates so Z can put them back
        if (std::memcmp(taxBeforeDrag, taxRates, sizeof(taxBeforeDrag)) != 0) {
            UndoStep step{"", 0, true};
            std::memcpy(step.tax, taxBeforeDrag, sizeof(step.tax));
            undoStack.push_back(step);
            if (undoStack.size() > UNDO_LIMIT) undoStack.erase(undoStack.begin());
        }
        activeSlider = -1;
    }
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
    row("Power & water", dollars(static_cast<float>(rep.upkeepUtilities)), TEXT);
    row("Services", dollars(static_cast<float>(rep.upkeepServices)), TEXT);
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

#include "./SaveGame.hpp"
