#pragma once
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <ctime>
#include <random>
#include "../../src/Scene.hpp"
#include "../../src/Audio.hpp"
#include "../../src/Settings.hpp"
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
    bool trafficDue = false;          // today's commutes still to be routed
    bool servicesDue = false;         // today's fires / mines / land value still to run (next frame)
    static constexpr int TRAFFIC_HOMES_PER_FRAME = 40;

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
    static const int OVERLAY_COUNT = 9; // Normal, Fertility, Groundwater, Deposits, Traffic, Power, Water, Land value, Services
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
    int buildBenchMetro();
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
    bool autosaveAllowed() const { return settings.autosave && !headless && benchSeconds <= 0.f && fastForwardDays <= 0; }

    // ---- Menus (play/Menu.hpp) ----
    enum MenuPage { MENU_NONE, MENU_TITLE, MENU_PAUSE, MENU_NEW, MENU_SETTINGS };
    MenuPage menuPage = MENU_NONE;
    MenuPage menuBack = MENU_TITLE;
    uint32_t newCitySeed = 1;
    bool newCitySandbox = false;
    bool menuMouseWasDown = false;
    bool wasEscKey = false;
    struct MenuButton { sf::FloatRect r; int id; std::string label; bool enabled; bool active; };
    std::vector<MenuButton> menuButtons() const;
    void openMenu(MenuPage page);
    void handleMenuClick(int id);
    void beginNewCity(uint32_t seed, int difficulty, bool sandbox);
    void updateMenu(float deltaTime);
    void renderMenu(const sf::Vector2f& mouse);

    // ---- Statistics (play/Stats.hpp) ----
    struct DaySample { int population = 0, money = 0, net = 0, happiness = 0; int demand[3] = {0, 0, 0}; float exports = 0.f, imports = 0.f; };
    std::deque<DaySample> history;
    static constexpr size_t HISTORY_DAYS = 360;
    static constexpr int STATS_TABS = 6;
    bool statsOpen = false;
    int statsTab = 0;
    bool wasStatsKey = false;
    void recordHistory(const DayReport& rep);
    sf::FloatRect statsBounds() const;
    sf::FloatRect statsTabBounds(int tab) const;
    sf::FloatRect statsChipBounds() const { return sf::FloatRect(uikit::SW() - 298.f, uikit::SH() - 58.f, 140.f, 24.f); }
    bool handleStatsClick(const sf::Vector2f& p);
    void drawChart(const sf::FloatRect& r, const std::vector<std::vector<float>>& series, const std::vector<sf::Color>& colors, bool symmetric);
    void renderStats(const sf::Vector2f& mouse);

    // ---- Day / night and minimap (play/Polish.hpp) ----
    static constexpr float DAYNIGHT_DAYS = 6.f;   // game days per light cycle
    float nightAmount() const;
    void renderNight(float viewL, float viewR, float viewT, float viewB);
    sf::VertexArray minimapQuads;
    float minimapScale = 0.05f;
    bool minimapDirty = true;         // something on the map changed since the last rebuild
    sf::FloatRect minimapBuiltFor;    // bounds the quads were built for (window resizes)
    bool wasMinimapKey = false;
    sf::FloatRect minimapBounds() const;
    sf::Color minimapColor(int x, int y) const;
    void rebuildMinimap();
    bool handleMinimapClick(const sf::Vector2f& p);
    void renderMinimap(const sf::Vector2f& mouse);

    // ---- City ranks (milestones) and tips ----
    bool milestonesEnabled = false;   // normal play only: tools unlock as the city grows
    int bestPopulation = 0;           // highest population reached (ranks never go down)
    int rankReached = 0;              // ranks need population, happiness and services; never go down
    int cityRank() const { return rankReached; }
    struct RankCheck { std::string label; bool met; std::string value; };
    std::vector<RankCheck> rankRequirements(int rank) const;
    bool meetsRank(int rank) const;
    std::string rankProgressText() const;
    void renderRankTab(const sf::FloatRect& area);
    void checkMilestones();
    std::string tipText;
    std::string computeTip() const;
    sf::FloatRect tipBounds() const;
    void renderTip(const sf::Vector2f& mouse);

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
    if (benchMetro) {
        int lots = buildBenchMetro();
        std::cout << "\nBench metro: " << lots << " zoned tiles\n";
    }
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
    if (startStats) { statsOpen = true; statsTab = 5; }
    if (showTitleMenu) {
        menuPage = MENU_TITLE;
        currentZoom = 1.3f;
        view.setSize(uikit::SW() * currentZoom, uikit::SH() * currentZoom);
    }

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
    trade.reset();
    history.clear();
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
    if (menuPage != MENU_NONE) {
        updateMenu(deltaTime);
        return;
    }
    toolbar.unlockedRank = milestonesEnabled ? cityRank() : 99;
    for (int s = 0; s < SECTOR_COUNT; s++)
        for (int w = 0; w < WEALTH_COUNT; w++) toolbar.zoneDemand[s][w] = economy.demand[s][w];
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
    bool overMinimap = settings.minimap && minimapBounds().contains(mousePixelF);
    bool overStats = statsChipBounds().contains(mousePixelF) || (statsOpen && statsBounds().contains(mousePixelF));
    bool overTip = settings.tips && milestonesEnabled && tipBounds().contains(mousePixelF);
    bool overLog = logChipBounds().contains(mousePixelF) || (logOpen && sf::FloatRect(uikit::SW() - 432.f, uikit::SH() - 420.f, 420.f, 356.f).contains(mousePixelF));
    bool mouseOverUI = !focused || economy.bankrupt || toolbar.contains(mousePixelF) || overViewPanel || overLog || overTip || overStats || overMinimap ||
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
        if (overMinimap && !(statsOpen && statsBounds().contains(mousePixelF))) {
            handleMinimapClick(mousePixelF);
        } else if (statsChipBounds().contains(mousePixelF)) {
            statsOpen = !statsOpen;
        } else if (statsOpen && handleStatsClick(mousePixelF)) {
            // stats panel used the click
        } else if (overTip) {
            sf::FloatRect r = tipBounds();
            if (mousePixelF.x >= r.left + r.width - 34.f) { settings.tips = false; settings.save(); }
        } else if (logChipBounds().contains(mousePixelF)) {
            logOpen = !logOpen;
        } else if (overLog) {
            // clicks on the open log do nothing
        } else if (overViewPanel) {
            handleViewPanelClick(mousePixelF);
        } else if (mousePixelF.y <= 50.f) {
            if (moneyChipBounds().contains(mousePixelF)) setBudgetOpen(!budgetOpen);
            else handleSpeedClick(mousePixelF);
        } else {
            if (toolbar.handleClick(mousePixelF)) audio.play("click", 0.6f);
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
    audio.update(gameClock.isPaused(), services.burning > 0);
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
        if (milestonesEnabled) tipText = computeTip();
    }
    int newDays = gameClock.advance(realSeconds);
    // The day's work is staggered over frames: economy on the day boundary,
    // services on the next frame, commutes from midday
    const bool skipping = realSeconds >= GameClock::SECONDS_PER_DAY * 0.5f;
    if (servicesDue && (newDays > 0 || !skipping)) { runServicesDay(); servicesDue = false; }
    for (int d = 0; d < newDays; d++) {
        if (servicesDue) { runServicesDay(); servicesDue = false; }   // catch up before the next day
        if (trafficDue) { traffic.update(); trafficDue = false; }
        else if (traffic.inProgress()) traffic.step(1 << 30);
        runNewDay();
    }
    if (servicesDue && skipping) { runServicesDay(); servicesDue = false; }
    // Commutes from midday, a slice of homes per frame (all at once when a
    // whole day is skipped, e.g. --fast-forward)
    if (trafficDue && (gameClock.getDayProgress() >= 0.5f || realSeconds >= GameClock::SECONDS_PER_DAY * 0.5f)) {
        traffic.begin();
        trafficDue = false;
        if (realSeconds >= GameClock::SECONDS_PER_DAY * 0.5f) traffic.step(1 << 30);
    } else if (traffic.inProgress()) {
        traffic.step(TRAFFIC_HOMES_PER_FRAME);
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
    if (services.runDay(gameClock.getTotalDays(), townPopulation[SECTOR_R])) { utilitiesDirty = true; minimapDirty = true; }
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
    if (milestonesEnabled) tipText = computeTip();
}

void Play_Scene::notify(const std::string& msg, bool isError) {
    showToast(msg, isError);
    if (isError) audio.play("alert", 0.8f);
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
    minimapDirty = true;
    trafficDue = true; // commutes are routed half a day later (spreads the daily work over two frames)
    DayReport rep = economy.runDay(happinessSystem);
    lastDailyIncome = rep.net;
    checkMilestones();
    recordHistory(rep);

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

    servicesDue = true;   // fires, mines, land value and tips run on the next frame

    // Autosave once a month (not in benchmarks / tests)
    if (autosaveAllowed() && today > 0 && today % GameClock::DAYS_PER_MONTH == 0) saveCity("autosave.sav");

    if (economy.bankrupt) {
        gameClock.setSpeedIndex(0);
        setBudgetOpen(false);
        placementErrorTimer = 0.f; // the game-over screen replaces any warning
        toolbar.setActiveTool(TOOL_NONE);
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

#include "./play/Input.hpp"
#include "./play/Placement.hpp"
#include "./play/Towns.hpp"
#include "./play/WorldRender.hpp"
#include "./play/Hud.hpp"
#include "./play/Menu.hpp"
#include "./play/Stats.hpp"
#include "./play/Polish.hpp"
#include "./play/Progress.hpp"
#include "./SaveGame.hpp"
