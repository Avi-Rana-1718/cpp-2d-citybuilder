#include "test_framework.hpp"

#include <SFML/Graphics.hpp>
#include <sstream>
#include "../src/_global.hpp"
#include "../src/Entity.hpp"
#include "../game/entities/_Zone.hpp"
#include "../game/entities/ResidentalZone.hpp"
#include "../game/entities/CommericalZone.hpp"
#include "../game/entities/IndustrialZone.hpp"
#include "../game/entities/FarmZone.hpp"
#include "../game/entities/HarbourZone.hpp"
#include "../game/entities/SchoolZone.hpp"
#include "../game/entities/ChurchZone.hpp"
#include "../game/entities/ParkZone.hpp"
#include "../game/entities/SubZone.hpp"
#include "../game/entities/roads/DirtRoad.hpp"
#include "../game/simulation/HappinessSystem.hpp"
#include "../game/ui/Toolbar.hpp"
#include "../game/scenes/Play_Scene.hpp"
#include "../game/world/WorldData.hpp"
#include "../game/world/WorldGen.hpp"
#include "../game/world/GroundRenderer.hpp"
#include "../game/simulation/GameClock.hpp"

// RAII helper to silence stdout logs from entity constructors/destructors during tests
struct ScopedCoutSilence {
    std::streambuf* orig;
    std::stringstream sink;
    ScopedCoutSilence() : orig(std::cout.rdbuf(sink.rdbuf())) {}
    ~ScopedCoutSilence() { std::cout.rdbuf(orig); }
};

// Helper to clear the grid before/after tests
static void clearTestGrid() {
    ScopedCoutSilence silence;
    resetWorldData();
    gameClock = GameClock(); // 1 Mar, Year 1, 1x
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            delete entities[i][j];
            entities[i][j] = NULL;
        }
    }
}

// -------------------------------------------------------------
// 1. Isometric Projection & Raycasting Tests
// -------------------------------------------------------------
TEST_CASE(Test_IsometricProjection_Formula) {
    int posX = 4;
    int posY = 6;
    float expectedX = static_cast<float>(tileSizeX * posX - posX * (tileSizeX / 2));
    float expectedY = static_cast<float>(tileSizeY * posY + posX * (tileSizeY / 2));

    ASSERT_EQ(expectedX, 128.0f); // 64 * 4 - 4 * 32 = 128
    ASSERT_EQ(expectedY, 256.0f); // 32 * 6 + 4 * 16 = 192 + 64 = 256

    sf::Vector2f tp = tileScreenPos(posX, posY);
    ASSERT_NEAR(tp.x, expectedX, 0.001f);
    ASSERT_NEAR(tp.y, expectedY, 0.001f);
}

TEST_CASE(Test_IsometricRaycasting_DiamondCenter) {
    int targetX = 5;
    int targetY = 8;
    float px = static_cast<float>(tileSizeX * targetX - targetX * (tileSizeX / 2));
    float py = static_cast<float>(tileSizeY * targetY + targetX * (tileSizeY / 2));

    // Mouse directly at diamond center: (px + 32, py + 16)
    sf::Vector2f mouseWorld(px + 32.f, py + 16.f);

    float dx = std::abs(mouseWorld.x - (px + tileSizeX / 2.0f));
    float dy = std::abs(mouseWorld.y - (py + tileSizeY / 2.0f));
    float dist = (dx / (tileSizeX / 2.0f)) + (dy / (tileSizeY / 2.0f));

    ASSERT_TRUE(dist <= 1.0f);
    ASSERT_NEAR(dist, 0.0f, 0.001f);
}

TEST_CASE(Test_IsometricRaycasting_DiamondTipsAndExclusion) {
    float px = 200.f;
    float py = 300.f;
    float cx = px + 32.f;
    float cy = py + 16.f;

    auto testPoint = [&](float x, float y) -> bool {
        float dx = std::abs(x - cx);
        float dy = std::abs(y - cy);
        return (dx / 32.f + dy / 16.f) <= 1.0f;
    };

    // Four diamond vertices (must be inside)
    ASSERT_TRUE(testPoint(cx, cy - 16.f)); // Top tip
    ASSERT_TRUE(testPoint(cx + 32.f, cy)); // Right tip
    ASSERT_TRUE(testPoint(cx, cy + 16.f)); // Bottom tip
    ASSERT_TRUE(testPoint(cx - 32.f, cy)); // Left tip

    // Points outside diamond (corners of bounding box)
    ASSERT_FALSE(testPoint(px, py));                  // Top-left corner
    ASSERT_FALSE(testPoint(px + 64.f, py));           // Top-right corner
    ASSERT_FALSE(testPoint(px, py + 32.f));           // Bottom-left corner
    ASSERT_FALSE(testPoint(px + 64.f, py + 32.f));    // Bottom-right corner
}

// -------------------------------------------------------------
// 2. Toolbar Component Tests
// -------------------------------------------------------------
TEST_CASE(Test_Toolbar_LayoutAndDefaults) {
    Toolbar tb;
    tb.init(1500.f, 900.f);

    // Starts in Inspect mode with the flyout closed
    ASSERT_EQ(static_cast<int>(tb.getActiveTool()), static_cast<int>(TOOL_NONE));
    ASSERT_FALSE(tb.isFlyoutOpen());

    // Sidebar hugs the left edge below the top bar
    sf::FloatRect side = tb.sidebarBounds();
    ASSERT_TRUE(side.left < 20.f);
    ASSERT_TRUE(side.top > 48.f);
    ASSERT_TRUE(tb.contains(sf::Vector2f(side.left + side.width / 2.f, side.top + 20.f)));
    ASSERT_FALSE(tb.contains(sf::Vector2f(750.f, 450.f)));  // map centre is free
    ASSERT_FALSE(tb.contains(sf::Vector2f(750.f, 850.f)));  // bottom of the screen is free

    // Every category icon sits inside the sidebar without overlapping the next
    ToolbarCategory cats[6] = {ToolbarCategory::INSPECT, ToolbarCategory::RESIDENTIAL, ToolbarCategory::COMMERCIAL,
                               ToolbarCategory::INDUSTRIAL, ToolbarCategory::CIVICS, ToolbarCategory::INFRASTRUCTURE};
    for (int i = 0; i < 6; i++) {
        sf::FloatRect b = tb.tabBounds(cats[i]);
        ASSERT_TRUE(b.width > 0.f);
        ASSERT_TRUE(side.contains(b.left, b.top) && side.contains(b.left + b.width, b.top + b.height));
        if (i > 0) ASSERT_FALSE(b.intersects(tb.tabBounds(cats[i - 1])));
    }
}

TEST_CASE(Test_Toolbar_FlyoutClickFlow) {
    Toolbar tb;
    tb.init(1500.f, 900.f);
    auto centre = [](const sf::FloatRect& r) { return sf::Vector2f(r.left + r.width / 2.f, r.top + r.height / 2.f); };

    // Category icon opens its flyout; the flyout region becomes UI
    ASSERT_TRUE(tb.handleClick(centre(tb.tabBounds(ToolbarCategory::CIVICS))));
    ASSERT_TRUE(tb.isFlyoutOpen());
    ASSERT_TRUE(tb.contains(centre(tb.flyoutBounds())));

    // Clicking a card selects the tool and closes the flyout
    ASSERT_TRUE(tb.handleClick(centre(tb.cardBoundsFor(TOOL_PARK))));
    ASSERT_EQ(static_cast<int>(tb.getActiveTool()), static_cast<int>(TOOL_PARK));
    ASSERT_FALSE(tb.isFlyoutOpen());

    // The active-tool pill shows while placing; its X cancels
    ASSERT_TRUE(tb.contains(centre(tb.pillBounds())));
    ASSERT_TRUE(tb.handleClick(centre(tb.pillCloseBounds())));
    ASSERT_EQ(static_cast<int>(tb.getActiveTool()), static_cast<int>(TOOL_NONE));
    ASSERT_FALSE(tb.contains(centre(tb.pillBounds())));

    // Second click on the same category closes its flyout
    tb.handleClick(centre(tb.tabBounds(ToolbarCategory::RESIDENTIAL)));
    ASSERT_TRUE(tb.isFlyoutOpen());
    tb.handleClick(centre(tb.tabBounds(ToolbarCategory::RESIDENTIAL)));
    ASSERT_FALSE(tb.isFlyoutOpen());

    // Inspect icon clears the tool
    tb.setActiveTool(TOOL_ROAD);
    tb.handleClick(centre(tb.tabBounds(ToolbarCategory::INSPECT)));
    ASSERT_EQ(static_cast<int>(tb.getActiveTool()), static_cast<int>(TOOL_NONE));

    // Hotkeys select a tool and its category without opening the flyout
    tb.setActiveTool(TOOL_FARM);
    ASSERT_TRUE(tb.getActiveCategory() == ToolbarCategory::INDUSTRIAL);
    ASSERT_FALSE(tb.isFlyoutOpen());

    // Card prices come from the same table the game charges
    Play_Scene scene;
    ASSERT_EQ(scene.getToolCost(TOOL_RES_TOWER), toolCost(TOOL_RES_TOWER));
    ASSERT_EQ(toolCost(TOOL_RES_COTTAGES), 100);
}

TEST_CASE(Test_Toolbar_ToolSelectionAndToggle) {
    Toolbar tb;
    tb.init(1500.f, 900.f);

    tb.setActiveTool(TOOL_ROAD);
    ASSERT_EQ(static_cast<int>(tb.getActiveTool()), static_cast<int>(TOOL_ROAD));

    tb.setActiveTool(TOOL_BULLDOZE);
    ASSERT_EQ(static_cast<int>(tb.getActiveTool()), static_cast<int>(TOOL_BULLDOZE));

    tb.setActiveTool(TOOL_NONE);
    ASSERT_EQ(static_cast<int>(tb.getActiveTool()), static_cast<int>(TOOL_NONE));
}

// -------------------------------------------------------------
// 3. Drag Gesture Computation Tests
// -------------------------------------------------------------
TEST_CASE(Test_DragComputation_RectangularBox) {
    auto computeBox = [](sf::Vector2i start, sf::Vector2i curr) {
        std::vector<sf::Vector2i> tiles;
        int minX = std::max(0, std::min(start.x, curr.x));
        int maxX = std::min(tileMapW - 1, std::max(start.x, curr.x));
        int minY = std::max(0, std::min(start.y, curr.y));
        int maxY = std::min(tileMapH - 1, std::max(start.y, curr.y));

        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                tiles.push_back({x, y});
            }
        }
        return tiles;
    };

    // Standard 3x4 drag: [2..4] x [3..6] = 12 tiles
    auto tiles = computeBox({2, 3}, {4, 6});
    ASSERT_EQ(tiles.size(), 12);
    ASSERT_EQ(tiles.front().x, 2);
    ASSERT_EQ(tiles.front().y, 3);
    ASSERT_EQ(tiles.back().x, 4);
    ASSERT_EQ(tiles.back().y, 6);

    // Reversed drag: start (5, 5) to (3, 3) must give same 3x3 = 9 tiles
    auto revTiles = computeBox({5, 5}, {3, 3});
    ASSERT_EQ(revTiles.size(), 9);
    ASSERT_EQ(revTiles.front().x, 3);
    ASSERT_EQ(revTiles.front().y, 3);
    ASSERT_EQ(revTiles.back().x, 5);
    ASSERT_EQ(revTiles.back().y, 5);

    // Single tile drag
    auto singleTile = computeBox({7, 7}, {7, 7});
    ASSERT_EQ(singleTile.size(), 1);
    ASSERT_EQ(singleTile[0].x, 7);
    ASSERT_EQ(singleTile[0].y, 7);
}

TEST_CASE(Test_DragComputation_RoadLPath) {
    auto computeRoadPath = [](sf::Vector2i start, sf::Vector2i curr) {
        std::vector<sf::Vector2i> tiles;
        int stepX = (curr.x >= start.x) ? 1 : -1;
        int stepY = (curr.y >= start.y) ? 1 : -1;

        for (int x = start.x; ; x += stepX) {
            tiles.push_back({x, start.y});
            if (x == curr.x) break;
        }
        for (int y = start.y + stepY; ; y += stepY) {
            if ((stepY > 0 && y > curr.y) || (stepY < 0 && y < curr.y)) break;
            tiles.push_back({curr.x, y});
            if (y == curr.y) break;
        }
        return tiles;
    };

    // Horizontal road (3, 2) to (7, 2) -> 5 tiles
    auto hRoad = computeRoadPath({3, 2}, {7, 2});
    ASSERT_EQ(hRoad.size(), 5);
    for (size_t i = 0; i < hRoad.size(); i++) {
        ASSERT_EQ(hRoad[i].x, 3 + static_cast<int>(i));
        ASSERT_EQ(hRoad[i].y, 2);
    }

    // Vertical road (4, 1) to (4, 5) -> 5 tiles
    auto vRoad = computeRoadPath({4, 1}, {4, 5});
    ASSERT_EQ(vRoad.size(), 5);
    for (size_t i = 0; i < vRoad.size(); i++) {
        ASSERT_EQ(vRoad[i].x, 4);
        ASSERT_EQ(vRoad[i].y, 1 + static_cast<int>(i));
    }

    // L-shaped road (2, 2) to (4, 4) -> (2,2), (3,2), (4,2) then (4,3), (4,4) -> 5 tiles
    auto lRoad = computeRoadPath({2, 2}, {4, 4});
    ASSERT_EQ(lRoad.size(), 5);
    ASSERT_EQ(lRoad[0].x, 2); ASSERT_EQ(lRoad[0].y, 2);
    ASSERT_EQ(lRoad[2].x, 4); ASSERT_EQ(lRoad[2].y, 2); // Corner
    ASSERT_EQ(lRoad[4].x, 4); ASSERT_EQ(lRoad[4].y, 4); // End
}

// -------------------------------------------------------------
// 4. Zone Capacity & Lifecycle Tests
// -------------------------------------------------------------
TEST_CASE(Test_Zone_CapacityAccounting) {
    clearTestGrid();

    int initialCapacity = townCapacity[0];

    // Create residential zone
    ScopedCoutSilence silence;
    ResidentalZone* rz = new ResidentalZone(5, 5);
    int addedCapacity = rz->capacity;
    ASSERT_EQ(townCapacity[0], initialCapacity + addedCapacity);

    // Destroy residential zone: capacity must decrement cleanly without leakage
    delete rz;
    ASSERT_EQ(townCapacity[0], initialCapacity);

    // Commercial zone capacity accounting
    int initialComCapacity = townCapacity[1];
    CommericalZone* cz = new CommericalZone(6, 6);
    int addedComCapacity = cz->capacity;
    ASSERT_EQ(townCapacity[1], initialComCapacity + addedComCapacity);

    delete cz;
    ASSERT_EQ(townCapacity[1], initialComCapacity);

    clearTestGrid();
}

// -------------------------------------------------------------
// 5. Zone Adjacency & Activation Tests
// -------------------------------------------------------------
TEST_CASE(Test_Zone_AdjacencyActivationRules) {
    clearTestGrid();
    ScopedCoutSilence silence;

    // Isolated zone must be inactive
    delete entities[5][5];
    ResidentalZone* rz = new ResidentalZone(5, 5);
    entities[5][5] = rz;
    rz->checkValid();
    ASSERT_FALSE(rz->isActive);

    // A road only touching the footprint at a diamond tip (0, +2) does not count
    entities[5][7] = new DirtRoad(5, 7);
    rz->checkValid();
    ASSERT_FALSE(rz->isActive);
    delete entities[5][7]; entities[5][7] = NULL;

    // Nor does the box corner (+2, +2), which does not touch the building at all
    entities[7][7] = new DirtRoad(7, 7);
    rz->checkValid();
    ASSERT_FALSE(rz->isActive);
    delete entities[7][7]; entities[7][7] = NULL;

    // Every edge-sharing border tile activates the zone
    for (const auto& o : BORDER_2X2) {
        int bx = 5 + o[0], by = 5 + o[1];
        entities[bx][by] = new DirtRoad(bx, by);
        rz->checkValid();
        ASSERT_TRUE(rz->isActive);
        delete entities[bx][by]; entities[bx][by] = NULL;
    }

    // Road at (4, 5) shares the west edge
    entities[4][5] = new DirtRoad(4, 5);
    rz->checkValid();
    ASSERT_TRUE(rz->isActive);

    // Removing it deactivates the zone
    delete entities[4][5];
    entities[4][5] = NULL;
    rz->checkValid();
    ASSERT_FALSE(rz->isActive);

    // No chaining: a zone touching only an active residential zone stays inactive
    entities[4][5] = new DirtRoad(4, 5);
    rz->checkValid();
    ASSERT_TRUE(rz->isActive);
    ResidentalZone* rz2 = new ResidentalZone(7, 5); // shares an edge with rz, not with the road
    entities[7][5] = rz2;
    rz2->checkValid();
    ASSERT_FALSE(rz2->isActive);

    clearTestGrid();
}

TEST_CASE(Test_Zone_RoadAccessUpdatesOnChange) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 50000;

    // Placement and activation use the same edge-sharing rule:
    // a road at (10, 12) only touches the lot at (10, 10) at a diamond tip
    scene.placeTile(10, 12, TOOL_ROAD);
    std::string reason;
    ASSERT_FALSE(scene.canPlaceTile(10, 10, TOOL_RES_COTTAGES, &reason));
    ASSERT_TRUE(reason.find("Requires Road Connection") != std::string::npos);

    // Road on the west edge (9, 10): placement allowed, building active immediately
    scene.placeTile(9, 10, TOOL_ROAD);
    ASSERT_TRUE(scene.canPlaceTile(10, 10, TOOL_RES_COTTAGES));
    scene.placeTile(10, 10, TOOL_RES_COTTAGES);
    Zone* home = dynamic_cast<Zone*>(entities[10][10]);
    ASSERT_TRUE(home != NULL);
    ASSERT_TRUE(home->isActive);

    // Bulldozing that road disconnects it without any update() call
    scene.placeTile(9, 10, TOOL_BULLDOZE);
    ASSERT_FALSE(home->isActive);

    // Building a road on another border tile (12, 10) reconnects it
    scene.placeTile(12, 10, TOOL_ROAD);
    ASSERT_TRUE(home->isActive);

    // update() no longer re-checks: a road removed behind the scene's back is not noticed
    delete entities[12][10]; entities[12][10] = NULL;
    home->update(0.016f);
    ASSERT_TRUE(home->isActive);

    clearTestGrid();
}

TEST_CASE(Test_SimulationTick_DrivesBuildings) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 50000;

    scene.placeTile(9, 10, TOOL_ROAD);
    scene.placeTile(10, 10, TOOL_RES_COTTAGES);
    Zone* home = dynamic_cast<Zone*>(entities[10][10]);
    Zone* sub = dynamic_cast<Zone*>(entities[11][10]);
    ASSERT_TRUE(home != NULL && home->isActive);
    ASSERT_TRUE(sub != NULL && sub->isSubTile);
    ASSERT_FALSE(sub->isActive); // sub-tiles sync on the next tick

    // Neither update() nor a simulation tick collects tax (that happens per game day)
    townTaxes[0] = 10;
    money = 1000;
    home->update(0.016f);
    scene.runSimulationTick();
    ASSERT_EQ(money, 1000);

    // The tick syncs the sub-tiles with the root
    ASSERT_TRUE(sub->isActive);

    money = 5000;
    clearTestGrid();
}

TEST_CASE(Test_Zone_BoundaryEdgeSafety) {
    clearTestGrid();
    ScopedCoutSilence silence;

    // Place zones along all 4 corners and borders
    delete entities[0][0];
    entities[0][0] = new ResidentalZone(0, 0);

    delete entities[19][0];
    entities[19][0] = new CommericalZone(19, 0);

    delete entities[0][19];
    entities[0][19] = new DirtRoad(0, 19);

    delete entities[19][19];
    entities[19][19] = new ResidentalZone(19, 19);

    // Must evaluate checkValid() without crashing or array out-of-bounds reads
    dynamic_cast<ResidentalZone*>(entities[0][0])->checkValid();
    dynamic_cast<CommericalZone*>(entities[19][0])->checkValid();
    dynamic_cast<DirtRoad*>(entities[0][19])->check();
    dynamic_cast<ResidentalZone*>(entities[19][19])->checkValid();

    ASSERT_TRUE(true);

    clearTestGrid();
}

// -------------------------------------------------------------
// 6. Taxation & Municipal Economy Tests
// -------------------------------------------------------------
TEST_CASE(Test_Economy_TaxationCycle) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;

    money = 500;
    townTaxes[0] = 10; // 10% residential tax
    townTaxes[1] = 15; // 15% commercial tax

    ResidentalZone* rz = new ResidentalZone(4, 4);
    entities[4][4] = rz;
    rz->zoneIncome = 100;
    rz->isActive = false;

    // Inactive zones pay nothing
    ASSERT_NEAR(rz->dailyTax(), 0.f, 0.001f);
    ASSERT_EQ(scene.collectDailyTaxes(), 0);
    ASSERT_EQ(money, 500);

    // Active residential: 100 x 10% = $10 per day
    rz->isActive = true;
    ASSERT_NEAR(rz->dailyTax(), 10.f, 0.001f);

    // Commercial uses its own rate: 200 x 15% = $30 per day
    CommericalZone* cz = new CommericalZone(8, 4);
    entities[8][4] = cz;
    cz->zoneIncome = 200;
    cz->isActive = true;
    ASSERT_NEAR(cz->dailyTax(), 30.f, 0.001f);

    // One day credits both, every day
    ASSERT_EQ(scene.collectDailyTaxes(), 40);
    ASSERT_EQ(money, 540);
    ASSERT_EQ(scene.getLastDailyIncome(), 40);
    ASSERT_EQ(scene.collectDailyTaxes(), 40);
    ASSERT_EQ(money, 580);

    townTaxes[0] = 5;
    townTaxes[1] = 5;
    clearTestGrid();
}

TEST_CASE(Test_Economy_DailyTaxCarriesFractions) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 0;
    townTaxes[0] = 5;

    // 30 x 5% = $1.50 per day: pays 1, then 2 (1.5 + carried 0.5), never loses cents
    ResidentalZone* rz = new ResidentalZone(4, 4);
    entities[4][4] = rz;
    rz->zoneIncome = 30;
    rz->isActive = true;
    ASSERT_EQ(scene.collectDailyTaxes(), 1);
    ASSERT_EQ(scene.collectDailyTaxes(), 2);
    ASSERT_EQ(scene.collectDailyTaxes(), 1);
    ASSERT_EQ(scene.collectDailyTaxes(), 2);
    ASSERT_EQ(money, 6);

    money = 5000;
    clearTestGrid();
}

TEST_CASE(Test_Time_AdvanceDrivesDaysAndPause) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 0;
    townTaxes[1] = 10;
    // A harbour has no tier changes, so its daily tax stays fixed: 120 x 10% = $12
    HarbourZone* hb = new HarbourZone(4, 4);
    entities[4][4] = hb;
    hb->isActive = true;
    ASSERT_NEAR(hb->dailyTax(), 12.f, 0.001f);

    // 1x: a day every 4 s
    scene.advanceTime(3.9f);
    ASSERT_EQ(money, 0);
    scene.advanceTime(0.2f);
    ASSERT_EQ(money, 12);
    ASSERT_EQ(day, 2);

    // Paused: nothing moves
    gameClock.togglePause();
    scene.advanceTime(100.f);
    ASSERT_EQ(money, 12);
    ASSERT_EQ(day, 2);

    // 4x: 4 s of real time = 4 days
    gameClock.setSpeedIndex(3);
    scene.advanceTime(4.f);
    ASSERT_EQ(money, 60);
    ASSERT_EQ(day, 6);

    // Speed buttons in the top bar
    sf::FloatRect b = scene.speedButtonBounds(0);
    ASSERT_TRUE(scene.handleSpeedClick(sf::Vector2f(b.left + 5.f, b.top + 5.f)));
    ASSERT_TRUE(gameClock.isPaused());
    b = scene.speedButtonBounds(2);
    ASSERT_TRUE(scene.handleSpeedClick(sf::Vector2f(b.left + 5.f, b.top + 5.f)));
    ASSERT_EQ(gameClock.getSpeedMultiplier(), 2);
    ASSERT_TRUE(b.top + b.height <= 48.f); // inside the top bar

    townTaxes[1] = 5;
    money = 5000;
    clearTestGrid();
}

// -------------------------------------------------------------
// 7. Population Immigration & Demand Tests
// -------------------------------------------------------------
TEST_CASE(Test_Economy_PopulationGrowthAndDemand) {
    townCapacity[0] = 100;
    townPopulation[0] = 0;
    townDemand[0] = 50;

    auto simulateImmigration = []() {
        for (int i = 0; i < 3; i++) {
            int k = townCapacity[i] - townPopulation[i];
            if (townDemand[i] <= 0 || k <= 0) continue;

            int inc = (k > 10) ? (std::rand() % (k / 10) + 1) : (std::rand() % k + 1);
            townPopulation[i] += inc;
            townDemand[i] -= inc;
        }
    };

    simulateImmigration();
    ASSERT_TRUE(townPopulation[0] > 0);
    ASSERT_TRUE(townDemand[0] < 50);
    ASSERT_EQ(townPopulation[0] + townDemand[0], 50); // Total conserved

    // If demand reaches zero, population must not grow
    townDemand[0] = 0;
    int popBefore = townPopulation[0];
    simulateImmigration();
    ASSERT_EQ(townPopulation[0], popBefore);

    // If capacity equals population, population must not grow
    townDemand[0] = 100;
    townCapacity[0] = townPopulation[0];
    simulateImmigration();
    ASSERT_EQ(townPopulation[0], popBefore);
}

// -------------------------------------------------------------
// 8. Road Autotiling Logic Tests
// -------------------------------------------------------------
TEST_CASE(Test_Roads_AutotilingLogic) {
    clearTestGrid();
    ScopedCoutSilence silence;

    delete entities[5][5];
    DirtRoad* road = new DirtRoad(5, 5);
    entities[5][5] = road;

    // Isolated road: check() runs safely
    road->check();
    ASSERT_TRUE(road->sprite != NULL);

    // Add roads to left and right: (4, 5) and (6, 5)
    delete entities[4][5];
    entities[4][5] = new DirtRoad(4, 5);
    delete entities[6][5];
    entities[6][5] = new DirtRoad(6, 5);

    // Now horizontal connection
    road->check();
    ASSERT_TRUE(road->sprite != NULL);

    // Test diagonal edge-sharing Axis 2 connection: (6, 4) [NE] and (4, 6) [SW]
    delete entities[6][4];
    entities[6][4] = new DirtRoad(6, 4);
    delete entities[4][6];
    entities[4][6] = new DirtRoad(4, 6);
    road->check();
    ASSERT_TRUE(road->sprite != NULL);

    clearTestGrid();
}

TEST_CASE(Test_Roads_NeighboursRetileOnChange) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 5000;

    // A lone road is a plain end cap (no rotation)
    scene.placeTile(10, 10, TOOL_ROAD);
    DirtRoad* a = dynamic_cast<DirtRoad*>(entities[10][10]);
    ASSERT_TRUE(a != NULL);
    ASSERT_NEAR(a->sprite->getRotation(), 0.f, 0.01f);

    // Building a road to its left re-tiles the existing road (left-only cap is rotated 180)
    scene.placeTile(9, 10, TOOL_ROAD);
    ASSERT_NEAR(a->sprite->getRotation(), 180.f, 0.01f);

    // update() no longer re-tiles: a road appearing without placeTile is not picked up
    delete entities[11][10]; entities[11][10] = new DirtRoad(11, 10);
    a->update(0.016f);
    ASSERT_NEAR(a->sprite->getRotation(), 180.f, 0.01f);
    delete entities[11][10]; entities[11][10] = NULL;

    // Bulldozing the neighbour re-tiles the survivor back to a plain cap
    scene.placeTile(9, 10, TOOL_BULLDOZE);
    ASSERT_TRUE(entities[9][10] == NULL);
    ASSERT_NEAR(a->sprite->getRotation(), 0.f, 0.01f);

    clearTestGrid();
}

// -------------------------------------------------------------
// 9. Industrial & Agriculture Zone Lifecycle Tests
// -------------------------------------------------------------
TEST_CASE(Test_IndustrialZone_LifecycleAndTiers) {
    clearTestGrid();
    ScopedCoutSilence silence;

    townCapacity[2] = 0;
    IndustrialZone* ind = new IndustrialZone(3, 3);
    entities[3][3] = ind;

    ASSERT_EQ(ind->getName(), "IndustrialZone");
    ASSERT_EQ(ind->tier, 1);
    ASSERT_TRUE(townCapacity[2] > 0);
    int initialCap = ind->capacity;

    // Connect to road
    delete entities[2][3];
    entities[2][3] = new DirtRoad(2, 3);
    ind->checkValid();
    ASSERT_TRUE(ind->isActive);

    // Tier 2 upgrade
    ind->upgradeTier(2);
    ASSERT_EQ(ind->tier, 2);
    ASSERT_TRUE(ind->capacity > initialCap);
    ASSERT_EQ(townCapacity[2], ind->capacity);

    // Tier 3 upgrade
    ind->upgradeTier(3);
    ASSERT_EQ(ind->tier, 3);

    // Demolish and verify capacity deduction
    delete entities[3][3];
    entities[3][3] = NULL;
    ASSERT_EQ(townCapacity[2], 0);

    clearTestGrid();
}

TEST_CASE(Test_FarmZone_RoadActivationAndTaxes) {
    clearTestGrid();
    ScopedCoutSilence silence;

    townCapacity[2] = 0;
    FarmZone* farm = new FarmZone(4, 4);
    entities[4][4] = farm;

    ASSERT_EQ(farm->getName(), "FarmZone");
    ASSERT_FALSE(farm->isActive);

    // Without road: checkValid keeps it inactive
    farm->checkValid();
    ASSERT_FALSE(farm->isActive);

    // Add adjacent dirt road
    delete entities[3][4];
    entities[3][4] = new DirtRoad(3, 4);
    farm->checkValid();
    ASSERT_TRUE(farm->isActive);

    // Active farm pays a daily tax at the industrial rate
    townTaxes[2] = 10;
    ASSERT_TRUE(farm->dailyTax() > 0.f);
    farm->isActive = false;
    ASSERT_NEAR(farm->dailyTax(), 0.f, 0.001f);
    townTaxes[2] = 5;

    clearTestGrid();
}

TEST_CASE(Test_HarbourZone_WaterAdjacency) {
    clearTestGrid();
    ScopedCoutSilence silence;

    townCapacity[1] = 0;
    HarbourZone* harbour = new HarbourZone(2, 2);
    entities[2][2] = harbour;

    ASSERT_EQ(harbour->getName(), "HarbourZone");
    ASSERT_FALSE(harbour->isActive);

    // Water on the east edge alone is not enough: a harbour also needs a road
    world[4][2].terrain = Terrain::Water;
    harbour->checkValid();
    ASSERT_FALSE(harbour->isActive);

    // Road on the west edge completes it
    entities[1][2] = new DirtRoad(1, 2);
    harbour->checkValid();
    ASSERT_TRUE(harbour->isActive);

    // Clean up
    delete entities[2][2];
    entities[2][2] = NULL;
    ASSERT_EQ(townCapacity[1], 0);

    clearTestGrid();
}

// -------------------------------------------------------------
// 10. Civic Services Radiuses & Coverage Tests
// -------------------------------------------------------------
TEST_CASE(Test_CivicServices_RadiusesAndTypes) {
    ScopedCoutSilence silence;

    SchoolZone school(1, 1);
    ASSERT_EQ(school.getName(), "SchoolZone");
    ASSERT_EQ(school.serviceRadius, 5);

    ChurchZone church(1, 1);
    ASSERT_EQ(church.getName(), "ChurchZone");
    ASSERT_EQ(church.serviceRadius, 4);

    ParkZone park(1, 1);
    ASSERT_EQ(park.getName(), "ParkZone");
    ASSERT_EQ(park.serviceRadius, 4);
    ASSERT_TRUE(park.isActive);
}

TEST_CASE(Test_HappinessSystem_CoverageMaps) {
    clearTestGrid();
    ScopedCoutSilence silence;

    HappinessSystem happySys;

    // Initially all coverage maps are 0
    happySys.updateCoverage();
    ASSERT_EQ(happySys.educationMap[10][10], 0);
    ASSERT_EQ(happySys.churchMap[10][10], 0);
    ASSERT_EQ(happySys.parkMap[10][10], 0);

    // Place active school at (10, 10)
    SchoolZone* school = new SchoolZone(10, 10);
    school->isActive = true;
    delete entities[10][10];
    entities[10][10] = school;

    // Place active park at (11, 10)
    ParkZone* park = new ParkZone(11, 10);
    delete entities[11][10];
    entities[11][10] = park;

    happySys.updateCoverage();

    // Check school coverage at center (10, 10) and radius 3 (13, 10)
    ASSERT_EQ(happySys.educationMap[10][10], 100);
    ASSERT_EQ(happySys.educationMap[13][10], 100);
    // Outside radius 5: (0, 0)
    ASSERT_EQ(happySys.educationMap[0][0], 0);

    // Check park coverage at (11, 10) and (12, 10)
    ASSERT_EQ(happySys.parkMap[11][10], 100);
    ASSERT_EQ(happySys.parkMap[12][10], 100);
    ASSERT_EQ(happySys.parkMap[0][0], 0);

    clearTestGrid();
}

TEST_CASE(Test_HappinessSystem_ScoringAndTierProgression) {
    clearTestGrid();
    ScopedCoutSilence silence;

    HappinessSystem happySys;

    // Setup active road (west edge) and residential zone
    delete entities[4][6];
    entities[4][6] = new DirtRoad(4, 6);

    delete entities[5][6];
    ResidentalZone* rz = new ResidentalZone(5, 6);
    entities[5][6] = rz;
    rz->checkValid();
    ASSERT_TRUE(rz->isActive);
    ASSERT_EQ(rz->tier, 1);

    // Place school and park in radius
    delete entities[6][6];
    SchoolZone* school = new SchoolZone(6, 6);
    school->isActive = true;
    entities[6][6] = school;

    delete entities[5][7];
    ParkZone* park = new ParkZone(5, 7);
    entities[5][7] = park;

    // Setup economy conditions: low taxes, balanced jobs
    townTaxes[0] = 5;
    townPopulation[0] = 30;
    townCapacity[1] = 40;
    townCapacity[2] = 40;

    happySys.updateSimulation();

    // Citizen happiness must be high due to school, park, low tax, and jobs
    ASSERT_TRUE(happySys.cityHappiness >= 60);
    ASSERT_TRUE(rz->overallHappiness >= 60);

    // Residental zone should have auto-upgraded to tier 2!
    ASSERT_EQ(rz->tier, 2);

    clearTestGrid();
}

TEST_CASE(Test_Toolbar_NewCategoriesAndToolNames) {
    Toolbar tb;
    tb.init(1500.f, 900.f);

    tb.setActiveTool(TOOL_FARM);
    ASSERT_EQ(tb.getActiveToolName(), "FARMS");

    tb.setActiveTool(TOOL_SCHOOL);
    ASSERT_EQ(tb.getActiveToolName(), "ELEMENTARY SCHOOL");

    tb.setActiveTool(TOOL_CHURCH);
    ASSERT_EQ(tb.getActiveToolName(), "COMMUNITY CHURCH");

    tb.setActiveTool(TOOL_PARK);
    ASSERT_EQ(tb.getActiveToolName(), "NATURE PARK");

    tb.setActiveTool(TOOL_HARBOUR);
    ASSERT_EQ(tb.getActiveToolName(), "TRADE HARBOUR");

    tb.setActiveTool(TOOL_WATER);
    ASSERT_EQ(tb.getActiveToolName(), "WATER BODY");

    tb.setActiveTool(TOOL_NONE);
    ASSERT_EQ(tb.getActiveToolName(), "INSPECT");
}

// -------------------------------------------------------------
// 11. Multi-Tile 2x2 Structures, Tiers, Variants & Demolition Tests
// -------------------------------------------------------------
TEST_CASE(Test_CivicServices_SchoolAndChurchProperties) {
    ScopedCoutSilence silence;

    SchoolZone school(3, 3);
    ASSERT_EQ(school.getName(), "SchoolZone");
    ASSERT_TRUE(school.isMultiTile);
    ASSERT_FALSE(school.isSubTile);
    ASSERT_EQ(school.footprintW, 2);
    ASSERT_EQ(school.footprintH, 2);
    ASSERT_EQ(school.serviceRadius, 5);
    ASSERT_FALSE(school.isActive);

    ChurchZone church(6, 6);
    ASSERT_EQ(church.getName(), "ChurchZone");
    ASSERT_TRUE(church.isMultiTile);
    ASSERT_FALSE(church.isSubTile);
    ASSERT_EQ(church.footprintW, 2);
    ASSERT_EQ(church.footprintH, 2);
    ASSERT_EQ(church.serviceRadius, 4);
    ASSERT_FALSE(church.isActive);

    ParkZone park(8, 8);
    ASSERT_EQ(park.getName(), "ParkZone");
    ASSERT_TRUE(park.isMultiTile);
    ASSERT_FALSE(park.isSubTile);
    ASSERT_EQ(park.footprintW, 2);
    ASSERT_EQ(park.footprintH, 2);
    ASSERT_EQ(park.serviceRadius, 4);
    ASSERT_TRUE(park.isActive);

    HarbourZone harbour(10, 10);
    ASSERT_EQ(harbour.getName(), "HarbourZone");
    ASSERT_TRUE(harbour.isMultiTile);
    ASSERT_FALSE(harbour.isSubTile);
    ASSERT_EQ(harbour.footprintW, 2);
    ASSERT_EQ(harbour.footprintH, 2);
}

TEST_CASE(Test_Zone_TiersVariantsAndHappinessProgression) {
    ScopedCoutSilence silence;

    ResidentalZone res(4, 4);
    ASSERT_TRUE(res.isMultiTile);
    ASSERT_EQ(res.footprintW, 2);
    ASSERT_EQ(res.footprintH, 2);
    ASSERT_EQ(res.tier, 1);
    ASSERT_TRUE(res.variant == 1 || res.variant == 2);
    ASSERT_EQ(res.zoneBaseTexture, "zone_residental");

    int initialCap = res.capacity;
    res.upgradeTier(2);
    ASSERT_EQ(res.tier, 2);
    ASSERT_TRUE(res.capacity > initialCap);
    ASSERT_TRUE(!res.getVariantName().empty());

    res.upgradeTier(3);
    ASSERT_EQ(res.tier, 3);
    ASSERT_TRUE(res.capacity > initialCap * 2);

    // Downgrade back to tier 1
    res.downgradeTier(1);
    ASSERT_EQ(res.tier, 1);
    ASSERT_EQ(res.capacity, 25);

    // Commercial zone tiers
    CommericalZone com(6, 6);
    ASSERT_EQ(com.tier, 1);
    ASSERT_EQ(com.zoneBaseTexture, "zone_commercial");
    com.upgradeTier(2);
    ASSERT_EQ(com.tier, 2);
    com.upgradeTier(3);
    ASSERT_EQ(com.tier, 3);

    // Industrial zone tiers
    IndustrialZone ind(8, 8);
    ASSERT_EQ(ind.tier, 1);
    ASSERT_EQ(ind.zoneBaseTexture, "zone_industrial");
    ind.upgradeTier(2);
    ASSERT_EQ(ind.tier, 2);
    ind.upgradeTier(3);
    ASSERT_EQ(ind.tier, 3);

    // Farm zone variants
    FarmZone farm(10, 10);
    ASSERT_TRUE(farm.variant == 1 || farm.variant == 2);
    ASSERT_EQ(farm.zoneBaseTexture, "zone_farm");
}

TEST_CASE(Test_Zone_SameBaseTextureAcrossSubZones) {
    ScopedCoutSilence silence;

    ResidentalZone res(2, 2);
    SubZone sub1(3, 2, 2, 2, "ResidentalZone", "zone_residental");
    SubZone sub2(1, 3, 2, 2, "ResidentalZone", "zone_residental");
    SubZone sub3(2, 3, 2, 2, "ResidentalZone", "zone_residental");

    ASSERT_EQ(res.zoneBaseTexture, "zone_residental");
    ASSERT_EQ(sub1.zoneBaseTexture, "zone_residental");
    ASSERT_EQ(sub2.zoneBaseTexture, "zone_residental");
    ASSERT_EQ(sub3.zoneBaseTexture, "zone_residental");

    ASSERT_TRUE(sub1.isMultiTile && sub1.isSubTile);
    ASSERT_TRUE(sub2.isMultiTile && sub2.isSubTile);
    ASSERT_TRUE(sub3.isMultiTile && sub3.isSubTile);
    ASSERT_EQ(sub1.rootX, 2);
    ASSERT_EQ(sub1.rootY, 2);
}

TEST_CASE(Test_Demolition_RestoresGrassTile) {
    clearTestGrid();
    ScopedCoutSilence silence;

    // Simulate 2x2 multi-tile placement at (4, 4) (True 2x2 Diamond)
    int rx = 4, ry = 4;
    const sf::Vector2i dOffsets[4] = {{0, 0}, {1, 0}, {-1, 1}, {0, 1}};
    for (int k = 0; k < 4; k++) {
        delete entities[rx + dOffsets[k].x][ry + dOffsets[k].y];
    }

    SchoolZone* school = new SchoolZone(rx, ry);
    entities[rx][ry] = school;
    entities[rx + 1][ry]     = new SubZone(rx + 1, ry,     rx, ry, "SchoolZone", "grass");
    entities[rx - 1][ry + 1] = new SubZone(rx - 1, ry + 1, rx, ry, "SchoolZone", "grass");
    entities[rx][ry + 1]     = new SubZone(rx,     ry + 1, rx, ry, "SchoolZone", "grass");

    // Road adjacent to perimeter at (6, 4)
    delete entities[6][4];
    entities[6][4] = new DirtRoad(6, 4);

    school->checkValid();
    ASSERT_TRUE(school->isActive);

    // Bulldoze one child tile at (4, 5) -> multi-tile demolition restores all 4 to GrassTile
    Zone* z = dynamic_cast<Zone*>(entities[rx][ry + 1]);
    ASSERT_TRUE(z != NULL && z->isMultiTile);
    int rootX = z->isSubTile ? z->rootX : rx;
    int rootY = z->isSubTile ? z->rootY : ry;
    ASSERT_EQ(rootX, rx);
    ASSERT_EQ(rootY, ry);

    for (int k = 0; k < 4; k++) {
        int cx = rootX + dOffsets[k].x;
        int cy = rootY + dOffsets[k].y;
        delete entities[cx][cy];
        entities[cx][cy] = NULL;
    }

    // Verify all 4 tiles are back to natural grass (empty structure slot)
    for (int k = 0; k < 4; k++) {
        ASSERT_TRUE(entities[rx + dOffsets[k].x][ry + dOffsets[k].y] == NULL);
        ASSERT_EQ(tileKind(rx + dOffsets[k].x, ry + dOffsets[k].y), "Grass");
    }

    clearTestGrid();
}

TEST_CASE(Test_World_FullBuildingCoverage) {
    clearTestGrid();
    ScopedCoutSilence silence;

    // Place 1x1 road and 2x2 multi-tile structures
    delete entities[2][2]; entities[2][2] = new DirtRoad(2, 2);

    int rx = 3, ry = 2;
    const sf::Vector2i dOffsets[4] = {{0, 0}, {1, 0}, {-1, 1}, {0, 1}};
    for (int k = 0; k < 4; k++) {
        delete entities[rx + dOffsets[k].x][ry + dOffsets[k].y];
    }

    ResidentalZone* rz = new ResidentalZone(rx, ry);
    entities[rx][ry] = rz;
    entities[rx + 1][ry]     = new SubZone(rx + 1, ry,     rx, ry, "ResidentalZone", "zone_residental");
    entities[rx - 1][ry + 1] = new SubZone(rx - 1, ry + 1, rx, ry, "ResidentalZone", "zone_residental");
    entities[rx][ry + 1]     = new SubZone(rx,     ry + 1, rx, ry, "ResidentalZone", "zone_residental");

    rz->checkValid();
    ASSERT_TRUE(rz->isActive);

    // Place water and adjacent Harbour
    world[10][1].terrain = Terrain::Water;           // east edge (+2, 0)
    entities[7][1] = new DirtRoad(7, 1);              // west edge (-1, 0)
    int hx = 8, hy = 1;
    for (int k = 0; k < 4; k++) {
        delete entities[hx + dOffsets[k].x][hy + dOffsets[k].y];
    }

    HarbourZone* hb = new HarbourZone(hx, hy);
    entities[hx][hy] = hb;
    entities[hx + 1][hy]     = new SubZone(hx + 1, hy,     hx, hy, "HarbourZone", "grass");
    entities[hx - 1][hy + 1] = new SubZone(hx - 1, hy + 1, hx, hy, "HarbourZone", "grass");
    entities[hx][hy + 1]     = new SubZone(hx,     hy + 1, hx, hy, "HarbourZone", "grass");

    hb->checkValid();
    ASSERT_TRUE(hb->isActive);

    clearTestGrid();
}

// -------------------------------------------------------------
// 14. TheoTown Placement Restrictions & Validation Tests
// -------------------------------------------------------------
TEST_CASE(Test_TheoTown_CannotBuildOnRoad) {
    clearTestGrid();
    ScopedCoutSilence silence;

    Play_Scene scene;
    money = 5000;

    // Place a road at (5, 5)
    delete entities[5][5];
    entities[5][5] = new DirtRoad(5, 5);

    // Try to place a 2x2 building at (5, 4) whose bottom diamond tile lands on (5, 5)
    std::string reason = "";
    bool canBuild = scene.canPlaceTile(5, 4, TOOL_RES_COTTAGES, &reason);
    ASSERT_FALSE(canBuild);
    ASSERT_TRUE(reason.find("Blocked by Road") != std::string::npos);

    // Try to place another road directly over existing road
    reason = "";
    bool canRoad = scene.canPlaceTile(5, 5, TOOL_ROAD, &reason);
    ASSERT_FALSE(canRoad);
    ASSERT_TRUE(reason.find("Road Already Built") != std::string::npos);

    clearTestGrid();
}

TEST_CASE(Test_TheoTown_CannotBuildOnWater) {
    clearTestGrid();
    ScopedCoutSilence silence;

    Play_Scene scene;
    money = 5000;

    // Place water at (6, 6)
    delete entities[6][6];
    entities[6][6] = NULL; world[6][6].terrain = Terrain::Water;

    // Try to place a 2x2 building at (6, 5) whose bottom diamond tile lands on (6, 6)
    std::string reason = "";
    bool canBuild = scene.canPlaceTile(6, 5, TOOL_COM_GROCERY, &reason);
    ASSERT_FALSE(canBuild);
    ASSERT_TRUE(reason.find("Blocked by Water") != std::string::npos);

    // Try to build a road on water
    reason = "";
    bool canRoad = scene.canPlaceTile(6, 6, TOOL_ROAD, &reason);
    ASSERT_FALSE(canRoad);
    ASSERT_TRUE(reason.find("Cannot Build Road on Water") != std::string::npos);

    clearTestGrid();
}

TEST_CASE(Test_TheoTown_RequiresRoadConnection) {
    clearTestGrid();
    ScopedCoutSilence silence;

    Play_Scene scene;
    money = 5000;

    // Grid is pure grass with zero roads
    std::string reason = "";
    bool canBuild = scene.canPlaceTile(5, 5, TOOL_RES_COTTAGES, &reason);
    ASSERT_FALSE(canBuild);
    ASSERT_TRUE(reason.find("Requires Road Connection") != std::string::npos);

    // Now place an adjacent road on the perimeter at (4, 5)
    delete entities[4][5];
    entities[4][5] = new DirtRoad(4, 5);

    // Now placement must be valid!
    reason = "";
    canBuild = scene.canPlaceTile(5, 5, TOOL_RES_COTTAGES, &reason);
    ASSERT_TRUE(canBuild);

    clearTestGrid();
}

TEST_CASE(Test_TheoTown_HarbourShorelineRules) {
    clearTestGrid();
    ScopedCoutSilence silence;

    Play_Scene scene;
    money = 5000;

    // 1. Place road at (4, 5), but NO water nearby
    delete entities[4][5];
    entities[4][5] = new DirtRoad(4, 5);

    std::string reason = "";
    bool canBuild = scene.canPlaceTile(5, 5, TOOL_HARBOUR, &reason);
    ASSERT_FALSE(canBuild);
    ASSERT_TRUE(reason.find("Harbour Must Touch Shoreline Water") != std::string::npos);

    // 2. Now place water at (7, 5) on the opposite perimeter
    delete entities[7][5];
    entities[7][5] = NULL; world[7][5].terrain = Terrain::Water;

    // Now harbour touches road at (4, 5) and touches water at (7, 5) -> Valid shoreline!
    reason = "";
    canBuild = scene.canPlaceTile(5, 5, TOOL_HARBOUR, &reason);
    ASSERT_TRUE(canBuild);

    clearTestGrid();
}

TEST_CASE(Test_TheoTown_InsufficientFundsAndBulldoze) {
    clearTestGrid();
    ScopedCoutSilence silence;

    Play_Scene scene;

    // Setup road connection
    delete entities[4][5];
    entities[4][5] = new DirtRoad(4, 5);

    // Set funds to $30 (Cottages cost $100)
    money = 30;
    std::string reason = "";
    bool canBuild = scene.canPlaceTile(5, 5, TOOL_RES_COTTAGES, &reason);
    ASSERT_FALSE(canBuild);
    ASSERT_TRUE(reason.find("Insufficient Funds") != std::string::npos);

    // Restore funds
    money = 5000;

    // Bulldozer: cannot bulldoze empty grass
    reason = "";
    bool canBulldozeGrass = scene.canPlaceTile(8, 8, TOOL_BULLDOZE, &reason);
    ASSERT_FALSE(canBulldozeGrass);
    ASSERT_TRUE(reason.find("Already Clear Ground") != std::string::npos);

    // Bulldozer: CAN bulldoze existing road
    reason = "";
    bool canBulldozeRoad = scene.canPlaceTile(4, 5, TOOL_BULLDOZE, &reason);
    ASSERT_TRUE(canBulldozeRoad);

    clearTestGrid();
}

// -------------------------------------------------------------
// 15. Procedural World Generation Tests
// -------------------------------------------------------------
static int countTerrain(Terrain t) {
    int n = 0;
    for (int i = 0; i < tileMapW; i++)
        for (int j = 0; j < tileMapH; j++)
            if (world[i][j].terrain == t) n++;
    return n;
}

static int countDeposit(Deposit d) {
    int n = 0;
    for (int i = 0; i < tileMapW; i++)
        for (int j = 0; j < tileMapH; j++)
            if (world[i][j].deposit == d) n++;
    return n;
}

TEST_CASE(Test_WorldGen_Deterministic) {
    generateWorld(12345u);
    static TileData snapshot[tileMapW][tileMapH];
    int spawnX = worldSpawnX, spawnY = worldSpawnY;
    for (int i = 0; i < tileMapW; i++)
        for (int j = 0; j < tileMapH; j++) snapshot[i][j] = world[i][j];

    // Same seed -> identical world, even if std::rand state changes in between
    std::srand(999);
    std::rand();
    generateWorld(12345u);
    bool identical = (spawnX == worldSpawnX && spawnY == worldSpawnY);
    for (int i = 0; i < tileMapW && identical; i++) {
        for (int j = 0; j < tileMapH; j++) {
            const TileData& a = snapshot[i][j];
            const TileData& b = world[i][j];
            if (a.terrain != b.terrain || a.fertility != b.fertility || a.groundwater != b.groundwater ||
                a.deposit != b.deposit || a.depositAmount != b.depositAmount) {
                identical = false;
                break;
            }
        }
    }
    ASSERT_TRUE(identical);

    // Different seed -> different terrain
    generateWorld(54321u);
    int diffs = 0;
    for (int i = 0; i < tileMapW; i++)
        for (int j = 0; j < tileMapH; j++)
            if (snapshot[i][j].terrain != world[i][j].terrain) diffs++;
    ASSERT_TRUE(diffs > 100);

    resetWorldData();
}

TEST_CASE(Test_WorldGen_TerrainMix) {
    const uint32_t seeds[5] = {1u, 42u, 777u, 2026u, 31337u};
    const int total = tileMapW * tileMapH;
    for (uint32_t seed : seeds) {
        generateWorld(seed);
        int water = countTerrain(Terrain::Water);
        ASSERT_TRUE(water >= total * 10 / 100);
        ASSERT_TRUE(water <= total * 45 / 100);
        ASSERT_TRUE(countTerrain(Terrain::Forest) > 0);
        ASSERT_TRUE(countTerrain(Terrain::Rock) > 0);
        ASSERT_TRUE(countTerrain(Terrain::Sand) > 0);
        ASSERT_TRUE(countTerrain(Terrain::Grass) > total / 4);
        ASSERT_TRUE(countDeposit(Deposit::Coal) > 0);
        ASSERT_TRUE(countDeposit(Deposit::Iron) > 0);
        ASSERT_TRUE(countDeposit(Deposit::Stone) > 0);
    }
    resetWorldData();
}

TEST_CASE(Test_WorldGen_ForestPathsExist) {
    // Paths are random walks, so check across several seeds rather than one
    const uint32_t seeds[5] = {1u, 42u, 777u, 2026u, 31337u};
    int withPaths = 0;
    for (uint32_t seed : seeds) {
        generateWorld(seed);
        if (countTerrain(Terrain::ForestPath) > 0) withPaths++;
    }
    ASSERT_TRUE(withPaths >= 4);
    resetWorldData();
}

TEST_CASE(Test_WorldGen_SpawnGuarantee) {
    const uint32_t seeds[6] = {3u, 99u, 404u, 8080u, 65535u, 123456789u};
    for (uint32_t seed : seeds) {
        generateWorld(seed);
        int sx = worldSpawnX, sy = worldSpawnY;

        // 8x8 spawn area is entirely open grassland
        bool allGrass = true;
        for (int i = sx - 4; i < sx + 4; i++)
            for (int j = sy - 4; j < sy + 4; j++)
                if (world[i][j].terrain != Terrain::Grass) allGrass = false;
        ASSERT_TRUE(allGrass);

        // Water reachable within 8 tiles (Manhattan) of the spawn centre
        bool waterNear = false;
        for (int i = sx - 8; i <= sx + 8 && !waterNear; i++)
            for (int j = sy - 8; j <= sy + 8; j++)
                if (inWorldBounds(i, j) && std::abs(i - sx) + std::abs(j - sy) <= 8 &&
                    world[i][j].terrain == Terrain::Water) { waterNear = true; break; }
        ASSERT_TRUE(waterNear);
    }
    resetWorldData();
}

TEST_CASE(Test_WorldGen_DataInvariants) {
    generateWorld(2468u);
    bool ok = true;
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            const TileData& t = world[i][j];
            if (t.fertility > 100 || t.groundwater > 100 || t.depositAmount > 100) ok = false;
            if (t.terrain == Terrain::Water && (t.fertility != 0 || t.groundwater != 0 || t.deposit != Deposit::None)) ok = false;
            if ((t.terrain == Terrain::Sand || t.terrain == Terrain::Rock) && t.fertility != 0) ok = false;
            if (t.deposit == Deposit::None && t.depositAmount != 0) ok = false;
        }
    }
    ASSERT_TRUE(ok);
    resetWorldData();
}

TEST_CASE(Test_WorldGen_NaturalGroundHasNoEntities) {
    ScopedCoutSilence silence;
    delete entities[3][3]; entities[3][3] = new DirtRoad(3, 3);
    generateWorld(777u);
    materializeWorld();

    // Natural ground is data only: no entities, kind comes from world[][]
    bool ok = true;
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            if (entities[i][j] != NULL) ok = false;
            if (tileKind(i, j) != groundKindName(world[i][j].terrain)) ok = false;
        }
    }
    ASSERT_TRUE(ok);

    // A structure's name takes precedence over the ground under it
    entities[3][3] = new DirtRoad(3, 3);
    ASSERT_EQ(tileKind(3, 3), "DirtRoad");
    ASSERT_EQ(tileKind(-1, 0), "");

    clearTestGrid();
}

TEST_CASE(Test_VisibleTileRange_MatchesBruteForce) {
    // Brute-force visibility, same rule as Play_Scene::renderWorld's isVisible
    auto visible = [](int i, int j, float l, float r, float t, float b, float margin) {
        sf::Vector2f p = tileScreenPos(i, j);
        return p.x + tileSizeX >= l && p.x <= r && p.y + tileSizeY + margin >= t && p.y <= b;
    };

    const float centres[5][2] = {{1050.f, 1500.f}, {0.f, 0.f}, {2016.f, 2016.f}, {-3000.f, 500.f}, {1000.f, 3500.f}};
    const float zooms[4] = {0.4f, 1.f, 2.f, 3.2f};
    const float margins[2] = {0.f, 140.f};
    bool complete = true, tight = true;

    for (auto& c : centres) {
        for (float z : zooms) {
            for (float m : margins) {
                float halfW = 750.f * z, halfH = 450.f * z;
                float l = c[0] - halfW - 64.f, r = c[0] + halfW + 64.f;
                float t = c[1] - halfH - 200.f, b = c[1] + halfH + 64.f;
                VisibleTiles v = visibleTileRange(l, r, t, b, m);
                for (int i = 0; i < tileMapW; i++) {
                    for (int j = 0; j < tileMapH; j++) {
                        bool inRange = v.contains(i, j);
                        // Every visible tile must be in the range
                        if (visible(i, j, l, r, t, b, m) && !inRange) complete = false;
                        // The range may only over-include tiles within one tile of the view
                        if (inRange && !visible(i, j, l - 64.f, r + 64.f, t - 64.f, b + 64.f, m)) tight = false;
                    }
                }
            }
        }
    }
    ASSERT_TRUE(complete);
    ASSERT_TRUE(tight);

    // A view entirely off the map yields no tiles
    VisibleTiles off = visibleTileRange(-9000.f, -8000.f, -9000.f, -8000.f, 140.f);
    ASSERT_TRUE(off.iMin > off.iMax);
    ASSERT_FALSE(off.contains(0, 0));
}

TEST_CASE(Test_GameClock_CalendarAndSeasons) {
    GameClock c;
    // Starts 1 Mar, Year 1 (spring), 1x
    ASSERT_EQ(c.getDay(), 1);
    ASSERT_EQ(c.getMonth(), 3);
    ASSERT_EQ(c.getYear(), 1);
    ASSERT_TRUE(c.getSeason() == Season::Spring);
    ASSERT_EQ(c.dateString(), "Mar 1, Year 1");

    // 4 s = 1 day
    ASSERT_EQ(c.advance(3.99f), 0);
    ASSERT_EQ(c.advance(0.02f), 1);
    ASSERT_EQ(c.getDay(), 2);

    // A long frame crosses several days at once
    ASSERT_EQ(c.advance(4.f * 10.f), 10);
    ASSERT_EQ(c.getDay(), 12);

    // 30 days -> next month; Dec -> Jan rolls the year
    c.setDate(1, 3, 30);
    c.advance(4.f);
    ASSERT_EQ(c.getMonth(), 4);
    ASSERT_EQ(c.getDay(), 1);
    c.setDate(1, 12, 30);
    c.advance(4.f);
    ASSERT_EQ(c.getMonth(), 1);
    ASSERT_EQ(c.getYear(), 2);

    // Jan/Feb of year 1 are before the start (March): roll to year 2, never negative
    c.setDate(1, 1, 15);
    ASSERT_EQ(c.getMonth(), 1);
    ASSERT_EQ(c.getDay(), 15);
    ASSERT_EQ(c.getYear(), 2);
    ASSERT_TRUE(c.getTotalDays() >= 0);

    // Season of every month
    const Season expected[12] = {Season::Winter, Season::Winter, Season::Spring, Season::Spring,
                                 Season::Spring, Season::Summer, Season::Summer, Season::Summer,
                                 Season::Autumn, Season::Autumn, Season::Autumn, Season::Winter};
    for (int m = 1; m <= 12; m++) {
        ASSERT_TRUE(GameClock::seasonOfMonth(m) == expected[m - 1]);
    }

    // Season progress runs 0 -> 1 across the three months
    c.setDate(2, 6, 1);
    ASSERT_NEAR(c.seasonProgress(), 0.f, 0.001f);
    c.setDate(2, 7, 16);
    ASSERT_NEAR(c.seasonProgress(), 45.f / 90.f, 0.001f);
    c.setDate(2, 8, 30);
    ASSERT_NEAR(c.seasonProgress(), 89.f / 90.f, 0.001f);
}

TEST_CASE(Test_GameClock_SpeedAndPause) {
    GameClock c;
    ASSERT_EQ(c.getSpeedMultiplier(), 1);

    c.faster(); ASSERT_EQ(c.getSpeedMultiplier(), 2);
    c.faster(); ASSERT_EQ(c.getSpeedMultiplier(), 4);
    c.faster(); ASSERT_EQ(c.getSpeedMultiplier(), 4); // capped

    // 4x: 1 real second = 1 day
    ASSERT_EQ(c.advance(1.f), 1);
    ASSERT_NEAR(c.scaledDelta(0.5f), 2.f, 0.001f);

    // Pause freezes time and resumes at the previous speed
    c.togglePause();
    ASSERT_TRUE(c.isPaused());
    ASSERT_EQ(c.advance(1000.f), 0);
    ASSERT_NEAR(c.scaledDelta(1.f), 0.f, 0.001f);
    c.togglePause();
    ASSERT_EQ(c.getSpeedMultiplier(), 4);

    c.slower(); c.slower(); c.slower();
    ASSERT_TRUE(c.isPaused()); // slowest step is pause
}

TEST_CASE(Test_Farm_SeasonalYield) {
    clearTestGrid();
    ScopedCoutSilence silence;
    townTaxes[2] = 10;
    FarmZone* farm = new FarmZone(4, 4, 1);
    entities[4][4] = farm;
    farm->isActive = true;

    gameClock.setDate(1, 4, 1);  float spring = farm->dailyTax();
    gameClock.setDate(1, 7, 1);  float summer = farm->dailyTax();
    gameClock.setDate(1, 10, 1); float autumn = farm->dailyTax();
    gameClock.setDate(1, 1, 1);  float winter = farm->dailyTax();

    // Harvest peaks in autumn, fields rest in winter
    ASSERT_TRUE(autumn > summer);
    ASSERT_TRUE(summer > spring);
    ASSERT_TRUE(spring > winter);
    ASSERT_NEAR(autumn / spring, 1.5f, 0.01f);
    ASSERT_NEAR(winter / spring, 0.3f, 0.01f);

    townTaxes[2] = 5;
    clearTestGrid();
}

TEST_CASE(Test_GroundRenderer_SeasonLook) {
    generateWorld(42u);
    GroundRenderer gr;
    gr.prepare();
    ASSERT_EQ(static_cast<int>(gr.snowVertexCount()), 0); // summer default: no snow

    // Mid-winter: snow on land tiles only, ground rebuilt
    gr.setSeasonLook(seasonLookAt(Season::Winter, 0.5f));
    ASSERT_TRUE(gr.isDirty());
    gr.prepare();
    int land = 0;
    for (int i = 0; i < tileMapW; i++)
        for (int j = 0; j < tileMapH; j++)
            if (world[i][j].terrain != Terrain::Water) land++;
    ASSERT_EQ(static_cast<int>(gr.snowVertexCount()), land * 6);

    // Same look again: no rebuild needed
    gr.setSeasonLook(seasonLookAt(Season::Winter, 0.5f));
    ASSERT_FALSE(gr.isDirty());

    // Trees: green in spring/summer, autumn colours, snowy in winter
    ASSERT_EQ(seasonLookAt(Season::Spring, 0.3f).treeVariant, 0);
    ASSERT_EQ(seasonLookAt(Season::Summer, 0.3f).treeVariant, 0);
    SeasonLook autumn = seasonLookAt(Season::Autumn, 0.3f);
    ASSERT_EQ(autumn.treeVariant, 1);
    ASSERT_EQ(static_cast<int>(autumn.snow), 0);
    ASSERT_EQ(seasonLookAt(Season::Winter, 0.3f).treeVariant, 2);

    // Late autumn blends toward winter: some snow starts to settle
    SeasonLook lateAutumn = seasonLookAt(Season::Autumn, 0.95f);
    ASSERT_TRUE(lateAutumn.snow > 0 && lateAutumn.snow < 120);

    resetWorldData();
}

TEST_CASE(Test_GroundRenderer_OneQuadPerTile) {
    generateWorld(42u);
    GroundRenderer gr;
    ASSERT_TRUE(gr.isDirty());
    gr.prepare();
    ASSERT_FALSE(gr.isDirty());
    ASSERT_EQ(static_cast<int>(gr.vertexCount()), tileMapW * tileMapH * 6);

    // Terrain edits flag a rebuild
    gr.markDirty();
    ASSERT_TRUE(gr.isDirty());
    gr.prepare();
    ASSERT_FALSE(gr.isDirty());
    resetWorldData();
}

TEST_CASE(Test_Placement_ForestAndRockBlock) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 5000;

    // Road at (4,6) so the 2x2 lot at (5,4) would otherwise be valid
    delete entities[4][6]; entities[4][6] = new DirtRoad(4, 6);

    world[5][5].terrain = Terrain::Forest;

    std::string reason;
    ASSERT_FALSE(scene.canPlaceTile(5, 4, TOOL_RES_COTTAGES, &reason));
    ASSERT_TRUE(reason.find("Forest") != std::string::npos);

    reason = "";
    ASSERT_FALSE(scene.canPlaceTile(5, 5, TOOL_ROAD, &reason));
    ASSERT_TRUE(reason.find("Forest") != std::string::npos);

    world[10][10].terrain = Terrain::Rock;
    reason = "";
    ASSERT_FALSE(scene.canPlaceTile(10, 10, TOOL_ROAD, &reason));
    ASSERT_TRUE(reason.find("Rocky") != std::string::npos);

    // Clearing forest costs $15 and leaves grassland
    ASSERT_EQ(scene.getBulldozeCost(5, 5), 15);
    ASSERT_EQ(scene.getBulldozeCost(10, 10), 60);
    money = 1000;
    ASSERT_TRUE(scene.canPlaceTile(5, 5, TOOL_BULLDOZE));
    scene.placeTile(5, 5, TOOL_BULLDOZE);
    ASSERT_EQ(money, 985);
    ASSERT_TRUE(entities[5][5] == NULL);
    ASSERT_TRUE(world[5][5].terrain == Terrain::Grass);
    ASSERT_TRUE(scene.canPlaceTile(5, 4, TOOL_RES_COTTAGES, &reason));

    // Sand and forest paths are buildable ground
    world[20][20].terrain = Terrain::Sand;
    ASSERT_TRUE(scene.canPlaceTile(20, 20, TOOL_ROAD));
    world[21][20].terrain = Terrain::ForestPath;
    ASSERT_TRUE(scene.canPlaceTile(21, 20, TOOL_ROAD));
    reason = "";
    ASSERT_FALSE(scene.canPlaceTile(20, 20, TOOL_BULLDOZE, &reason));
    ASSERT_TRUE(reason.find("Already Clear Ground") != std::string::npos);

    // Insufficient funds uses the terrain-specific bulldoze cost
    money = 30;
    reason = "";
    ASSERT_FALSE(scene.canPlaceTile(10, 10, TOOL_BULLDOZE, &reason));
    ASSERT_TRUE(reason.find("$60") != std::string::npos);

    money = 5000;
    clearTestGrid();
}

TEST_CASE(Test_Demolition_RestoresTerrainBase) {
    clearTestGrid();
    ScopedCoutSilence silence;

    Play_Scene scene;
    money = 5000;

    world[6][6].terrain = Terrain::Sand;
    world[7][6].terrain = Terrain::ForestPath;

    // Roads may be built on sand and forest paths
    scene.placeTile(6, 6, TOOL_ROAD);
    scene.placeTile(7, 6, TOOL_ROAD);
    ASSERT_EQ(entities[6][6]->getName(), "DirtRoad");
    ASSERT_EQ(entities[7][6]->getName(), "DirtRoad");

    // Demolishing restores the natural ground, not hardcoded grass
    scene.placeTile(6, 6, TOOL_BULLDOZE);
    scene.placeTile(7, 6, TOOL_BULLDOZE);
    ASSERT_TRUE(entities[6][6] == NULL);
    ASSERT_TRUE(entities[7][6] == NULL);
    ASSERT_EQ(tileKind(6, 6), "Sand");
    ASSERT_EQ(tileKind(7, 6), "ForestPath");

    clearTestGrid();
}

TEST_CASE(Test_MapView_SelectorButtons) {
    Play_Scene scene;

    // Starts on the normal terrain view
    ASSERT_EQ(scene.getOverlayMode(), 0);
    ASSERT_EQ(scene.getOverlayName(), "Normal");

    // Each panel button selects its view
    const char* expected[4] = {"Normal", "Fertility", "Groundwater", "Deposits"};
    for (int m = 3; m >= 0; m--) {
        sf::FloatRect b = scene.viewButtonBounds(m);
        sf::Vector2f centre(b.left + b.width / 2.f, b.top + b.height / 2.f);
        ASSERT_TRUE(scene.viewPanelBounds().contains(centre));
        ASSERT_TRUE(scene.handleViewPanelClick(centre));
        ASSERT_EQ(scene.getOverlayMode(), m);
        ASSERT_EQ(scene.getOverlayName(), std::string(expected[m]));
    }

    // Buttons do not overlap, and clicks outside them change nothing
    ASSERT_FALSE(scene.viewButtonBounds(0).intersects(scene.viewButtonBounds(1)));
    scene.setOverlayMode(2);
    ASSERT_FALSE(scene.handleViewPanelClick(sf::Vector2f(750.f, 450.f)));
    ASSERT_EQ(scene.getOverlayMode(), 2);

    // Panel stays clear of the top bar and bottom dock
    Toolbar tb;
    tb.init(1500.f, 900.f);
    sf::FloatRect panel = scene.viewPanelBounds();
    ASSERT_TRUE(panel.top > 50.f);
    ASSERT_FALSE(tb.contains(sf::Vector2f(panel.left, panel.top + panel.height)));

    scene.setOverlayMode(99); // invalid -> normal
    ASSERT_EQ(scene.getOverlayMode(), 0);
}

TEST_CASE(Test_Farm_FertilityScalesIncome) {
    clearTestGrid();
    ScopedCoutSilence silence;
    townCapacity[2] = 0;

    // Default soil (50) -> 1.0x
    FarmZone* average = new FarmZone(4, 4, 1);
    ASSERT_EQ(average->zoneIncome, 35);
    delete average;

    const int offs[4][2] = {{0, 0}, {1, 0}, {-1, 1}, {0, 1}};
    for (auto& o : offs) world[10 + o[0]][10 + o[1]].fertility = 100;
    for (auto& o : offs) world[20 + o[0]][20 + o[1]].fertility = 0;

    FarmZone* rich = new FarmZone(10, 10, 1);
    FarmZone* barren = new FarmZone(20, 20, 1);
    ASSERT_TRUE(rich->zoneIncome > 35);
    ASSERT_TRUE(barren->zoneIncome < 35);
    ASSERT_TRUE(rich->fertilityMul > barren->fertilityMul);
    delete rich;
    delete barren;

    townCapacity[2] = 0;
    clearTestGrid();
}

// -------------------------------------------------------------
// Main Test Runner Entrypoint
// -------------------------------------------------------------
int main() {
    // Setup minimal environment globals for tests
    m_textures["grass"] = sf::Texture();
    m_textures["player"] = sf::Texture();
    m_textures["road"] = sf::Texture();
    m_textures["road-mid"] = sf::Texture();
    m_textures["residental"] = sf::Texture();
    m_textures["commercial"] = sf::Texture();
    m_textures["industrial"] = sf::Texture();
    m_textures["farm"] = sf::Texture();
    m_textures["harbour"] = sf::Texture();
    m_textures["water"] = sf::Texture();
    m_textures["school"] = sf::Texture();
    m_textures["church"] = sf::Texture();
    m_textures["park"] = sf::Texture();

    // Zone bases
    m_textures["zone_residental"] = sf::Texture();
    m_textures["zone_commercial"] = sf::Texture();
    m_textures["zone_industrial"] = sf::Texture();
    m_textures["zone_farm"] = sf::Texture();

    // Tiers & Variants
    m_textures["res_t1_v1"] = sf::Texture();
    m_textures["res_t1_v2"] = sf::Texture();
    m_textures["res_t2_v1"] = sf::Texture();
    m_textures["res_t2_v2"] = sf::Texture();
    m_textures["res_t3_v1"] = sf::Texture();
    m_textures["res_t3_v2"] = sf::Texture();

    m_textures["com_t1_v1"] = sf::Texture();
    m_textures["com_t1_v2"] = sf::Texture();
    m_textures["com_t2_v1"] = sf::Texture();
    m_textures["com_t2_v2"] = sf::Texture();
    m_textures["com_t3_v1"] = sf::Texture();
    m_textures["com_t3_v2"] = sf::Texture();

    m_textures["ind_t1_v1"] = sf::Texture();
    m_textures["ind_t1_v2"] = sf::Texture();
    m_textures["ind_t2_v1"] = sf::Texture();
    m_textures["ind_t2_v2"] = sf::Texture();
    m_textures["ind_t3_v1"] = sf::Texture();
    m_textures["ind_t3_v2"] = sf::Texture();

    m_textures["farm_v1"] = sf::Texture();
    m_textures["farm_v2"] = sf::Texture();

    return TestRegistry::instance().runAll();
}
