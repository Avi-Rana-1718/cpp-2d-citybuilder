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
// 11. Civic Buildings, Demolition & World Coverage Tests
// -------------------------------------------------------------
TEST_CASE(Test_CivicServices_SchoolAndChurchProperties) {
    ScopedCoutSilence silence;

    SchoolZone school(3, 3);
    ASSERT_EQ(school.getName(), "SchoolZone");
    ASSERT_EQ(school.serviceRadius, 5);
    ASSERT_FALSE(school.isActive);

    ChurchZone church(6, 6);
    ASSERT_EQ(church.getName(), "ChurchZone");
    ASSERT_EQ(church.serviceRadius, 4);
    ASSERT_FALSE(church.isActive);

    ParkZone park(8, 8);
    ASSERT_EQ(park.getName(), "ParkZone");
    ASSERT_EQ(park.serviceRadius, 4);
}

TEST_CASE(Test_Demolition_RestoresGrassTile) {
    clearTestGrid();
    ScopedCoutSilence silence;

    int x = 4, y = 4;
    delete entities[x][y];
    entities[x][y] = new SchoolZone(x, y);
    ASSERT_EQ(entities[x][y]->getName(), "SchoolZone");

    // Road adjacent to activate
    delete entities[x + 1][y];
    entities[x + 1][y] = new DirtRoad(x + 1, y);

    SchoolZone* s = dynamic_cast<SchoolZone*>(entities[x][y]);
    s->checkValid();
    ASSERT_TRUE(s->isActive);

    // Bulldoze SchoolZone -> must restore cleanly to GrassTile
    delete entities[x][y];
    entities[x][y] = new GrassTile(x, y);
    ASSERT_EQ(entities[x][y]->getName(), "GrassTile");

    // Bulldoze Road -> must restore cleanly to GrassTile
    delete entities[x + 1][y];
    entities[x + 1][y] = new GrassTile(x + 1, y);
    ASSERT_EQ(entities[x + 1][y]->getName(), "GrassTile");

    clearTestGrid();
}

TEST_CASE(Test_World_FullBuildingCoverage) {
    clearTestGrid();
    ScopedCoutSilence silence;

    // Place one of each key zone type
    delete entities[2][2]; entities[2][2] = new DirtRoad(2, 2);
    delete entities[3][2]; entities[3][2] = new ResidentalZone(3, 2);
    delete entities[4][2]; entities[4][2] = new CommericalZone(4, 2);
    delete entities[5][2]; entities[5][2] = new IndustrialZone(5, 2);
    delete entities[6][2]; entities[6][2] = new FarmZone(6, 2);
    delete entities[7][2]; entities[7][2] = new SchoolZone(7, 2);
    delete entities[8][2]; entities[8][2] = new ChurchZone(8, 2);
    delete entities[9][2]; entities[9][2] = new ParkZone(9, 2);
    delete entities[10][2]; entities[10][2] = new WaterTile(10, 2);
    delete entities[10][1]; entities[10][1] = new HarbourZone(10, 1);

    ASSERT_EQ(entities[2][2]->getName(), "DirtRoad");
    ASSERT_EQ(entities[3][2]->getName(), "ResidentalZone");
    ASSERT_EQ(entities[4][2]->getName(), "CommericalZone");
    ASSERT_EQ(entities[5][2]->getName(), "IndustrialZone");
    ASSERT_EQ(entities[6][2]->getName(), "FarmZone");
    ASSERT_EQ(entities[7][2]->getName(), "SchoolZone");
    ASSERT_EQ(entities[8][2]->getName(), "ChurchZone");
    ASSERT_EQ(entities[9][2]->getName(), "ParkZone");
    ASSERT_EQ(entities[10][2]->getName(), "WaterTile");
    ASSERT_EQ(entities[10][1]->getName(), "HarbourZone");

    // Harbours adjacent to water activate
    HarbourZone* hb = dynamic_cast<HarbourZone*>(entities[10][1]);
    hb->checkValid();
    ASSERT_TRUE(hb->isActive);

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

    return TestRegistry::instance().runAll();
}
