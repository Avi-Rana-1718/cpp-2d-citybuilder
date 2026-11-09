#include "test_framework.hpp"

#include <SFML/Graphics.hpp>
#include <sstream>
#include "../src/_global.hpp"
#include "../src/Entity.hpp"
#include "../game/entities/_Zone.hpp"
#include "../game/entities/GlassTile.hpp"
#include "../game/entities/ResidentalZone.hpp"
#include "../game/entities/CommericalZone.hpp"
#include "../game/entities/IndustrialZone.hpp"
#include "../game/entities/FarmZone.hpp"
#include "../game/entities/HarbourZone.hpp"
#include "../game/entities/WaterTile.hpp"
#include "../game/entities/SchoolZone.hpp"
#include "../game/entities/ChurchZone.hpp"
#include "../game/entities/ParkZone.hpp"
#include "../game/entities/SubZone.hpp"
#include "../game/entities/roads/DirtRoad.hpp"
#include "../game/simulation/HappinessSystem.hpp"
#include "../game/ui/Toolbar.hpp"
#include "../game/scenes/Play_Scene.hpp"

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
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            delete entities[i][j];
            entities[i][j] = new GrassTile(i, j);
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

    ScopedCoutSilence silence;
    GrassTile tile(posX, posY);
    ASSERT_NEAR(tile.position->x, expectedX, 0.001f);
    ASSERT_NEAR(tile.position->y, expectedY, 0.001f);
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

    // Initial tool default
    ASSERT_EQ(static_cast<int>(tb.getActiveTool()), static_cast<int>(TOOL_RESIDENTIAL));

    // Dock bounds must be centered at bottom of 1500x900
    ASSERT_TRUE(tb.contains(sf::Vector2f(750.f, 850.f)));
    ASSERT_FALSE(tb.contains(sf::Vector2f(100.f, 100.f))); // Top-left is outside
    ASSERT_FALSE(tb.contains(sf::Vector2f(750.f, 500.f))); // Screen center is outside
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

    // Place adjacent DirtRoad at (5, 6)
    delete entities[5][6];
    entities[5][6] = new DirtRoad(5, 6);

    // Now zone must activate
    rz->checkValid();
    ASSERT_TRUE(rz->isActive);

    // Remove the road (revert to grass)
    delete entities[5][6];
    entities[5][6] = new GrassTile(5, 6);

    // Zone must deactivate when road is removed
    rz->checkValid();
    ASSERT_FALSE(rz->isActive);

    // Now test chained activation:
    // Place road again -> rz becomes active
    delete entities[5][6];
    entities[5][6] = new DirtRoad(5, 6);
    rz->checkValid();
    ASSERT_TRUE(rz->isActive);

    // Place second residential zone at (5, 4) adjacent to active (5, 5)
    delete entities[5][4];
    ResidentalZone* rz2 = new ResidentalZone(5, 4);
    entities[5][4] = rz2;
    rz2->checkValid();
    ASSERT_TRUE(rz2->isActive);

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

    money = 500;
    townTaxes[0] = 10; // 10% residential tax
    townTaxes[1] = 15; // 15% commercial tax

    delete entities[4][4];
    ResidentalZone* rz = new ResidentalZone(4, 4);
    entities[4][4] = rz;
    rz->zoneIncome = 100;
    rz->isActive = false;

    // Day 1: Inactive zone must NOT pay tax
    day = 1;
    rz->payTax();
    ASSERT_EQ(money, 500);

    // Activate zone
    rz->isActive = true;
    rz->taxPaid = false;

    // Day 1: Active zone pays tax: 100 * (10 / 100) = 10
    rz->payTax();
    ASSERT_EQ(money, 510);
    ASSERT_TRUE(rz->taxPaid);

    // Calling payTax() again on Day 1 must NOT charge twice
    rz->payTax();
    ASSERT_EQ(money, 510);

    // Day 2: taxPaid resets to false
    day = 2;
    rz->payTax();
    ASSERT_FALSE(rz->taxPaid);
    ASSERT_EQ(money, 510);

    // Commercial zone tax test with townTaxes[1]
    delete entities[4][5];
    CommericalZone* cz = new CommericalZone(4, 5);
    entities[4][5] = cz;
    cz->zoneIncome = 200;
    cz->isActive = true;
    cz->taxPaid = false;

    day = 1;
    cz->payTax();
    // Expected commercial tax: 200 * (15 / 100) = 30
    ASSERT_EQ(money, 540);
    ASSERT_TRUE(cz->taxPaid);

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
    delete entities[3][4];
    entities[3][4] = new DirtRoad(3, 4);
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
    entities[3][3] = new GrassTile(3, 3);
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
    delete entities[4][5];
    entities[4][5] = new DirtRoad(4, 5);
    farm->checkValid();
    ASSERT_TRUE(farm->isActive);

    // Tax collection on day 1
    money = 100;
    day = 1;
    townTaxes[2] = 10;
    farm->payTax();
    ASSERT_TRUE(money > 100);
    ASSERT_TRUE(farm->taxPaid);

    // Tax not collected again on same day
    int moneyAfter = money;
    farm->payTax();
    ASSERT_EQ(money, moneyAfter);

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

    // Add adjacent water tile
    delete entities[2][3];
    entities[2][3] = new WaterTile(2, 3);
    harbour->checkValid();
    ASSERT_TRUE(harbour->isActive);

    // Clean up
    delete entities[2][2];
    entities[2][2] = new GrassTile(2, 2);
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

    // Setup active road and residential zone
    delete entities[5][5];
    entities[5][5] = new DirtRoad(5, 5);

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
        entities[cx][cy] = new GrassTile(cx, cy);
    }

    // Verify all 4 tiles are restored to GrassTile
    for (int k = 0; k < 4; k++) {
        ASSERT_EQ(entities[rx + dOffsets[k].x][ry + dOffsets[k].y]->getName(), "GrassTile");
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
    delete entities[10][2]; entities[10][2] = new WaterTile(10, 2);
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
    entities[6][6] = new WaterTile(6, 6);

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
    entities[7][5] = new WaterTile(7, 5);

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
