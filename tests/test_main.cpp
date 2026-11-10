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
#include "../game/simulation/Economy.hpp"

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
    resetTaxRates();         // every bucket back to 9%
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            delete entities[i][j];
            entities[i][j] = NULL;
        }
    }
    roadNet.clear();
    traffic.reset();
    utilities.reset();
    services.reset();
    trade.reset();
}

// Dirt road from the west border tile of a 2x2 building rooted at (x, y)
// to the map edge, so the building has an outside connection
static void connectToOutside(int x, int y) {
    for (int i = 0; i <= x - 1; i++) {
        delete entities[i][y];
        entities[i][y] = new DirtRoad(i, y);
    }
    roadNet.rebuild();
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

TEST_CASE(Test_HUD_AdaptsToWindowSize) {
    ScopedCoutSilence silence;
    Play_Scene scene;
    Toolbar& tb = scene.toolbar;
    tb.init(1500.f, 900.f);
    auto centre = [](const sf::FloatRect& r) {
        return sf::Vector2i(static_cast<int>(r.left + r.width / 2.f), static_cast<int>(r.top + r.height / 2.f));
    };

    // Maximised 1920x1020: panels re-anchor to the new edges instead of stretching
    scene.onResize(1920, 1020);
    ASSERT_EQ(screenW, 1920);
    ASSERT_EQ(screenH, 1020);
    ASSERT_NEAR(scene.viewPanelBounds().left, 1920.f - 452.f, 0.01f);           // top-right
    ASSERT_NEAR(tb.pillBounds().top + tb.pillBounds().height, 1020.f - 12.f, 0.01f); // bottom-left
    sf::FloatRect budget = scene.budgetBounds();
    ASSERT_NEAR(budget.left + budget.width / 2.f, 960.f, 0.5f);                  // centred
    ASSERT_NEAR(view.getSize().x / view.getSize().y, 1920.f / 1020.f, 0.001f);    // map not stretched

    // HUD coordinates are window pixels: a click where a button is drawn hits it
    const sf::Vector2u big(1920, 1020);
    for (int i = 0; i < GameClock::SPEED_COUNT; i++) {
        sf::FloatRect b = scene.speedButtonBounds(i);
        ASSERT_TRUE(b.contains(uikit::hudFromPixel(centre(b), big)));
    }
    sf::FloatRect roads = tb.tabBounds(ToolbarCategory::INFRASTRUCTURE);
    ASSERT_TRUE(tb.handleClick(uikit::hudFromPixel(centre(roads), big)));
    ASSERT_TRUE(tb.getActiveCategory() == ToolbarCategory::INFRASTRUCTURE);
    ASSERT_TRUE(tb.handleClick(uikit::hudFromPixel(centre(tb.cardBoundsFor(TOOL_ROAD)), big)));
    ASSERT_EQ(static_cast<int>(tb.getActiveTool()), static_cast<int>(TOOL_ROAD));

    // A frame drawn before the resize event is handled still maps correctly
    sf::Vector2f early = uikit::hudFromPixel(sf::Vector2i(1920, 1020), sf::Vector2u(3840, 2040));
    ASSERT_NEAR(early.x, 960.f, 0.01f);
    ASSERT_NEAR(early.y, 510.f, 0.01f);

    // Narrow window: top bar drops the city name so the speed buttons still fit
    scene.onResize(1100, 700);
    sf::FloatRect last = scene.speedButtonBounds(GameClock::SPEED_COUNT - 1);
    ASSERT_TRUE(last.left + last.width <= 1100.f);
    ASSERT_TRUE(scene.moneyChipBounds().left >= 0.f);
    ASSERT_TRUE(scene.budgetBounds().top >= 48.f);

    scene.onResize(WINDOW_W, WINDOW_H);
    ASSERT_EQ(screenW, WINDOW_W);
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
    // Every step of a road drag shares a diamond edge with the previous tile
    auto edgeStep = [](sf::Vector2i a, sf::Vector2i b) {
        for (const auto& d : ROAD_DIR)
            if (b.x - a.x == d[0] && b.y - a.y == d[1]) return true;
        return false;
    };

    // Along x (SE): (3, 2) to (7, 2) -> 5 tiles in a straight line
    auto hRoad = roadDragPath({3, 2}, {7, 2});
    ASSERT_EQ(hRoad.size(), 5);
    for (size_t i = 0; i < hRoad.size(); i++) {
        ASSERT_EQ(hRoad[i].x, 3 + static_cast<int>(i));
        ASSERT_EQ(hRoad[i].y, 2);
    }

    // Along NE (+1, -1): (5, 8) to (8, 5) -> 4 tiles
    auto dRoad = roadDragPath({5, 8}, {8, 5});
    ASSERT_EQ(dRoad.size(), 4);
    ASSERT_EQ(dRoad[3].x, 8); ASSERT_EQ(dRoad[3].y, 5);

    // Any drag (here straight down the screen, which is tip-touching) becomes
    // an edge-connected L that ends exactly on the target
    for (int sx = 2; sx < 9; sx += 3)
        for (int ex = 0; ex < 12; ex += 2)
            for (int ey = 0; ey < 12; ey += 3) {
                auto path = roadDragPath({sx, 6}, {ex, ey});
                ASSERT_EQ(path.front().x, sx);
                ASSERT_EQ(path.back().x, ex);
                ASSERT_EQ(path.back().y, ey);
                for (size_t k = 1; k < path.size(); k++) ASSERT_TRUE(edgeStep(path[k - 1], path[k]));
            }

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

    // Tax = occupants x income per occupant x that bucket's rate
    ResidentalZone* rz = new ResidentalZone(4, 4, 1, 1); // R $
    entities[4][4] = rz;
    rz->population = 20;
    rz->isActive = false;
    ASSERT_NEAR(rz->dailyTax(), 0.f, 0.001f); // no road -> no tax

    rz->isActive = true;
    taxRates[SECTOR_R][WEALTH_LOW] = 10;
    ASSERT_NEAR(rz->dailyTax(), 20.f * INCOME_PER_CAPITA[SECTOR_R][WEALTH_LOW] * 0.10f, 0.001f);

    // Empty buildings pay nothing
    rz->population = 0;
    ASSERT_NEAR(rz->dailyTax(), 0.f, 0.001f);

    // Each income level has its own slider and richer occupants pay more
    ResidentalZone* rich = new ResidentalZone(10, 4, 3, 1); // R $$$
    entities[10][4] = rich;
    rich->isActive = true;
    rich->population = 20;
    ASSERT_EQ(rich->wealth(), static_cast<int>(WEALTH_HIGH));
    taxRates[SECTOR_R][WEALTH_HIGH] = 5;
    float before = rich->dailyTax();
    ASSERT_NEAR(before, 20.f * INCOME_PER_CAPITA[SECTOR_R][WEALTH_HIGH] * 0.05f, 0.001f);
    taxRates[SECTOR_R][WEALTH_LOW] = 20; // changing R $ does not touch R $$$
    ASSERT_NEAR(rich->dailyTax(), before, 0.001f);

    // Commercial uses the commercial row of sliders
    CommericalZone* cz = new CommericalZone(16, 4, 2, 1); // C $$
    entities[16][4] = cz;
    cz->isActive = true;
    cz->population = 10;
    taxRates[SECTOR_C][WEALTH_MID] = 15;
    ASSERT_NEAR(cz->dailyTax(), 10.f * INCOME_PER_CAPITA[SECTOR_C][WEALTH_MID] * 0.15f, 0.001f);

    clearTestGrid();
}
TEST_CASE(Test_Economy_DailyTaxCarriesFractions) {
    clearTestGrid();
    ScopedCoutSilence silence;
    EconomySystem econ;
    money = 0;

    // 3 residents x $4 x 10% = $1.20 per day: credits 1, 1, 1, 1, 2 (no lost cents)
    taxRates[SECTOR_R][WEALTH_LOW] = 10;
    ResidentalZone* rz = new ResidentalZone(4, 4, 1, 1);
    entities[4][4] = rz;
    rz->isActive = true;
    rz->population = 3;
    ASSERT_EQ(econ.settle().net, 1);
    ASSERT_EQ(econ.settle().net, 1);
    ASSERT_EQ(econ.settle().net, 1);
    ASSERT_EQ(econ.settle().net, 1);
    ASSERT_EQ(econ.settle().net, 2);
    ASSERT_EQ(money, 6);

    money = 5000;
    clearTestGrid();
}
TEST_CASE(Test_Time_AdvanceDrivesDaysAndPause) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 100;
    // One road costs $1 upkeep per day: an exact day counter
    entities[4][4] = new DirtRoad(4, 4);

    // 1x: a day every 4 s
    scene.advanceTime(3.9f);
    ASSERT_EQ(money, 100);
    scene.advanceTime(0.2f);
    ASSERT_EQ(money, 99);
    ASSERT_EQ(day, 2);
    ASSERT_EQ(scene.getLastDailyIncome(), -1);

    // Paused: nothing moves
    gameClock.togglePause();
    scene.advanceTime(100.f);
    ASSERT_EQ(money, 99);
    ASSERT_EQ(day, 2);

    // 4x: 4 s of real time = 4 days
    gameClock.setSpeedIndex(3);
    scene.advanceTime(4.f);
    ASSERT_EQ(money, 95);
    ASSERT_EQ(day, 6);

    // Speed buttons in the top bar
    sf::FloatRect b = scene.speedButtonBounds(0);
    ASSERT_TRUE(scene.handleSpeedClick(sf::Vector2f(b.left + 5.f, b.top + 5.f)));
    ASSERT_TRUE(gameClock.isPaused());
    b = scene.speedButtonBounds(2);
    ASSERT_TRUE(scene.handleSpeedClick(sf::Vector2f(b.left + 5.f, b.top + 5.f)));
    ASSERT_EQ(gameClock.getSpeedMultiplier(), 2);
    ASSERT_TRUE(b.top + b.height <= 48.f); // inside the top bar

    money = 5000;
    clearTestGrid();
}
TEST_CASE(Test_Economy_PopulationGrowthAndDemand) {
    clearTestGrid();
    ScopedCoutSilence silence;
    EconomySystem econ;

    ResidentalZone* home = new ResidentalZone(4, 4, 1, 1);
    entities[4][4] = home;
    ResidentalZone* cutOff = new ResidentalZone(10, 4, 1, 1);
    entities[10][4] = cutOff;
    home->isActive = true;
    cutOff->isActive = false; // no road
    connectToOutside(4, 4);

    // Positive R $ demand: the connected home fills over a few days, never past capacity
    econ.demand[SECTOR_R][WEALTH_LOW] = 60.f;
    for (int d = 0; d < 30; d++) econ.updateOccupancy();
    ASSERT_EQ(home->population, home->capacity);
    ASSERT_EQ(cutOff->population, 0);

    // Strongly negative demand: people move out
    econ.demand[SECTOR_R][WEALTH_LOW] = -80.f;
    econ.updateOccupancy();
    ASSERT_TRUE(home->population < home->capacity);

    // Losing road access empties a building
    home->isActive = false;
    for (int d = 0; d < 10; d++) econ.updateOccupancy();
    ASSERT_EQ(home->population, 0);

    // Scan totals feed the city-wide counters
    home->population = 7;
    econ.scan();
    ASSERT_EQ(econ.occupants[SECTOR_R][WEALTH_LOW], 7);
    ASSERT_EQ(townPopulation[0], 7);
    ASSERT_EQ(townCapacity[0], home->capacity + cutOff->capacity);

    clearTestGrid();
}
TEST_CASE(Test_Roads_AutotilingLogic) {
    clearTestGrid();
    ScopedCoutSilence silence;

    // Every combination of the 4 edge neighbours picks its own frame
    for (int m = 0; m < 16; m++) {
        clearTestGrid();
        entities[5][5] = new DirtRoad(5, 5);
        for (int d = 0; d < 4; d++) {
            if (!(m & (1 << d))) continue;
            int x = 5 + ROAD_DIR[d][0], y = 5 + ROAD_DIR[d][1];
            entities[x][y] = new DirtRoad(x, y);
        }
        // Tip-touching neighbours (0, +-1) never connect
        entities[5][4] = new DirtRoad(5, 4);
        entities[5][6] = new DirtRoad(5, 6);
        RoadTile* road = RoadTile::at(5, 5);
        road->check();
        ASSERT_EQ(road->mask, m);
        ASSERT_EQ(road->frame(), m);
    }
    clearTestGrid();
}

TEST_CASE(Test_Roads_NeighboursRetileOnChange) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 5000;

    // A lone road is a dead-end pad (mask 0)
    scene.placeTile(10, 10, TOOL_ROAD);
    RoadTile* a = RoadTile::at(10, 10);
    ASSERT_TRUE(a != NULL);
    ASSERT_EQ(a->frame(), 0);

    // Building a road on its north-west edge re-tiles the existing road
    scene.placeTile(9, 10, TOOL_ROAD);
    ASSERT_EQ(a->frame(), 8);

    // update() does not re-tile: a road appearing without placeTile is not picked up
    delete entities[11][10]; entities[11][10] = new DirtRoad(11, 10);
    a->update(0.016f);
    ASSERT_EQ(a->frame(), 8);
    delete entities[11][10]; entities[11][10] = NULL;

    // Bulldozing the neighbour re-tiles the survivor back to a pad
    scene.placeTile(9, 10, TOOL_BULLDOZE);
    ASSERT_TRUE(entities[9][10] == NULL);
    ASSERT_EQ(a->frame(), 0);

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

    // An occupied, connected farm pays at the industrial $ rate
    ASSERT_EQ(farm->sector(), static_cast<int>(SECTOR_I));
    ASSERT_EQ(farm->wealth(), static_cast<int>(WEALTH_LOW));
    farm->population = 10;
    ASSERT_TRUE(farm->dailyTax() > 0.f);
    farm->isActive = false;
    ASSERT_NEAR(farm->dailyTax(), 0.f, 0.001f);

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
    for (int w = 0; w < 3; w++) taxRates[SECTOR_R][w] = 5;
    townPopulation[0] = 30;
    townCapacity[1] = 40;
    townCapacity[2] = 40;

    happySys.updateSimulation();

    // Citizen happiness must be high due to school, park, low tax, and jobs
    ASSERT_TRUE(happySys.cityHappiness >= 60);
    ASSERT_TRUE(rz->overallHappiness >= 60);

    // Happiness alone no longer upgrades buildings (that needs demand, see Test_Economy_UpgradeNeedsDemand)
    ASSERT_EQ(rz->tier, 1);

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

TEST_CASE(Test_Economy_DemandDrivers) {
    clearTestGrid();
    EconomySystem econ;

    // Empty town: low-income homes wanted, high-income needs schools first
    econ.computeDemand();
    ASSERT_TRUE(econ.demand[SECTOR_R][WEALTH_LOW] > 0.f);
    ASSERT_TRUE(econ.demand[SECTOR_R][WEALTH_HIGH] < 0.f);

    // Jobs pull residents: open job capacity raises R demand
    econ.occupants[SECTOR_R][WEALTH_LOW] = 200;
    econ.computeDemand();
    float noJobs = econ.demand[SECTOR_R][WEALTH_LOW];
    econ.capacity[SECTOR_I][WEALTH_LOW] = 300;
    econ.computeDemand();
    ASSERT_TRUE(econ.demand[SECTOR_R][WEALTH_LOW] > noJobs);

    // Residents create customers: C $ demand falls as shops fill up
    float unserved = econ.demand[SECTOR_C][WEALTH_LOW];
    econ.occupants[SECTOR_C][WEALTH_LOW] = 120;
    econ.computeDemand();
    ASSERT_TRUE(econ.demand[SECTOR_C][WEALTH_LOW] < unserved);

    // Tax: the same increase hurts $$$ more than $
    econ.avgEducation = 100.f;
    econ.computeDemand();
    float lowBefore = econ.demand[SECTOR_R][WEALTH_LOW], highBefore = econ.demand[SECTOR_R][WEALTH_HIGH];
    taxRates[SECTOR_R][WEALTH_LOW] = 15;
    taxRates[SECTOR_R][WEALTH_HIGH] = 15;
    econ.computeDemand();
    float lowDrop = lowBefore - econ.demand[SECTOR_R][WEALTH_LOW];
    float highDrop = highBefore - econ.demand[SECTOR_R][WEALTH_HIGH];
    ASSERT_TRUE(lowDrop > 0.f);
    ASSERT_TRUE(highDrop > lowDrop);
    resetTaxRates();

    // Education unlocks medium / high income homes and high-tech industry
    econ.avgEducation = 0.f;
    econ.computeDemand();
    float midNoSchool = econ.demand[SECTOR_R][WEALTH_MID], hiNoSchool = econ.demand[SECTOR_R][WEALTH_HIGH];
    float techNoSchool = econ.demand[SECTOR_I][WEALTH_HIGH];
    econ.avgEducation = 100.f;
    econ.computeDemand();
    ASSERT_TRUE(econ.demand[SECTOR_R][WEALTH_MID] > midNoSchool);
    ASSERT_TRUE(econ.demand[SECTOR_R][WEALTH_HIGH] > hiNoSchool);
    ASSERT_TRUE(econ.demand[SECTOR_I][WEALTH_HIGH] > techNoSchool);

    // Pollution near homes hits $$$ hardest
    float lowClean = econ.demand[SECTOR_R][WEALTH_LOW], hiClean = econ.demand[SECTOR_R][WEALTH_HIGH];
    econ.avgPollution = 50.f;
    econ.computeDemand();
    ASSERT_TRUE(hiClean - econ.demand[SECTOR_R][WEALTH_HIGH] > lowClean - econ.demand[SECTOR_R][WEALTH_LOW]);
    econ.avgPollution = 0.f;

    // Happiness: unhappy cities want fewer newcomers
    econ.happiness = 80;
    econ.computeDemand();
    float happy = econ.demand[SECTOR_R][WEALTH_MID];
    econ.happiness = 20;
    econ.computeDemand();
    ASSERT_TRUE(econ.demand[SECTOR_R][WEALTH_MID] < happy);

    // Always clamped to -100..100, with a readable reason
    for (int s = 0; s < 3; s++)
        for (int w = 0; w < 3; w++) {
            ASSERT_TRUE(econ.demand[s][w] >= -100.f && econ.demand[s][w] <= 100.f);
            ASSERT_FALSE(econ.limitingFactor(s, w).empty());
        }
    clearTestGrid();
}

TEST_CASE(Test_Economy_UpgradeNeedsDemand) {
    clearTestGrid();
    ScopedCoutSilence silence;
    EconomySystem econ;
    econ.happiness = 80;

    ResidentalZone* home = new ResidentalZone(4, 4, 1, 1);
    entities[4][4] = home;
    home->isActive = true;
    home->overallHappiness = 85;
    connectToOutside(4, 4);

    // Full and happy, but no medium-income demand: stays tier 1
    econ.demand[SECTOR_R][WEALTH_LOW] = 50.f;
    econ.demand[SECTOR_R][WEALTH_MID] = 0.f;
    for (int d = 0; d < 40; d++) { econ.dayCounter++; econ.updateOccupancy(); }
    ASSERT_EQ(home->population, home->capacity);
    ASSERT_EQ(home->tier, 1);

    // With $$ demand it moves up within a few weeks
    econ.demand[SECTOR_R][WEALTH_MID] = 60.f;
    for (int d = 0; d < 40 && home->tier == 1; d++) { econ.dayCounter++; econ.updateOccupancy(); }
    ASSERT_EQ(home->tier, 2);
    ASSERT_EQ(home->wealth(), static_cast<int>(WEALTH_MID));
    ASSERT_TRUE(home->population <= home->capacity);

    // Unhappy homes do not upgrade even with demand
    ResidentalZone* sad = new ResidentalZone(10, 4, 1, 1);
    entities[10][4] = sad;
    sad->isActive = true;
    sad->overallHappiness = 30;
    for (int d = 0; d < 40; d++) { econ.dayCounter++; econ.updateOccupancy(); }
    ASSERT_EQ(sad->tier, 1);

    clearTestGrid();
}

TEST_CASE(Test_Economy_UpkeepAndReport) {
    clearTestGrid();
    ScopedCoutSilence silence;
    EconomySystem econ;
    money = 1000;

    entities[2][2] = new DirtRoad(2, 2);
    entities[3][2] = new DirtRoad(3, 2);
    entities[4][2] = new DirtRoad(4, 2);
    SchoolZone* school = new SchoolZone(10, 10);
    entities[10][10] = school;
    ParkZone* park = new ParkZone(20, 20);
    entities[20][20] = park;

    DayReport r = econ.settle();
    ASSERT_EQ(r.roadTiles, 3);
    ASSERT_EQ(r.upkeepRoads, 3 * UPKEEP_ROAD);
    ASSERT_EQ(r.upkeepSchools, UPKEEP_SCHOOL);
    ASSERT_EQ(r.upkeepParks, UPKEEP_PARK);
    ASSERT_EQ(r.net, -(3 * UPKEEP_ROAD + UPKEEP_SCHOOL + UPKEEP_PARK));
    ASSERT_EQ(money, 1000 + r.net);

    // Private homes cost nothing to run
    ResidentalZone rz(30, 30);
    ASSERT_EQ(rz.dailyUpkeep(), 0);

    money = 5000;
    clearTestGrid();
}

TEST_CASE(Test_Economy_LoansAndBankruptcy) {
    clearTestGrid();
    ScopedCoutSilence silence;
    EconomySystem econ;
    money = 5000; // enough to cover the interest, so repaying never dips into debt

    // Loans: +$10,000 each, at most 3
    ASSERT_TRUE(econ.takeLoan());
    ASSERT_EQ(money, 5000 + LOAN_AMOUNT);
    ASSERT_TRUE(econ.takeLoan());
    ASSERT_TRUE(econ.takeLoan());
    ASSERT_FALSE(econ.takeLoan());
    ASSERT_EQ(money, 5000 + 3 * LOAN_AMOUNT);

    // Repaid daily with 10% interest over a year, then gone
    float daily = econ.dailyLoanPayments();
    ASSERT_NEAR(daily, 3.f * LOAN_AMOUNT * 1.1f / 360.f, 0.01f);
    for (int d = 0; d < LOAN_TERM_DAYS; d++) econ.settle();
    ASSERT_EQ(static_cast<int>(econ.loans.size()), 0);
    ASSERT_NEAR(static_cast<float>(5000 + 3 * LOAN_AMOUNT - money), 3.f * LOAN_AMOUNT * 1.1f, 2.f);
    ASSERT_EQ(econ.daysInDebt, 0);
    ASSERT_TRUE(econ.canTakeLoan());

    // Debt clock: recovering resets it
    money = -100;
    econ.settle();
    econ.settle();
    ASSERT_EQ(econ.daysInDebt, 2);
    money = 500;
    econ.settle();
    ASSERT_EQ(econ.daysInDebt, 0);

    // 180 days in the red = bankrupt
    money = -100;
    for (int d = 0; d < DEBT_LIMIT_DAYS - 1; d++) econ.settle();
    ASSERT_FALSE(econ.bankrupt);
    ASSERT_EQ(econ.daysUntilBankrupt(), 1);
    econ.settle();
    ASSERT_TRUE(econ.bankrupt);
    ASSERT_FALSE(econ.canTakeLoan());

    money = 5000;
    clearTestGrid();
}

TEST_CASE(Test_Economy_BankruptCityAndRestart) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 5000;
    scene.economy.bankrupt = true;

    std::string reason;
    ASSERT_FALSE(scene.canPlaceTile(10, 10, TOOL_ROAD, &reason));
    ASSERT_TRUE(reason.find("bankrupt") != std::string::npos);

    // Time stops on the game-over screen
    long daysBefore = gameClock.getTotalDays();
    scene.advanceTime(40.f);
    ASSERT_EQ(gameClock.getTotalDays(), daysBefore);

    // A new city resets money, economy and taxes
    taxRates[SECTOR_C][WEALTH_HIGH] = 20;
    scene.startNewCity(4242u);
    ASSERT_FALSE(scene.economy.bankrupt);
    ASSERT_EQ(money, STARTING_MONEY);
    ASSERT_EQ(taxRates[SECTOR_C][WEALTH_HIGH], TAX_DEFAULT);
    ASSERT_EQ(worldSeed, 4242u);
    ASSERT_TRUE(scene.canPlaceTile(worldSpawnX, worldSpawnY, TOOL_ROAD));

    money = 5000;
    clearTestGrid();
}

TEST_CASE(Test_Budget_SlidersAndButtons) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 0;
    scene.setBudgetOpen(true);

    // Panel fits on screen below the top bar
    sf::FloatRect b = scene.budgetBounds();
    ASSERT_TRUE(b.left >= 0.f && b.top > 48.f);
    ASSERT_TRUE(b.left + b.width <= 1500.f && b.top + b.height <= 900.f);

    // Pressing a slider's ends sets 0% / 20%; dragging follows the mouse
    sf::FloatRect t = scene.taxSliderBounds(SECTOR_I, WEALTH_MID);
    float midY = t.top + t.height / 2.f;
    ASSERT_TRUE(scene.handleBudgetPointer(sf::Vector2f(t.left, midY), true, true));
    ASSERT_EQ(taxRates[SECTOR_I][WEALTH_MID], TAX_MIN);
    ASSERT_TRUE(scene.handleBudgetPointer(sf::Vector2f(t.left + t.width, midY + 40.f), true, false)); // drag off the track
    ASSERT_EQ(taxRates[SECTOR_I][WEALTH_MID], TAX_MAX);
    scene.handleBudgetPointer(sf::Vector2f(t.left + t.width / 2.f, midY), false, false);            // release
    ASSERT_EQ(taxRates[SECTOR_I][WEALTH_MID], TAX_MAX);
    ASSERT_EQ(taxRates[SECTOR_I][WEALTH_LOW], TAX_DEFAULT); // other sliders untouched

    // Sliders do not overlap each other
    for (int s = 0; s < 3; s++)
        for (int w = 0; w < 3; w++)
            for (int s2 = 0; s2 < 3; s2++)
                for (int w2 = 0; w2 < 3; w2++)
                    if (s * 3 + w != s2 * 3 + w2)
                        ASSERT_FALSE(scene.taxSliderBounds(s, w).intersects(scene.taxSliderBounds(s2, w2)));

    // Loan button pays out; close button hides the panel
    sf::FloatRect lb = scene.loanButtonBounds();
    ASSERT_TRUE(scene.handleBudgetPointer(sf::Vector2f(lb.left + 5.f, lb.top + 5.f), true, true));
    ASSERT_EQ(money, LOAN_AMOUNT);
    sf::FloatRect xb = scene.budgetCloseBounds();
    ASSERT_TRUE(scene.handleBudgetPointer(sf::Vector2f(xb.left + 5.f, xb.top + 5.f), true, true));
    ASSERT_FALSE(scene.isBudgetOpen());

    // Clicks outside the panel are left for the map
    scene.setBudgetOpen(true);
    ASSERT_FALSE(scene.handleBudgetPointer(sf::Vector2f(5.f, 880.f), true, true));

    money = 5000;
    clearTestGrid();
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
    FarmZone* farm = new FarmZone(4, 4, 1);
    entities[4][4] = farm;
    farm->isActive = true;
    farm->population = 10;

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
// Roads: tiers, bridges, roundabouts, outside connection, traffic
// -------------------------------------------------------------
TEST_CASE(Test_Roads_TierUpgradeChargesDifference) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 1000;

    scene.placeTile(10, 10, TOOL_ROAD);
    ASSERT_EQ(money, 1000 - ROAD_COST[0]);
    RoadTile* r = RoadTile::at(10, 10);
    ASSERT_EQ(r->dailyUpkeep(), ROAD_UPKEEP[0]);

    std::string reason;
    ASSERT_FALSE(scene.canPlaceTile(10, 10, TOOL_ROAD, &reason));
    ASSERT_TRUE(reason.find("Already") != std::string::npos);

    // Avenue over dirt: an in-place upgrade paying only the difference
    ASSERT_EQ(scene.getPlacementCost(10, 10, TOOL_ROAD_AVENUE), ROAD_COST[2] - ROAD_COST[0]);
    int before = money;
    scene.placeTile(10, 10, TOOL_ROAD_AVENUE);
    ASSERT_EQ(money, before - (ROAD_COST[2] - ROAD_COST[0]));
    ASSERT_TRUE(RoadTile::at(10, 10) == r);
    ASSERT_EQ(tileKind(10, 10), "Avenue");
    ASSERT_EQ(r->dailyUpkeep(), ROAD_UPKEEP[2]);
    ASSERT_EQ(r->capacity(), ROAD_CAPACITY[2]);

    // Lower or equal tiers are not a downgrade; a drag just skips them
    ASSERT_FALSE(scene.canPlaceTile(10, 10, TOOL_ROAD_ASPHALT));
    before = money;
    ASSERT_EQ(scene.buildRoadPath(roadDragPath({9, 10}, {11, 10}), TOOL_ROAD_ASPHALT), "");
    ASSERT_EQ(money, before - 2 * ROAD_COST[1]);
    ASSERT_EQ(tileKind(10, 10), "Avenue");
    ASSERT_EQ(tileKind(9, 10), "AsphaltRoad");

    // Buildings take access from any tier
    ASSERT_TRUE(hasAdjacentRoad(9, 11));   // its (1, -1) border tile is the avenue (10, 10)
    money = 5000;
    clearTestGrid();
}

TEST_CASE(Test_Roads_BridgeRules) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 5000;

    // A 3-tile river across row y = 10
    for (int x = 10; x <= 12; x++) world[x][10].terrain = Terrain::Water;
    std::string reason;
    ASSERT_FALSE(scene.canPlaceTile(11, 10, TOOL_ROAD, &reason));
    ASSERT_TRUE(reason.find("bridge") != std::string::npos);

    // Dragging straight across builds land roads and bridge decks
    ASSERT_EQ(scene.buildRoadPath(roadDragPath({8, 10}, {14, 10}), TOOL_ROAD), "");
    ASSERT_EQ(money, 5000 - 4 * ROAD_COST[0] - 3 * ROAD_COST[0] * BRIDGE_COST_MULT);
    for (int x = 10; x <= 12; x++) {
        ASSERT_EQ(tileKind(x, 10), "Bridge");
        ASSERT_TRUE(RoadTile::at(x, 10)->isBridge());
        ASSERT_EQ(RoadTile::at(x, 10)->dailyUpkeep(), ROAD_UPKEEP[0] * BRIDGE_UPKEEP_MULT);
    }
    // The bridge joins both banks into one network
    roadNet.rebuild();
    ASSERT_EQ(roadNet.componentAt(8, 10), roadNet.componentAt(14, 10));
    // Bridges only link along their axis: a road beside the deck does not join it
    entities[12][9] = new DirtRoad(12, 9);   // NE of the deck
    ASSERT_EQ(RoadTile::linkMask(11, 10) & (1 | 4), 0);
    ASSERT_EQ(RoadTile::linkMask(12, 9), 0);
    // ...and no building can use a deck as its road access: the lot at (10, 11)
    // borders only the decks (11, 10) and (12, 10)
    ASSERT_FALSE(hasAdjacentRoad(10, 11));
    delete entities[12][9]; entities[12][9] = NULL;

    // Bulldozing a deck leaves the river
    scene.placeTile(11, 10, TOOL_BULLDOZE);
    ASSERT_EQ(tileKind(11, 10), "Water");

    // Too long: 9 tiles of water
    for (int x = 20; x <= 28; x++) world[x][30].terrain = Terrain::Water;
    int before = money;
    ASSERT_TRUE(scene.buildRoadPath(roadDragPath({18, 30}, {30, 30}), TOOL_ROAD).find("too long") != std::string::npos);
    ASSERT_EQ(tileKind(24, 30), "Water");

    // A bridge must end on land
    world[40][40].terrain = Terrain::Water;
    ASSERT_TRUE(scene.buildRoadPath(roadDragPath({38, 40}, {40, 40}), TOOL_ROAD).find("land") != std::string::npos);
    ASSERT_EQ(tileKind(40, 40), "Water");

    // A bridge cannot turn: the L's corner sits in the water
    for (int x = 50; x <= 53; x++)
        for (int y = 48; y <= 51; y++) world[x][y].terrain = Terrain::Water;
    ASSERT_TRUE(scene.buildRoadPath(roadDragPath({49, 50}, {54, 47}), TOOL_ROAD).find("straight") != std::string::npos);
    (void)before;

    money = 5000;
    clearTestGrid();
}

TEST_CASE(Test_Roads_RoundaboutNeedsJunction) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 5000;

    scene.buildRoadPath(roadDragPath({9, 10}, {11, 10}), TOOL_ROAD);
    std::string reason;
    ASSERT_FALSE(scene.canPlaceTile(10, 10, TOOL_ROUNDABOUT, &reason));  // straight piece
    ASSERT_TRUE(reason.find("Junction") != std::string::npos);
    ASSERT_FALSE(scene.canPlaceTile(20, 20, TOOL_ROUNDABOUT));           // not a road

    // A T-junction (third arm to the NE) takes a roundabout
    scene.placeTile(11, 9, TOOL_ROAD);
    RoadTile* r = RoadTile::at(10, 10);
    float plain = TrafficSystem::effectiveCapacity(r);
    ASSERT_NEAR(plain, ROAD_CAPACITY[0] * JUNCTION_CAPACITY, 0.01f);
    ASSERT_TRUE(scene.canPlaceTile(10, 10, TOOL_ROUNDABOUT));
    int before = money;
    scene.placeTile(10, 10, TOOL_ROUNDABOUT);
    ASSERT_EQ(money, before - ROUNDABOUT_COST);
    ASSERT_TRUE(r->roundabout);
    ASSERT_EQ(r->dailyUpkeep(), ROAD_UPKEEP[0] + UPKEEP_ROUNDABOUT);
    ASSERT_NEAR(TrafficSystem::effectiveCapacity(r), ROAD_CAPACITY[0] * ROUNDABOUT_CAPACITY, 0.01f);
    ASSERT_FALSE(scene.canPlaceTile(10, 10, TOOL_ROUNDABOUT));           // already one

    money = 5000;
    clearTestGrid();
}

TEST_CASE(Test_Roads_OutsideConnection) {
    clearTestGrid();
    ScopedCoutSilence silence;

    for (int x = 0; x <= 6; x++) entities[x][5] = new DirtRoad(x, 5);
    entities[20][20] = new DirtRoad(20, 20);
    roadNet.rebuild();
    ASSERT_TRUE(roadNet.tileOutside(3, 5));
    ASSERT_FALSE(roadNet.tileOutside(20, 20));
    ASSERT_TRUE(roadNet.componentAt(0, 5) != roadNet.componentAt(20, 20));

    // A lot whose west border tile (-1, 0) is the road at (6, 5)
    ASSERT_TRUE(roadNet.buildingOutside(7, 5));
    ASSERT_FALSE(roadNet.buildingOutside(21, 20));

    // Cutting the road at the edge isolates the network
    delete entities[0][5]; entities[0][5] = NULL;
    roadNet.rebuild();
    ASSERT_FALSE(roadNet.buildingOutside(7, 5));
    clearTestGrid();
}

TEST_CASE(Test_Economy_SoftPenaltyWithoutOutside) {
    clearTestGrid();
    ScopedCoutSilence silence;
    EconomySystem econ;
    econ.happiness = 90;

    // Road access, but the road is an island: slow growth to half full, no upgrade
    ResidentalZone* home = new ResidentalZone(10, 10, 1, 1);
    entities[10][10] = home;
    entities[9][10] = new DirtRoad(9, 10);
    home->checkValid();
    home->overallHappiness = 90;
    ASSERT_TRUE(home->isActive);
    roadNet.rebuild();

    econ.demand[SECTOR_R][WEALTH_LOW] = 60.f;
    econ.demand[SECTOR_R][WEALTH_MID] = 80.f;
    for (int d = 0; d < 60; d++) { econ.dayCounter++; econ.updateOccupancy(); }
    ASSERT_EQ(home->population, home->capacity / 2);
    ASSERT_EQ(home->tier, 1);

    // Connected to the map edge it fills up
    connectToOutside(10, 10);
    econ.demand[SECTOR_R][WEALTH_MID] = 0.f;
    for (int d = 0; d < 30; d++) { econ.dayCounter++; econ.updateOccupancy(); }
    ASSERT_EQ(home->population, home->capacity);
    clearTestGrid();
}

TEST_CASE(Test_Traffic_RoutesCommutersToJobs) {
    clearTestGrid();
    ScopedCoutSilence silence;

    // One road row; a home and a shop on its south side
    for (int x = 0; x <= 20; x++) entities[x][10] = new DirtRoad(x, 10);
    for (int x = 0; x <= 20; x++) RoadTile::at(x, 10)->check();
    ResidentalZone* home = new ResidentalZone(5, 11, 1, 1);
    entities[5][11] = home;
    CommericalZone* shop = new CommericalZone(15, 11, 1, 1);
    entities[15][11] = shop;
    home->checkValid();
    shop->checkValid();
    ASSERT_TRUE(home->isActive && shop->isActive);
    home->population = 20;
    shop->population = shop->capacity;   // at least 15 jobs

    traffic.update();
    int commuters = 12;                  // 60% of 20
    ASSERT_EQ(traffic.totalTrips, commuters);
    ASSERT_EQ(traffic.outsideTrips, 0);
    // Route: home's NE border road (7, 10) to the shop's (16, 10)
    ASSERT_NEAR(traffic.load[7][10], 12.f, 0.01f);
    ASSERT_NEAR(traffic.load[12][10], 12.f, 0.01f);
    ASSERT_NEAR(traffic.load[16][10], 12.f, 0.01f);
    ASSERT_NEAR(traffic.load[3][10], 0.f, 0.01f);
    ASSERT_NEAR(traffic.load[18][10], 0.f, 0.01f);
    ASSERT_NEAR(traffic.congestion[12][10], 12.f / ROAD_CAPACITY[0], 0.001f);

    // Higher tiers carry more: the same load is a smaller share of an avenue
    RoadTile::at(12, 10)->setTier(2);
    traffic.update();
    ASSERT_NEAR(traffic.congestion[12][10], 12.f / ROAD_CAPACITY[2], 0.001f);

    // No jobs left: commuters drive out of town through the map edge
    shop->population = 0;
    traffic.update();
    ASSERT_EQ(traffic.outsideTrips, commuters);
    ASSERT_NEAR(traffic.load[0][10], 12.f, 0.01f);
    clearTestGrid();
}

TEST_CASE(Test_Traffic_HappinessPenaltyOnlyWhenJammed) {
    clearTestGrid();
    ScopedCoutSilence silence;
    ASSERT_EQ(TrafficSystem::happinessPenalty(0.9f), 0);
    ASSERT_EQ(TrafficSystem::happinessPenalty(1.5f), 10);
    ASSERT_EQ(TrafficSystem::happinessPenalty(4.f), 20);
    ASSERT_EQ(TrafficSystem::scoreFor(0.3f), 100);
    ASSERT_EQ(TrafficSystem::scoreFor(1.5f), 0);

    ResidentalZone* home = new ResidentalZone(5, 11, 1, 1);
    entities[5][11] = home;
    home->isActive = true;
    HappinessSystem hs;
    hs.updateSimulation();
    int base = home->overallHappiness;
    traffic.homeCongestion[5][11] = 0.95f;   // busy but under capacity: no penalty
    hs.updateSimulation();
    ASSERT_EQ(home->overallHappiness, base);
    traffic.homeCongestion[5][11] = 2.f;     // twice capacity: -20
    hs.updateSimulation();
    ASSERT_EQ(home->overallHappiness, std::max(5, base - 20));
    clearTestGrid();
}

TEST_CASE(Test_Economy_RoadUpkeepByTier) {
    clearTestGrid();
    ScopedCoutSilence silence;
    entities[2][2] = new DirtRoad(2, 2);
    entities[3][2] = new AsphaltRoad(3, 2);
    entities[4][2] = new Avenue(4, 2);
    world[5][2].terrain = Terrain::Water;
    entities[5][2] = new RoadTile(5, 2, 0, 1);
    EconomySystem econ;
    DayReport r = econ.settle();
    ASSERT_EQ(r.roadTiles, 4);
    ASSERT_EQ(r.upkeepRoads, ROAD_UPKEEP[0] + ROAD_UPKEEP[1] + ROAD_UPKEEP[2] + ROAD_UPKEEP[0] * BRIDGE_UPKEEP_MULT);
    money = 5000;
    clearTestGrid();
}

TEST_CASE(Test_Cars_StayOnRoads) {
    clearTestGrid();
    ScopedCoutSilence silence;
    for (int x = 0; x <= 20; x++) entities[x][10] = new DirtRoad(x, 10);
    for (int x = 0; x <= 20; x++) RoadTile::at(x, 10)->check();
    roadNet.rebuild();
    for (int x = 0; x <= 20; x++) traffic.load[x][10] = 30.f;
    traffic.totalTrips = 60;

    CarRenderer cars;
    ASSERT_EQ(CarRenderer::targetCount(60, 21), 10);
    for (int f = 0; f < 120; f++) cars.update(0.05f);
    ASSERT_TRUE(!cars.cars.empty());
    ASSERT_TRUE(static_cast<int>(cars.cars.size()) <= 10);
    for (const auto& c : cars.cars) ASSERT_TRUE(RoadTile::at(c.x, c.y) != NULL);

    // Paused: nothing moves
    auto snapshot = cars.cars;
    cars.update(0.f);
    ASSERT_NEAR(cars.cars[0].t, snapshot[0].t, 0.0001f);

    // Roads gone: cars disappear
    for (int x = 0; x <= 20; x++) { delete entities[x][10]; entities[x][10] = NULL; }
    cars.update(0.05f);
    ASSERT_TRUE(cars.cars.empty());
    clearTestGrid();
}

// -------------------------------------------------------------
// Main Test Runner Entrypoint
// -------------------------------------------------------------
// -------------------------------------------------------------
// Power & water
// -------------------------------------------------------------
// Layout used below: a dirt road along row 10, buildings rooted on row 11
// (their north-east border tiles (x+1, 10) / (x+2, 10) are the road).
static void utilRoadRow(int y, int x0, int x1) {
    for (int x = x0; x <= x1; x++) {
        delete entities[x][y];
        entities[x][y] = new DirtRoad(x, y);
    }
    roadNet.rebuild();
}
static ResidentalZone* utilHome(int x, int y, int pop) {
    ResidentalZone* h = new ResidentalZone(x, y, 1, 1);
    delete entities[x][y];
    entities[x][y] = h;
    h->checkValid();
    h->population = pop;
    return h;
}
static UtilityZone* utilBuilding(int x, int y, int kind) {
    UtilityZone* u = new UtilityZone(x, y, kind);
    delete entities[x][y];
    entities[x][y] = u;
    u->checkValid();
    return u;
}
static void utilGroundwater(int value) {
    for (int i = 0; i < tileMapW; i++)
        for (int j = 0; j < tileMapH; j++) world[i][j].groundwater = world[i][j].groundwaterMax = static_cast<uint8_t>(value);
}

TEST_CASE(Test_Utilities_GraceForSmallTowns) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilRoadRow(10, 0, 30);
    ResidentalZone* h = utilHome(4, 11, 20);
    utilities.refreshService();
    ASSERT_TRUE(utilities.grace);
    ASSERT_TRUE(utilities.served(4, 11));      // small town: nobody needs utilities yet
    ASSERT_EQ(utilities.unpowered, 1);         // ...but the shortfall is still counted

    h->population = UTILITY_GRACE_POP;
    utilities.refreshService();
    ASSERT_FALSE(utilities.grace);
    ASSERT_FALSE(utilities.powered(4, 11));
    ASSERT_FALSE(utilities.watered(4, 11));
    clearTestGrid();
}

TEST_CASE(Test_Utilities_PlantNeedsTransformer) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilRoadRow(10, 0, 30);
    utilHome(4, 11, 200);
    utilBuilding(8, 11, UTIL_COAL);
    utilities.refreshService();
    ASSERT_FALSE(utilities.powered(4, 11));    // 400 MW on the grid, but nothing steps it down
    ASSERT_NEAR(utilities.powerGrids[0].powerSupply, 400.f, 0.01f);

    utilBuilding(12, 11, UTIL_TRANSFORMER);
    utilities.refreshService();
    ASSERT_TRUE(utilities.powered(4, 11));
    clearTestGrid();
}

TEST_CASE(Test_Utilities_TransformerCapacityNoRange) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilRoadRow(10, 0, 60);
    utilBuilding(50, 11, UTIL_COAL);
    utilBuilding(54, 11, UTIL_TRANSFORMER);
    utilHome(2, 11, 200);                      // 52 tiles from the transformer: distance does not matter
    utilities.refreshService();
    ASSERT_TRUE(utilities.powered(2, 11));

    // Five big homes: each needs 160 x 0.25 x (1 + 800/3000) = 50.7 MW, so the
    // 150 MW transformer carries two of them (scan order); the coal plant has plenty left
    delete entities[2][11]; entities[2][11] = NULL;
    for (int x : {2, 6, 10, 14, 18}) utilHome(x, 11, 160);
    utilities.refreshService();
    int powered = 0;
    for (int x : {2, 6, 10, 14, 18}) powered += utilities.powered(x, 11) ? 1 : 0;
    ASSERT_EQ(powered, 2);
    ASSERT_TRUE(utilities.powered(2, 11));
    ASSERT_FALSE(utilities.powered(18, 11));
    ASSERT_NEAR(utilities.powerSupply, 150.f, 0.01f);   // deliverable = min(plants, transformers)

    utilBuilding(40, 11, UTIL_TRANSFORMER);    // a second one anywhere on the grid adds 150 MW
    utilities.refreshService();
    powered = 0;
    for (int x : {2, 6, 10, 14, 18}) powered += utilities.powered(x, 11) ? 1 : 0;
    ASSERT_EQ(powered, 5);
    clearTestGrid();
}

TEST_CASE(Test_Utilities_GridsFollowRoads) {
    clearTestGrid();
    ScopedCoutSilence silence;
    // Two road segments on row 11 with a gap at x = 10..11
    for (int x = 0; x <= 9; x++) entities[x][11] = new DirtRoad(x, 11);
    for (int x = 12; x <= 30; x++) entities[x][11] = new DirtRoad(x, 11);
    roadNet.rebuild();
    ASSERT_TRUE(roadNet.componentAt(5, 11) != roadNet.componentAt(20, 11));

    utilBuilding(3, 12, UTIL_WIND);            // west network
    utilBuilding(6, 12, UTIL_TRANSFORMER);
    utilHome(14, 12, 200);                     // east network (8 tiles from the transformer): no power
    utilities.refreshService();
    ASSERT_EQ(static_cast<int>(utilities.powerGrids.size()), 2);
    ASSERT_FALSE(utilities.powered(14, 12));

    // A building in the gap borders both ends ((-1, 0) and (2, 0)) and joins the grids
    utilBuilding(10, 11, UTIL_TOWER);
    utilities.refreshService();
    ASSERT_EQ(static_cast<int>(utilities.powerGrids.size()), 1);
    ASSERT_TRUE(utilities.powered(14, 12));
    clearTestGrid();
}

TEST_CASE(Test_Utilities_OutputBySeasonAndLand) {
    clearTestGrid();
    ScopedCoutSilence silence;
    UtilityZone* solar = utilBuilding(10, 11, UTIL_SOLAR);
    gameClock.setDate(1, 7, 1);
    ASSERT_TRUE(gameClock.getSeason() == Season::Summer);
    ASSERT_NEAR(UtilitySystem::outputOf(solar), 144.f, 0.01f);
    gameClock.setDate(1, 1, 1);
    ASSERT_TRUE(gameClock.getSeason() == Season::Winter);
    ASSERT_NEAR(UtilitySystem::outputOf(solar), 60.f, 0.01f);

    UtilityZone* coal = utilBuilding(20, 11, UTIL_COAL);
    ASSERT_NEAR(UtilitySystem::outputOf(coal), 400.f, 0.01f);
    world[21][11].deposit = Deposit::Coal;     // any footprint tile on coal
    ASSERT_NEAR(UtilitySystem::outputOf(coal), 500.f, 0.01f);

    // Pumps and wells scale with the reserve around them
    utilGroundwater(50);
    UtilityZone* pump = utilBuilding(30, 30, UTIL_PUMP);
    UtilityZone* well = utilBuilding(40, 30, UTIL_WELL);
    ASSERT_NEAR(UtilitySystem::outputOf(pump), 200.f, 0.01f);
    ASSERT_NEAR(UtilitySystem::outputOf(well), 40.f, 0.01f);
    utilGroundwater(100);
    ASSERT_NEAR(UtilitySystem::outputOf(pump), 400.f, 0.01f);
    clearTestGrid();
}

TEST_CASE(Test_Utilities_PumpingDrainsAndRecharges) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilGroundwater(60);
    utilRoadRow(10, 0, 40);
    utilBuilding(20, 11, UTIL_PUMP);
    for (int x : {4, 8, 12}) utilHome(x, 11, 400);   // ~112 water each, more than the pump gives
    gameClock.setDate(1, 7, 1);                        // summer: slow recharge

    const float before = UtilityZone::groundwaterAround(20, 11, PUMP_RADIUS);
    for (int d = 0; d < 30; d++) utilities.runDay();
    const float drained = UtilityZone::groundwaterAround(20, 11, PUMP_RADIUS);
    ASSERT_TRUE(drained < before - 0.02f);
    ASSERT_EQ(static_cast<int>(world[2][2].groundwater), 60);   // far away: untouched

    // Nobody drawing water: the reserve recovers towards its natural level
    for (int x : {4, 8, 12}) { delete entities[x][11]; entities[x][11] = NULL; }
    gameClock.setDate(1, 4, 1);                        // spring: fast recharge
    for (int d = 0; d < 30; d++) utilities.runDay();
    const float recovered = UtilityZone::groundwaterAround(20, 11, PUMP_RADIUS);
    ASSERT_TRUE(recovered > drained + 0.02f);
    ASSERT_TRUE(recovered <= 0.6f + 0.001f);
    clearTestGrid();
}

TEST_CASE(Test_Utilities_TreatmentAndWells) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilGroundwater(50);
    utilRoadRow(10, 0, 40);
    utilHome(4, 11, 200);                      // ~42.7 water
    utilBuilding(20, 11, UTIL_PUMP);           // 200 raw
    utilities.refreshService();
    ASSERT_TRUE(utilities.watered(4, 11));
    ASSERT_TRUE(utilities.untreated(4, 11));   // raw water straight to the tap

    utilBuilding(24, 11, UTIL_TREATMENT);
    utilities.refreshService();
    ASSERT_TRUE(utilities.watered(4, 11));
    ASSERT_FALSE(utilities.untreated(4, 11));

    // A well alone gives clean water
    delete entities[20][11]; entities[20][11] = NULL;
    delete entities[24][11]; entities[24][11] = NULL;
    utilGroundwater(100);
    utilBuilding(28, 11, UTIL_WELL);           // 80 clean
    utilities.refreshService();
    ASSERT_TRUE(utilities.watered(4, 11));
    ASSERT_FALSE(utilities.untreated(4, 11));
    clearTestGrid();
}

TEST_CASE(Test_Utilities_TowerCoversShortfall) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilGroundwater(100);
    utilRoadRow(10, 0, 40);
    utilHome(4, 11, 200);                      // ~42.7 a day
    utilBuilding(20, 11, UTIL_WELL);           // 80 a day
    UtilityZone* tower = utilBuilding(24, 11, UTIL_TOWER);
    for (int d = 0; d < 12; d++) utilities.runDay();
    ASSERT_NEAR(tower->stored, 300.f, 0.5f);   // the spare water filled it

    delete entities[20][11]; entities[20][11] = NULL;   // the well is gone
    utilities.runDay();
    ASSERT_TRUE(utilities.watered(4, 11));     // the tower covers the day
    ASSERT_TRUE(tower->stored < 300.f - 40.f);
    clearTestGrid();
}

TEST_CASE(Test_Utilities_DemandScalesWithTierAndCity) {
    clearTestGrid();
    ScopedCoutSilence silence;
    ResidentalZone* low = utilHome(4, 11, 100);
    ResidentalZone* mid = new ResidentalZone(8, 11, 2, 1);
    entities[8][11] = mid;
    mid->population = 100;
    float a = UtilitySystem::demandOf(low, false, 0), b = UtilitySystem::demandOf(mid, false, 0);
    ASSERT_NEAR(a, 25.f, 0.01f);
    ASSERT_NEAR(b / a, UTIL_TIER_FACTOR[1], 0.001f);
    ASSERT_NEAR(UtilitySystem::demandOf(low, false, 3000) / a, 2.f, 0.001f);   // a 3,000 city uses twice as much per head
    ASSERT_NEAR(UtilitySystem::demandOf(low, true, 0), 20.f, 0.01f);
    low->population = 0;                                                         // empty lots still need 10%
    ASSERT_NEAR(UtilitySystem::demandOf(low, false, 0), 0.1f * low->capacity * 0.25f, 0.01f);
    clearTestGrid();
}

TEST_CASE(Test_Utilities_UnservedBuildingsShrink) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilRoadRow(10, 0, 40);
    ResidentalZone* h = utilHome(4, 11, 0);
    h->population = h->capacity;
    utilHome(20, 11, 200);
    utilities.refreshService();
    ASSERT_FALSE(utilities.served(4, 11));

    EconomySystem econ;
    econ.happiness = 90;
    h->overallHappiness = 90;
    econ.demand[SECTOR_R][WEALTH_LOW] = 80.f;
    econ.demand[SECTOR_R][WEALTH_MID] = 80.f;
    for (int d = 0; d < 60; d++) { econ.dayCounter++; econ.updateOccupancy(); }
    ASSERT_EQ(h->population, h->capacity / 4);  // never abandoned outright
    ASSERT_EQ(h->tier, 1);                      // and never upgraded
    ASSERT_EQ(econ.limitingFactor(SECTOR_R, WEALTH_LOW), std::string("Strong demand"));
    econ.demand[SECTOR_R][WEALTH_LOW] = 0.f;
    ASSERT_EQ(econ.limitingFactor(SECTOR_R, WEALTH_LOW), std::string("No power"));
    clearTestGrid();
}

TEST_CASE(Test_Utilities_HappinessPenalty) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilGroundwater(100);
    utilRoadRow(10, 0, 40);
    ResidentalZone* h = utilHome(4, 11, 200);
    HappinessSystem hs;
    utilities.refreshService();
    hs.updateSimulation();
    const int dark = h->overallHappiness;
    ASSERT_EQ(h->happinessIndex[7], 0);

    utilBuilding(12, 11, UTIL_WIND);
    utilBuilding(8, 11, UTIL_TRANSFORMER);
    utilBuilding(16, 11, UTIL_WELL);
    utilities.refreshService();
    ASSERT_TRUE(utilities.served(4, 11));
    hs.updateSimulation();
    ASSERT_EQ(h->happinessIndex[7], 100);
    ASSERT_EQ(h->overallHappiness - dark, 24);

    // Untreated water costs health
    const int healthy = h->happinessIndex[4];
    delete entities[16][11]; entities[16][11] = NULL;
    utilBuilding(16, 11, UTIL_PUMP);
    utilities.refreshService();
    ASSERT_TRUE(utilities.untreated(4, 11));
    hs.updateSimulation();
    ASSERT_EQ(healthy - h->happinessIndex[4], 20);
    clearTestGrid();
}

TEST_CASE(Test_Utilities_UpkeepAndToolbar) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilRoadRow(10, 0, 20);
    utilBuilding(4, 11, UTIL_WIND);
    utilBuilding(8, 11, UTIL_TREATMENT);
    EconomySystem econ;
    DayReport rep = econ.settle();
    ASSERT_EQ(rep.upkeepUtilities, UTIL_UPKEEP[UTIL_WIND] + UTIL_UPKEEP[UTIL_TREATMENT]);

    ASSERT_EQ(utilityToolKind(TOOL_SEWAGE_OUTFLOW), static_cast<int>(UTIL_SEWAGE_OUTFLOW));
    ASSERT_EQ(utilityToolKind(TOOL_SEWAGE_PLANT), static_cast<int>(UTIL_SEWAGE_PLANT));
    for (int k = 0; k <= UTIL_TOWER; k++) {
        BuildTool t = static_cast<BuildTool>(TOOL_POWER_COAL + k);
        ASSERT_TRUE(isUtilityTool(t));
        ASSERT_EQ(utilityToolKind(t), k);
        ASSERT_EQ(toolCost(t), UTIL_COST[k]);
        ASSERT_EQ(toolThumbnailKey(t), std::string(utilityTextureKey(k)));
    }
    ASSERT_FALSE(isUtilityTool(TOOL_BULLDOZE));

    Toolbar tb;
    tb.init(1600.f, 900.f);
    tb.setActiveTool(TOOL_WATER_PUMP);
    ASSERT_TRUE(tb.getActiveCategory() == ToolbarCategory::UTILITIES);
    ASSERT_TRUE(tb.tabBounds(ToolbarCategory::UTILITIES).height > 0.f);

    // Placing through the scene builds a UtilityZone with its sub tiles
    Play_Scene scene;
    money = 100000;
    ASSERT_TRUE(scene.canPlaceTile(14, 11, TOOL_POWER_SOLAR));
    scene.placeTile(14, 11, TOOL_POWER_SOLAR);
    UtilityZone* u = dynamic_cast<UtilityZone*>(entities[14][11]);
    ASSERT_TRUE(u != NULL);
    ASSERT_EQ(u->kind, UTIL_SOLAR);
    ASSERT_TRUE(u->isActive);
    ASSERT_EQ(tileKind(15, 11), std::string("UtilityZone"));
    ASSERT_EQ(money, 100000 - UTIL_COST[UTIL_SOLAR]);
    clearTestGrid();
}

TEST_CASE(Test_Utilities_LinesConnectOffRoadBuildings) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilGroundwater(100);
    utilRoadRow(10, 0, 30);
    utilHome(4, 11, 200);
    utilBuilding(8, 11, UTIL_TRANSFORMER);
    UtilityZone* wind = utilBuilding(20, 4, UTIL_WIND);       // no road anywhere near
    ASSERT_TRUE(wind->isActive);                              // utilities need no road
    utilities.refreshService();
    ASSERT_FALSE(wind->linked);
    ASSERT_FALSE(utilities.powered(4, 11));

    // Power line from the wind farm's south-west border (19, 6) to the road at (15, 10)
    const int path[4][2] = {{19, 6}, {18, 7}, {17, 8}, {16, 9}};
    for (const auto& p : path) world[p[0]][p[1]].lines = LINE_POWER;
    utilities.refreshService();
    ASSERT_TRUE(wind->linked);
    ASSERT_TRUE(utilities.powered(4, 11));

    // A pipe carries water, not power
    for (const auto& p : path) world[p[0]][p[1]].lines = LINE_PIPE;
    utilities.refreshService();
    ASSERT_FALSE(utilities.powered(4, 11));

    // ...and links an off-road well to the homes
    UtilityZone* well = utilBuilding(26, 4, UTIL_WELL);
    utilities.refreshService();
    ASSERT_FALSE(utilities.watered(4, 11));
    const int pipe[4][2] = {{25, 6}, {24, 7}, {23, 8}, {22, 9}};
    for (const auto& p : pipe) world[p[0]][p[1]].lines = LINE_PIPE;
    utilities.refreshService();
    ASSERT_TRUE(well->linked);
    ASSERT_TRUE(utilities.watered(4, 11));
    clearTestGrid();
}

TEST_CASE(Test_Utilities_LinePlacementRules) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 100000;
    utilRoadRow(10, 0, 30);

    ASSERT_TRUE(scene.canPlaceTile(5, 5, TOOL_POWER_LINE));
    scene.placeTile(5, 5, TOOL_POWER_LINE);
    ASSERT_TRUE(hasLine(5, 5, LINE_POWER));
    ASSERT_EQ(money, 100000 - LINE_COST_POWER);
    ASSERT_FALSE(scene.canPlaceTile(5, 5, TOOL_POWER_LINE));    // already there
    ASSERT_FALSE(scene.canPlaceTile(3, 10, TOOL_POWER_LINE));   // roads carry both already
    world[7][7].terrain = Terrain::Water;
    ASSERT_FALSE(scene.canPlaceTile(7, 7, TOOL_WATER_PIPE));
    std::string reason;
    ASSERT_FALSE(scene.canPlaceTile(5, 5, TOOL_SCHOOL, &reason)); // pylon in the footprint
    ASSERT_EQ(reason, std::string("Power Line in the Way - Bulldoze First"));

    // A pipe shares the tile; bulldozing takes the power line first, then the pipe
    scene.placeTile(5, 5, TOOL_WATER_PIPE);
    ASSERT_EQ(static_cast<int>(world[5][5].lines), LINE_POWER | LINE_PIPE);
    scene.placeTile(5, 5, TOOL_BULLDOZE);
    ASSERT_EQ(static_cast<int>(world[5][5].lines), static_cast<int>(LINE_PIPE));
    scene.placeTile(5, 5, TOOL_BULLDOZE);
    ASSERT_EQ(static_cast<int>(world[5][5].lines), 0);
    ASSERT_FALSE(scene.canPlaceTile(5, 5, TOOL_BULLDOZE));

    // Pipes run under buildings, power lines do not
    scene.placeTile(12, 11, TOOL_RES_COTTAGES);
    ASSERT_TRUE(scene.canPlaceTile(12, 11, TOOL_WATER_PIPE));
    ASSERT_FALSE(scene.canPlaceTile(12, 11, TOOL_POWER_LINE));

    // Utility buildings need no road
    ASSERT_TRUE(scene.canPlaceTile(30, 30, TOOL_POWER_WIND));
    ASSERT_FALSE(scene.canPlaceTile(30, 30, TOOL_SCHOOL));

    // Upkeep: $1 a day per 10 line tiles
    clearTestGrid();
    for (int x = 0; x < 20; x++) world[x][20].lines = LINE_PIPE;
    EconomySystem econ;
    ASSERT_EQ(econ.settle().upkeepUtilities, 20 / LINE_TILES_PER_UPKEEP);
    clearTestGrid();
}

TEST_CASE(Test_Utilities_FlyoutWrapsOnSmallWindows) {
    Toolbar tb;
    tb.init(1024.f, 680.f);
    tb.openFlyout(ToolbarCategory::UTILITIES);
    sf::FloatRect fb = tb.flyoutBounds();
    ASSERT_TRUE(fb.top + fb.height <= 680.f);
    ASSERT_TRUE(fb.width > 400.f);                               // two columns
    sf::FloatRect first = tb.cardBounds(0), last = tb.cardBounds(9);
    ASSERT_TRUE(last.left > first.left);
    ASSERT_TRUE(tb.cardBoundsFor(TOOL_WATER_PIPE) == last);
    tb.init(1600.f, 1000.f);                                     // tall window: one column
    ASSERT_NEAR(tb.flyoutBounds().width, 280.f, 0.01f);
}

// -------------------------------------------------------------
// Zones
// -------------------------------------------------------------
// Road along row 10 from the map edge; lots rooted on row 11 border it.
static void zoneRect(Play_Scene& scene, BuildTool tool, int x0, int y0, int x1, int y1) {
    for (int x = x0; x <= x1; x++)
        for (int y = y0; y <= y1; y++)
            if (scene.canPlaceTile(x, y, tool)) scene.placeTile(x, y, tool);
}

TEST_CASE(Test_Zones_PaintAndDezone) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 10000;
    utilRoadRow(10, 0, 30);
    world[6][12].terrain = Terrain::Water;

    zoneRect(scene, TOOL_ZONE_R_MED, 2, 10, 6, 12);   // row 10 is road, (6, 12) water: both skipped
    ASSERT_EQ(static_cast<int>(world[3][11].zone), static_cast<int>(makeZone(SECTOR_R, 1)));
    ASSERT_EQ(static_cast<int>(world[3][10].zone), 0);
    ASSERT_EQ(static_cast<int>(world[6][12].zone), 0);
    ASSERT_EQ(money, 10000 - 9 * ZONE_COST_PER_TILE);
    ASSERT_EQ(zoneMaxTier(world[3][11].zone), 2);
    std::string reason;
    ASSERT_FALSE(scene.canPlaceTile(3, 11, TOOL_ZONE_R_MED, &reason));
    ASSERT_EQ(reason, std::string("Already Zoned"));
    ASSERT_TRUE(scene.canPlaceTile(3, 11, TOOL_ZONE_C_LOW));   // rezoning to another type is allowed

    // Dezone is free; bulldozing bare zoned ground also clears it
    scene.placeTile(3, 11, TOOL_DEZONE);
    ASSERT_EQ(static_cast<int>(world[3][11].zone), 0);
    ASSERT_TRUE(scene.canPlaceTile(4, 11, TOOL_BULLDOZE));
    scene.placeTile(4, 11, TOOL_BULLDOZE);
    ASSERT_EQ(static_cast<int>(world[4][11].zone), 0);
    ASSERT_EQ(zoneToolValue(TOOL_ZONE_C_HIGH), makeZone(SECTOR_C, 2));
    clearTestGrid();
}

TEST_CASE(Test_Zones_BuildingsGrowWithDemand) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 10000;
    utilRoadRow(10, 0, 30);
    zoneRect(scene, TOOL_ZONE_R_LOW, 1, 11, 14, 12);

    scene.economy.demand[SECTOR_R][WEALTH_LOW] = 0.f;
    ASSERT_EQ(scene.growZones(), 0);                          // no demand, no growth

    scene.economy.demand[SECTOR_R][WEALTH_LOW] = 60.f;        // 1 + 60 / 30 = 3 a day
    ASSERT_EQ(scene.growZones(), 3);
    int grown = 0;
    for (int x = 0; x < tileMapW; x++)
        for (int y = 0; y < tileMapH; y++) {
            ResidentalZone* r = dynamic_cast<ResidentalZone*>(entities[x][y]);
            if (r == NULL || r->isSubTile) continue;
            grown++;
            ASSERT_TRUE(r->zoned);
            ASSERT_FALSE(r->fixedTier);
            ASSERT_EQ(r->maxTier, 1);
            ASSERT_EQ(r->tier, 1);
            ASSERT_TRUE(r->isActive);
        }
    ASSERT_EQ(grown, 3);
    ASSERT_EQ(money, 10000 - 28 * ZONE_COST_PER_TILE);       // growing is free
    clearTestGrid();
}

TEST_CASE(Test_Zones_NoGrowthOnBadLots) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 10000;
    utilRoadRow(10, 0, 30);
    scene.economy.demand[SECTOR_R][WEALTH_LOW] = 90.f;

    zoneRect(scene, TOOL_ZONE_R_LOW, 20, 30, 30, 40);         // nowhere near a road
    world[5][11].zone = makeZone(SECTOR_R, 0);                // only part of a footprint
    world[6][11].zone = makeZone(SECTOR_R, 0);
    world[5][12].zone = makeZone(SECTOR_R, 0);
    for (int x : {12, 13}) world[x][11].zone = makeZone(SECTOR_R, 0);   // two zone types in one lot
    for (int x : {11, 12}) world[x][12].zone = makeZone(SECTOR_R, 1);
    for (int x : {18, 19}) { world[x][11].zone = world[x - 1][12].zone = world[x][12].zone = makeZone(SECTOR_R, 0); }
    world[17][12].zone = makeZone(SECTOR_R, 0);
    world[18][11].lines = LINE_POWER;                         // a pylon in the way
    ASSERT_EQ(scene.growZones(), 0);
    clearTestGrid();
}

TEST_CASE(Test_Zones_DensityCapsUpgrades) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilRoadRow(10, 0, 30);
    EconomySystem econ;
    econ.happiness = 90;
    for (int w = 0; w < WEALTH_COUNT; w++) econ.demand[SECTOR_R][w] = 80.f;

    ResidentalZone* low = utilHome(4, 11, 0);
    low->zoned = true; low->maxTier = 1;
    ResidentalZone* high = utilHome(12, 11, 0);
    high->zoned = true; high->maxTier = 3;
    for (ResidentalZone* h : {low, high}) { h->population = h->capacity; h->overallHappiness = 90; }
    for (int d = 0; d < 150; d++) {
        econ.dayCounter++;
        econ.updateOccupancy();
        low->overallHappiness = high->overallHappiness = 90;
    }
    ASSERT_EQ(low->tier, 1);
    ASSERT_TRUE(high->tier >= 2);
    clearTestGrid();
}

TEST_CASE(Test_Zones_PlacedBuildingsKeepTheirTier) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 10000;
    utilRoadRow(10, 0, 30);
    scene.placeTile(4, 11, TOOL_RES_COTTAGES);
    ResidentalZone* h = dynamic_cast<ResidentalZone*>(entities[4][11]);
    ASSERT_TRUE(h != NULL);
    ASSERT_TRUE(h->fixedTier);
    ASSERT_FALSE(h->zoned);

    EconomySystem econ;
    econ.happiness = 90;
    for (int w = 0; w < WEALTH_COUNT; w++) econ.demand[SECTOR_R][w] = 80.f;
    for (int d = 0; d < 150; d++) {
        econ.dayCounter++;
        econ.updateOccupancy();
        h->overallHappiness = 90;
    }
    ASSERT_EQ(h->tier, 1);                                     // never upgrades...
    ASSERT_EQ(h->population, h->capacity);                     // ...but still fills with demand
    clearTestGrid();
}

TEST_CASE(Test_Zones_DemolishedLotRegrows) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 10000;
    utilRoadRow(10, 0, 30);
    for (const auto& o : FOOTPRINT_2X2) scene.placeTile(4 + o[0], 11 + o[1], TOOL_ZONE_C_LOW);   // exactly one lot
    scene.economy.demand[SECTOR_C][WEALTH_LOW] = 10.f;
    ASSERT_EQ(scene.growZones(), 1);
    ASSERT_TRUE(dynamic_cast<CommericalZone*>(entities[4][11]) != NULL);

    scene.placeTile(4, 11, TOOL_BULLDOZE);
    ASSERT_TRUE(entities[4][11] == NULL);
    ASSERT_EQ(static_cast<int>(world[4][11].zone), static_cast<int>(makeZone(SECTOR_C, 0)));   // zone kept
    ASSERT_EQ(scene.growZones(), 1);
    ASSERT_TRUE(dynamic_cast<CommericalZone*>(entities[4][11]) != NULL);

    Toolbar tb;
    tb.init(1600.f, 1000.f);
    tb.setActiveTool(TOOL_ZONE_I_HIGH);
    ASSERT_TRUE(tb.getActiveCategory() == ToolbarCategory::INDUSTRIAL);
    tb.setActiveTool(TOOL_DEZONE);
    ASSERT_TRUE(tb.getActiveCategory() == ToolbarCategory::INFRASTRUCTURE);
    ASSERT_EQ(toolCost(TOOL_ZONE_R_LOW), ZONE_COST_PER_TILE);
    ASSERT_EQ(toolCost(TOOL_DEZONE), 0);
    clearTestGrid();
}

// -------------------------------------------------------------
// Save / load, undo, services, fires, mines, buses, land value
// -------------------------------------------------------------
static ServiceZone* svcBuilding(int x, int y, int kind) {
    ServiceZone* s = new ServiceZone(x, y, kind);
    delete entities[x][y];
    entities[x][y] = s;
    s->checkValid();
    return s;
}

TEST_CASE(Test_Save_RoundTrip) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 12345;
    utilRoadRow(10, 0, 30);
    RoadTile::at(5, 10)->setTier(2);
    world[3][3].lines = LINE_POWER | LINE_PIPE;
    world[8][14].zone = makeZone(SECTOR_C, 2);
    world[9][9].groundwater = 17;
    world[9][9].landValue = 77;
    ResidentalZone* h = utilHome(4, 11, 21);
    h->fixedTier = true;
    UtilityZone* tower = utilBuilding(12, 11, UTIL_TOWER);
    tower->stored = 123.f;
    svcBuilding(16, 11, SERVICE_POLICE);
    gameClock.restore(400, 0.25f, 2);
    taxRates[SECTOR_I][WEALTH_HIGH] = 15;
    scene.economy.takeLoan();

    std::stringstream file;
    ASSERT_TRUE(scene.writeCity(file));
    const int savedMoney = money;

    // Wreck everything, then load
    clearTestGrid();
    money = 0;
    resetTaxRates();
    scene.economy.reset();
    std::stringstream in(file.str());
    ASSERT_TRUE(scene.readCity(in, false));

    ASSERT_EQ(money, savedMoney);
    ASSERT_EQ(static_cast<int>(gameClock.getTotalDays()), 400);
    ASSERT_EQ(gameClock.getSpeedIndex(), 2);
    ASSERT_EQ(taxRates[SECTOR_I][WEALTH_HIGH], 15);
    ASSERT_EQ(static_cast<int>(scene.economy.loans.size()), 1);
    ASSERT_TRUE(RoadTile::at(5, 10) != NULL && RoadTile::at(5, 10)->tier == 2);
    ASSERT_EQ(RoadTile::at(6, 10)->mask, 2 | 8);                         // autotiling rebuilt
    ASSERT_EQ(static_cast<int>(world[3][3].lines), LINE_POWER | LINE_PIPE);
    ASSERT_EQ(static_cast<int>(world[8][14].zone), static_cast<int>(makeZone(SECTOR_C, 2)));
    ASSERT_EQ(static_cast<int>(world[9][9].groundwater), 17);
    ResidentalZone* h2 = dynamic_cast<ResidentalZone*>(entities[4][11]);
    ASSERT_TRUE(h2 != NULL);
    ASSERT_EQ(h2->population, 21);
    ASSERT_TRUE(h2->fixedTier);
    ASSERT_TRUE(h2->isActive);
    ASSERT_EQ(tileKind(5, 11), std::string("ResidentalZone"));          // sub tiles back
    UtilityZone* t2 = dynamic_cast<UtilityZone*>(entities[12][11]);
    ASSERT_TRUE(t2 != NULL && t2->kind == UTIL_TOWER);
    ASSERT_NEAR(t2->stored, 123.f, 0.01f);
    ASSERT_TRUE(dynamic_cast<ServiceZone*>(entities[16][11]) != NULL);

    std::stringstream junk("NOTASAVE 1");
    ASSERT_FALSE(scene.readCity(junk, false));
    ASSERT_TRUE(dynamic_cast<ResidentalZone*>(entities[4][11]) != NULL);  // a bad file changes nothing
    clearTestGrid();
}

TEST_CASE(Test_Undo_RestoresAndRefunds) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 5000;
    utilRoadRow(10, 0, 30);
    std::ostringstream before;
    scene.writeCity(before);
    scene.placeTile(4, 11, TOOL_SCHOOL);
    ASSERT_EQ(money, 5000 - toolCost(TOOL_SCHOOL));
    scene.undoStack.push_back({before.str(), toolCost(TOOL_SCHOOL)});
    money -= 300;                                  // the city kept running meanwhile
    ASSERT_TRUE(scene.undoLast());
    ASSERT_TRUE(entities[4][11] == NULL);
    ASSERT_EQ(money, 5000 - 300);                  // the school's price comes back, later spending stays
    ASSERT_FALSE(scene.undoLast());                // nothing left
    clearTestGrid();
}

TEST_CASE(Test_Services_CoverageAndGarbage) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilRoadRow(10, 0, 40);
    ResidentalZone* home = utilHome(4, 11, 200);
    svcBuilding(8, 11, SERVICE_POLICE);
    svcBuilding(14, 11, SERVICE_CLINIC);
    roadNet.rebuild();
    services.update();
    ASSERT_EQ(static_cast<int>(services.policeMap[4][11]), 100);
    ASSERT_EQ(static_cast<int>(services.clinicMap[30][30]), 0);
    ASSERT_FALSE(services.grace);
    ASSERT_FALSE(services.garbageOk(4, 11));            // 200 people, no landfill

    HappinessSystem hs;
    hs.updateSimulation();
    const int unsafe = home->happinessIndex[3], ill = home->happinessIndex[4];
    svcBuilding(20, 11, SERVICE_LANDFILL);
    roadNet.rebuild();
    services.update();
    ASSERT_TRUE(services.garbageOk(4, 11));
    ASSERT_NEAR(services.garbageMade, 200.f * GARBAGE_PER_CAPITA, 0.01f);
    hs.updateSimulation();
    ASSERT_EQ(home->happinessIndex[4] - ill, 15);       // collected garbage = healthier
    ASSERT_EQ(home->happinessIndex[3], unsafe);

    // A landfill on another road network does not help
    delete entities[20][11]; entities[20][11] = NULL;
    for (int x = 50; x <= 60; x++) entities[x][40] = new DirtRoad(x, 40);
    svcBuilding(54, 41, SERVICE_LANDFILL);
    roadNet.rebuild();
    services.update();
    ASSERT_FALSE(services.garbageOk(4, 11));
    clearTestGrid();
}

TEST_CASE(Test_Services_FiresBurnWithoutStation) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilRoadRow(10, 0, 40);
    ResidentalZone* a = utilHome(4, 11, 50);
    ResidentalZone* b = utilHome(30, 11, 50);
    roadNet.rebuild();
    services.update();

    // Small towns never burn
    a->fireDays = 1;
    services.runDay(10, 100);
    ASSERT_EQ(a->fireDays, 0);

    // Unchecked, a fire destroys the building after FIRE_BURN_DAYS
    a->fireDays = 1;
    for (int d = 0; d <= FIRE_BURN_DAYS && entities[4][11] != NULL; d++) services.runDay(20 + d, 1000);
    ASSERT_TRUE(entities[4][11] == NULL);

    // A fire station in range puts it out the next day
    svcBuilding(26, 11, SERVICE_FIRE);
    roadNet.rebuild();
    services.update();
    b->fireDays = 1;
    services.runDay(40, 1000);
    ASSERT_EQ(b->fireDays, 0);
    ASSERT_TRUE(entities[30][11] != NULL);
    bool putOut = false;
    for (const auto& e : services.events) putOut |= (e.kind == 1 && e.x == 30);
    ASSERT_TRUE(putOut);
    clearTestGrid();
}

TEST_CASE(Test_Services_MinesAndBuses) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    money = 10000;
    utilRoadRow(10, 0, 40);
    std::string reason;
    ASSERT_FALSE(scene.canPlaceTile(4, 11, TOOL_MINE, &reason));          // no deposit here
    world[5][11].deposit = Deposit::Coal;
    world[5][11].depositAmount = 80;
    ASSERT_TRUE(scene.canPlaceTile(4, 11, TOOL_MINE));
    scene.placeTile(4, 11, TOOL_MINE);
    ServiceZone* mine = dynamic_cast<ServiceZone*>(entities[4][11]);
    ASSERT_TRUE(mine != NULL);
    ASSERT_EQ(mine->sector(), static_cast<int>(SECTOR_I));
    ASSERT_EQ(mine->capacity, SERVICE_MINE_JOBS);
    mine->population = 30;
    ASSERT_TRUE(mine->dailyTax() > 0.f);
    services.update();
    ASSERT_EQ(services.activeMines, 1);
    for (int d = 0; d < 10; d++) services.runDay(d, 0);                   // 30 miners x 0.02 = 0.6 a day
    ASSERT_TRUE(world[5][11].depositAmount >= 74 && world[5][11].depositAmount <= 75);   // ~6 points used

    // Bus depot: fewer car commuters nearby
    ResidentalZone* home = utilHome(20, 11, 100);
    CommericalZone* shop = new CommericalZone(26, 11, 1, 1);
    entities[26][11] = shop;
    shop->checkValid();
    shop->population = 100;
    roadNet.rebuild();
    traffic.update();
    const int cars = traffic.totalTrips;
    svcBuilding(16, 11, SERVICE_BUS_DEPOT);
    services.update();
    ASSERT_EQ(static_cast<int>(services.busMap[20][11]), 100);
    traffic.update();
    ASSERT_TRUE(traffic.totalTrips < cars);
    ASSERT_EQ(traffic.totalTrips, static_cast<int>(100.f * COMMUTER_SHARE * (1.f - BUS_RIDER_SHARE) + 0.5f));
    (void)home;
    clearTestGrid();
}

TEST_CASE(Test_LandValue_GatesUpgradesAndGrowth) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilRoadRow(10, 0, 40);
    // Land value from services / water / pollution
    static int park[tileMapW][tileMapH], edu[tileMapW][tileMapH], com[tileMapW][tileMapH], pol[tileMapW][tileMapH];
    static float jam[tileMapW][tileMapH];
    std::memset(park, 0, sizeof(park)); std::memset(edu, 0, sizeof(edu)); std::memset(com, 0, sizeof(com));
    std::memset(pol, 0, sizeof(pol)); std::memset(jam, 0, sizeof(jam));
    park[5][5] = 100; edu[5][5] = 100;
    pol[7][7] = 60;
    world[20][20].terrain = Terrain::Water;
    services.computeLandValue(park, edu, com, pol, jam);
    ASSERT_EQ(static_cast<int>(world[5][5].landValue), 65);        // 30 + 20 + 15
    ASSERT_EQ(static_cast<int>(world[7][7].landValue), 0);         // 30 - 30
    ASSERT_EQ(static_cast<int>(world[21][21].landValue), 42);      // 30 + water nearby
    ASSERT_EQ(static_cast<int>(world[20][20].landValue), 0);

    // Low land value blocks the upgrade a happy, full, wanted building would get
    EconomySystem econ;
    econ.happiness = 90;
    for (int w = 0; w < WEALTH_COUNT; w++) econ.demand[SECTOR_R][w] = 80.f;
    ResidentalZone* h = utilHome(4, 11, 0);
    h->population = h->capacity;
    world[4][11].landValue = LAND_VALUE_TIER2 - 1;
    for (int d = 0; d < 100; d++) { econ.dayCounter++; econ.updateOccupancy(); h->overallHappiness = 90; }
    ASSERT_EQ(h->tier, 1);
    world[4][11].landValue = LAND_VALUE_TIER2;
    for (int d = 0; d < 100; d++) { econ.dayCounter++; econ.updateOccupancy(); h->overallHappiness = 90; }
    ASSERT_EQ(h->tier, 2);                                         // tier 3 needs 60
    clearTestGrid();

    // Zones grow the richest wanted tier the density and land value allow
    Play_Scene scene;
    money = 10000;
    utilRoadRow(10, 0, 30);
    for (const auto& o : FOOTPRINT_2X2) {
        scene.placeTile(4 + o[0], 11 + o[1], TOOL_ZONE_R_HIGH);
        world[4 + o[0]][11 + o[1]].landValue = 70;
    }
    scene.economy.demand[SECTOR_R][WEALTH_LOW] = -10.f;
    scene.economy.demand[SECTOR_R][WEALTH_MID] = 40.f;
    scene.economy.demand[SECTOR_R][WEALTH_HIGH] = -50.f;
    ASSERT_EQ(scene.growZones(), 1);
    ResidentalZone* grown = dynamic_cast<ResidentalZone*>(entities[4][11]);
    ASSERT_TRUE(grown != NULL);
    ASSERT_EQ(grown->tier, 2);
    ASSERT_TRUE(grown->zoned);
    clearTestGrid();
}

// -------------------------------------------------------------
// Waste, sewage, tax undo, buses
// -------------------------------------------------------------
TEST_CASE(Test_Waste_DisposalOrderAndLandfillFills) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilRoadRow(10, 0, 40);
    ResidentalZone* home = utilHome(4, 11, 1000);             // 100 garbage a day
    ServiceZone* fill = svcBuilding(20, 11, SERVICE_LANDFILL);
    roadNet.rebuild();
    services.update();
    ASSERT_TRUE(services.garbageOk(4, 11));
    ASSERT_NEAR(services.dumped, 100.f, 0.01f);
    services.runDay(1, 0);                                    // the day's garbage goes in
    ASSERT_NEAR(fill->stored, 100.f, 0.01f);

    // Recycling takes it first, so the landfill stops filling
    ServiceZone* rec = svcBuilding(24, 11, SERVICE_RECYCLING);
    services.update();
    ASSERT_NEAR(services.recycled, 100.f, 0.01f);
    ASSERT_NEAR(fill->handled, 0.f, 0.01f);
    delete entities[24][11]; entities[24][11] = NULL;
    (void)rec;

    // A nearly full landfill can't take a whole day's load; once full it takes nothing
    fill->stored = LANDFILL_SPACE - 40.f;                     // 40 of 100: under half
    services.update();
    ASSERT_FALSE(services.garbageOk(4, 11));
    home->population = 400;                                   // 40 a day fits
    services.update();
    ASSERT_TRUE(services.garbageOk(4, 11));
    services.runDay(2, 0);
    ASSERT_NEAR(fill->stored, LANDFILL_SPACE, 0.01f);
    bool fullEvent = false;
    for (const auto& e : services.events) fullEvent |= e.kind == 3;
    ASSERT_TRUE(fullEvent);
    services.update();
    ASSERT_EQ(services.fullLandfills, 1);
    ASSERT_FALSE(services.garbageOk(4, 11));
    clearTestGrid();
}

TEST_CASE(Test_Waste_IncineratorMakesPower) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilRoadRow(10, 0, 40);
    utilHome(4, 11, 400);                                     // 40 garbage a day
    utilBuilding(8, 11, UTIL_TRANSFORMER);
    svcBuilding(20, 11, SERVICE_INCINERATOR);
    roadNet.rebuild();
    services.update();
    ASSERT_NEAR(services.burnt, 40.f, 0.01f);
    utilities.refreshService();
    const UtilityGrid* g = utilities.powerGridAt(4, 11);
    ASSERT_TRUE(g != nullptr);
    ASSERT_NEAR(g->powerSupply, 40.f * INCINERATOR_POWER, 0.01f);   // no plant at all, yet power
    clearTestGrid();
}

TEST_CASE(Test_Sewage_PlantsOutflowsAndHealth) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilRoadRow(10, 0, 40);
    ResidentalZone* h = utilHome(4, 11, 300);
    utilities.refreshService();
    ASSERT_TRUE(utilities.sewageGrace);                       // under 400 people: septic tanks
    ASSERT_TRUE(utilities.sewered(4, 11));
    h->population = 500;
    utilities.refreshService();
    ASSERT_FALSE(utilities.sewered(4, 11));

    HappinessSystem hs;
    hs.updateSimulation();
    const int ill = h->happinessIndex[4];
    utilBuilding(20, 11, UTIL_SEWAGE_PLANT);
    utilities.refreshService();
    ASSERT_TRUE(utilities.sewered(4, 11));
    hs.updateSimulation();
    ASSERT_EQ(h->happinessIndex[4] - ill, 15);

    // Outflows must touch water (but need no road)
    Play_Scene scene;
    money = 10000;
    std::string reason;
    ASSERT_FALSE(scene.canPlaceTile(30, 30, TOOL_SEWAGE_OUTFLOW, &reason));
    ASSERT_EQ(reason, std::string("Sewage Outflow Must Touch Water"));
    world[29][30].terrain = Terrain::Water;
    ASSERT_TRUE(scene.canPlaceTile(30, 30, TOOL_SEWAGE_OUTFLOW));
    clearTestGrid();
}

TEST_CASE(Test_Undo_TaxChange) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    scene.setBudgetOpen(true);
    sf::FloatRect t = scene.taxSliderBounds(SECTOR_R, WEALTH_LOW);
    sf::Vector2f p(t.left + t.width * 0.9f, t.top + t.height / 2.f);
    ASSERT_TRUE(scene.handleBudgetPointer(p, true, true));
    scene.handleBudgetPointer(p, false, false);             // release
    ASSERT_EQ(taxRates[SECTOR_R][WEALTH_LOW], 18);
    ASSERT_TRUE(scene.undoLast());
    ASSERT_EQ(taxRates[SECTOR_R][WEALTH_LOW], TAX_DEFAULT);
    clearTestGrid();
}

TEST_CASE(Test_Cars_BusesLeaveDepots) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilRoadRow(10, 0, 30);
    for (int x = 0; x <= 30; x++) RoadTile::at(x, 10)->check();
    svcBuilding(10, 11, SERVICE_BUS_DEPOT);
    roadNet.rebuild();
    CarRenderer cars;
    for (int k = 0; k < 40; k++) cars.update(0.25f);
    int buses = 0;
    for (const auto& c : cars.cars) {
        if (c.model == CarRenderer::BUS) buses++;
        ASSERT_TRUE(RoadTile::at(c.x, c.y) != NULL);
    }
    ASSERT_TRUE(buses >= 1 && buses <= BUSES_PER_DEPOT);
    clearTestGrid();
}

// -------------------------------------------------------------
// Settings, ranks, trade, stats, menus, resumable traffic
// -------------------------------------------------------------
TEST_CASE(Test_Settings_SaveLoadAndClamp) {
    GameSettings a;
    a.sfxVolume = 30;
    a.ambientVolume = 150;          // clamped to 100 on load
    a.cameraSpeed = 1.5f;
    a.autosave = false;
    a.tips = false;
    a.difficulty = 2;
    a.save("test_settings.cfg");
    GameSettings b;
    b.load("test_settings.cfg");
    std::remove("test_settings.cfg");
    ASSERT_EQ(b.sfxVolume, 30);
    ASSERT_EQ(b.ambientVolume, 100);
    ASSERT_NEAR(b.cameraSpeed, 1.5f, 0.001f);
    ASSERT_FALSE(b.autosave);
    ASSERT_FALSE(b.tips);
    ASSERT_EQ(b.difficulty, 2);
    ASSERT_EQ(GameSettings::startingMoneyFor(0), 50000);
    ASSERT_EQ(GameSettings::startingMoneyFor(2), 10000);
}

TEST_CASE(Test_Milestones_UnlockAndReward) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    scene.milestonesEnabled = true;
    money = 10000;
    utilRoadRow(10, 0, 30);
    std::string reason;
    ASSERT_FALSE(scene.canPlaceTile(5, 5, TOOL_ROAD_AVENUE, &reason));
    ASSERT_EQ(reason, std::string("Unlocks when Chime becomes a Town (1000 people)"));
    ASSERT_TRUE(scene.canPlaceTile(5, 5, TOOL_ROAD));

    townPopulation[0] = 1200;
    scene.checkMilestones();
    ASSERT_EQ(scene.cityRank(), 2);                                      // Village and Town in one go
    ASSERT_EQ(money, 10000 + CITY_RANKS[1].reward + CITY_RANKS[2].reward);
    ASSERT_TRUE(scene.canPlaceTile(5, 5, TOOL_ROAD_AVENUE));
    ASSERT_FALSE(scene.canPlaceTile(4, 11, TOOL_RECYCLING));             // a City building

    townPopulation[0] = 100;                                             // ranks never go back down
    scene.checkMilestones();
    ASSERT_EQ(scene.cityRank(), 2);
    scene.milestonesEnabled = false;                                     // sandbox / dev runs: all unlocked
    ASSERT_TRUE(scene.canPlaceTile(4, 11, TOOL_RECYCLING));
    townPopulation[0] = 0;
    clearTestGrid();
}

TEST_CASE(Test_Trade_ExportsImportsAndShortages) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilRoadRow(10, 0, 30);                                              // (0, 10) leaves the map
    IndustrialZone* plant = new IndustrialZone(20, 11, 1, 1);
    entities[20][11] = plant;
    plant->checkValid();
    plant->population = 100;                                             // 100 goods
    utilHome(4, 11, 200);                                                // uses 20 goods, 30 food
    roadNet.rebuild();
    trade.runDay();
    ASSERT_NEAR(trade.capacity, TRADE_PER_EDGE_ROAD, 0.01f);
    ASSERT_NEAR(trade.goodsExported, 80.f, 0.01f);
    ASSERT_NEAR(trade.foodImported, 30.f, 0.01f);
    ASSERT_NEAR(trade.exportIncome, 80.f * GOODS_PRICE, 0.01f);
    ASSERT_NEAR(trade.importCost, 30.f * FOOD_PRICE * IMPORT_MARKUP, 0.01f);
    ASSERT_NEAR(trade.shortage, 0.f, 0.001f);

    EconomySystem econ;
    DayReport rep = econ.settle();
    ASSERT_NEAR(rep.tradeIncome, trade.exportIncome, 0.01f);
    ASSERT_NEAR(rep.tradeCost, trade.importCost, 0.01f);

    // Cut the road to the edge: nothing goes in or out
    delete entities[0][10]; entities[0][10] = NULL;
    roadNet.rebuild();
    trade.runDay();
    ASSERT_NEAR(trade.capacity, 0.f, 0.01f);
    ASSERT_NEAR(trade.glut, 0.8f, 0.001f);                               // 80 of 100 goods stuck
    ASSERT_NEAR(trade.shortage, 0.6f, 0.001f);                           // 30 of 50 needs unmet
    clearTestGrid();
}

TEST_CASE(Test_Waste_PartialPickupCounts) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilRoadRow(10, 0, 40);
    utilHome(4, 11, 1000);                                               // 100 garbage a day
    ServiceZone* fill = svcBuilding(20, 11, SERVICE_LANDFILL);
    fill->stored = LANDFILL_SPACE - 60.f;                                // room for 60
    roadNet.rebuild();
    services.update();
    ASSERT_TRUE(services.garbageOk(4, 11));                              // more than half taken
    ASSERT_NEAR(services.garbageCollected, 60.f, 0.01f);
    clearTestGrid();
}

TEST_CASE(Test_Traffic_ResumableMatchesFullDay) {
    clearTestGrid();
    ScopedCoutSilence silence;
    utilRoadRow(10, 0, 30);
    for (int x = 0; x <= 30; x++) RoadTile::at(x, 10)->check();
    for (int x : {2, 6, 10}) utilHome(x, 11, 50);
    CommericalZone* shop = new CommericalZone(24, 11, 1, 1);
    entities[24][11] = shop;
    shop->checkValid();
    shop->population = 40;
    traffic.update();
    const int trips = traffic.totalTrips, outside = traffic.outsideTrips;
    const float load = traffic.load[15][10];
    ASSERT_TRUE(trips > 0 && outside > 0);                              // 90 commuters, 40 jobs

    traffic.reset();
    traffic.begin();
    int steps = 0;
    while (!traffic.step(1)) steps++;                                    // one home per call
    ASSERT_EQ(steps, 2);
    ASSERT_FALSE(traffic.inProgress());
    ASSERT_EQ(traffic.totalTrips, trips);
    ASSERT_EQ(traffic.outsideTrips, outside);
    ASSERT_NEAR(traffic.load[15][10], load, 0.001f);
    clearTestGrid();
}

TEST_CASE(Test_Stats_HistoryIsCapped) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    DayReport rep;
    rep.net = 7;
    for (int d = 0; d < 400; d++) scene.recordHistory(rep);
    ASSERT_EQ(scene.history.size(), Play_Scene::HISTORY_DAYS);
    ASSERT_EQ(scene.history.back().net, 7);
    clearTestGrid();
}

TEST_CASE(Test_Menu_NewCityFlow) {
    clearTestGrid();
    ScopedCoutSilence silence;
    Play_Scene scene;
    scene.menuPage = Play_Scene::MENU_TITLE;
    bool hasNew = false;
    for (const auto& b : scene.menuButtons()) hasNew |= b.id == MB_NEW && b.enabled;
    ASSERT_TRUE(hasNew);
    scene.handleMenuClick(MB_NEW);
    ASSERT_TRUE(scene.menuPage == Play_Scene::MENU_NEW);
    scene.beginNewCity(1234u, 0, false);
    ASSERT_TRUE(scene.menuPage == Play_Scene::MENU_NONE);
    ASSERT_EQ(money, 50000);
    ASSERT_EQ(static_cast<int>(worldSeed), 1234);
    ASSERT_TRUE(scene.milestonesEnabled);
    ASSERT_EQ(scene.cityRank(), 0);
    clearTestGrid();
}

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
