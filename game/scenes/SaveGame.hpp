#pragma once
// -------------------------------------------------------------
// Save / load (included at the end of Play_Scene.hpp)
// -------------------------------------------------------------
// A plain-text file, one value per token:
//
//   CHIMESAVE <version>
//   seed money spawnX spawnY
//   clock <totalDays> <dayProgress> <speedIndex>
//   tax <9 rates>
//   economy <daysInDebt> <bankrupt> <dayCounter> <carry>
//   loans <n> { <remainingDays> <dailyPayment> }
//   tiles  4096 x { terrain fertility groundwater groundwaterMax deposit amount lines zone }
//   roads <n> { x y tier bridgeAxis roundabout }
//   buildings <n> { name x y tier variant population capacity fixedTier zoned maxTier happiness extra stored }
//
// Everything else (road masks, road networks, utility grids, traffic,
// happiness, demand) is rebuilt from that on load.

#include <fstream>
#include <sstream>

inline const char* SAVE_MAGIC = "CHIMESAVE";
const int SAVE_VERSION = 3;   // 2: + progress line, 3: + rank reached

// Builds a 2x2 structure of the given entity name with its three sub tiles.
// Returns the root, or NULL for an unknown name / blocked spot.
Zone* Play_Scene::createStructure(const std::string& name, int x, int y, int tier, int variant, int extra) {
    if (x < 1 || x + 1 >= tileMapW || y < 0 || y + 1 >= tileMapH) return NULL;
    Zone* z = NULL;
    std::string base = "grass";
    if (name == "ResidentalZone")      { z = new ResidentalZone(x, y, tier, variant); base = "zone_residental"; }
    else if (name == "CommericalZone") { z = new CommericalZone(x, y, tier, variant); base = "zone_commercial"; }
    else if (name == "IndustrialZone") { z = new IndustrialZone(x, y, tier, variant); base = "zone_industrial"; }
    else if (name == "FarmZone")       { z = new FarmZone(x, y, variant); base = "zone_farm"; }
    else if (name == "SchoolZone")     z = new SchoolZone(x, y);
    else if (name == "ChurchZone")     z = new ChurchZone(x, y);
    else if (name == "ParkZone")       z = new ParkZone(x, y);
    else if (name == "HarbourZone")    z = new HarbourZone(x, y);
    else if (name == "UtilityZone")    z = new UtilityZone(x, y, extra);
    else if (serviceKindOfName(name) >= 0) z = new ServiceZone(x, y, serviceKindOfName(name));
    if (z == NULL) return NULL;
    for (const auto& o : FOOTPRINT_2X2) {
        int tx = x + o[0], ty = y + o[1];
        delete entities[tx][ty];
        entities[tx][ty] = (o[0] == 0 && o[1] == 0) ? static_cast<Entity*>(z)
                                                    : new SubZone(tx, ty, x, y, name, base);
    }
    return z;
}

bool Play_Scene::saveCity(const std::string& path) {
    std::ofstream out(path, std::ios::trunc);
    return out && writeCity(out);
}

bool Play_Scene::loadCity(const std::string& path) {
    std::ifstream in(path);
    if (!in) return loadFail("cannot open " + path);
    return readCity(in, false);
}

bool Play_Scene::writeCity(std::ostream& out) {
    out << SAVE_MAGIC << ' ' << SAVE_VERSION << '\n';
    out << worldSeed << ' ' << money << ' ' << worldSpawnX << ' ' << worldSpawnY << '\n';
    out << "clock " << gameClock.getTotalDays() << ' ' << gameClock.getDayProgress() << ' ' << gameClock.getSpeedIndex() << '\n';
    out << "tax";
    for (int s = 0; s < SECTOR_COUNT; s++) for (int w = 0; w < WEALTH_COUNT; w++) out << ' ' << taxRates[s][w];
    out << '\n';
    out << "economy " << economy.daysInDebt << ' ' << (economy.bankrupt ? 1 : 0) << ' ' << economy.dayCounter << ' '
        << economy.getCarry() << '\n';
    out << "loans " << economy.loans.size();
    for (const Loan& l : economy.loans) out << ' ' << l.remainingDays << ' ' << l.dailyPayment;
    out << '\n';
    out << "progress " << bestPopulation << ' ' << (milestonesEnabled ? 1 : 0) << ' ' << rankReached << '\n';

    out << "tiles\n";
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            const TileData& t = world[i][j];
            out << static_cast<int>(t.terrain) << ' ' << static_cast<int>(t.fertility) << ' ' << static_cast<int>(t.groundwater) << ' '
                << static_cast<int>(t.groundwaterMax) << ' ' << static_cast<int>(t.deposit) << ' ' << static_cast<int>(t.depositAmount) << ' '
                << static_cast<int>(t.lines) << ' ' << static_cast<int>(t.zone) << ' ' << static_cast<int>(t.landValue) << '\n';
        }
    }

    std::vector<RoadTile*> roads;
    std::vector<Zone*> buildings;
    for (int i = 0; i < tileMapW; i++)
        for (int j = 0; j < tileMapH; j++) {
            if (RoadTile* r = RoadTile::at(i, j)) { roads.push_back(r); continue; }
            Zone* z = dynamic_cast<Zone*>(entities[i][j]);
            if (z != NULL && z->isMultiTile && !z->isSubTile) buildings.push_back(z);
        }
    out << "roads " << roads.size() << '\n';
    for (RoadTile* r : roads)
        out << r->posX << ' ' << r->posY << ' ' << r->tier << ' ' << r->bridgeAxis << ' ' << (r->roundabout ? 1 : 0) << '\n';
    out << "buildings " << buildings.size() << '\n';
    for (Zone* z : buildings) {
        int extra = 0;
        float stored = 0.f;
        if (UtilityZone* u = dynamic_cast<UtilityZone*>(z)) { extra = u->kind; stored = u->stored; }
        else if (ServiceZone* sv = dynamic_cast<ServiceZone*>(z)) {
            extra = sv->kind;
            stored = sv->kind == SERVICE_LANDFILL ? sv->stored : sv->depleteCarry;   // landfill fill / mine progress
        }
        out << z->getName() << ' ' << z->posX << ' ' << z->posY << ' ' << z->tier << ' ' << z->variant << ' '
            << z->population << ' ' << z->capacity << ' ' << (z->fixedTier ? 1 : 0) << ' ' << (z->zoned ? 1 : 0) << ' '
            << z->maxTier << ' ' << z->overallHappiness << ' ' << extra << ' ' << stored << '\n';
    }
    return static_cast<bool>(out);
}

// keepTime (undo): only the map, roads and buildings come back; the calendar,
// money, taxes, loans and debt stay as they are now
bool Play_Scene::readCity(std::istream& in, bool keepTime) {
    std::string magic, word;
    int version = 0;
    in >> magic >> version;
    if (magic != SAVE_MAGIC || version < 1 || version > SAVE_VERSION) return loadFail("step 1");

    uint32_t seed = 0;
    int savedMoney = 0, sx = 0, sy = 0;
    long days = 0;
    float progress = 0.f;
    int speed = 1;
    in >> seed >> savedMoney >> sx >> sy;
    in >> word >> days >> progress >> speed;
    if (!in || word != "clock") return loadFail("step 2");
    int tax[SECTOR_COUNT][WEALTH_COUNT];
    in >> word;
    for (auto& row : tax) for (int& r : row) in >> r;
    int debtDays = 0, bankrupt = 0;
    long dayCounter = 0;
    float carry = 0.f;
    in >> word >> debtDays >> bankrupt >> dayCounter >> carry;
    size_t loanCount = 0;
    in >> word >> loanCount;
    std::vector<Loan> loans(loanCount);
    for (Loan& l : loans) in >> l.remainingDays >> l.dailyPayment;
    int best = 0, ranked = 0, rankSaved = -1;
    if (version >= 2) in >> word >> best >> ranked;   // progress line
    if (version >= 3) in >> rankSaved;
    if (!in || loanCount > 100) return loadFail("step 3");

    // Tiles go into a scratch copy first, so a broken file leaves the city alone
    std::vector<TileData> tiles(static_cast<size_t>(tileMapW * tileMapH));
    in >> word;
    if (word != "tiles") return loadFail("step 4");
    for (TileData& t : tiles) {
        int v[9];
        for (int& x : v) in >> x;
        t.terrain = static_cast<Terrain>(v[0]);
        t.fertility = static_cast<uint8_t>(v[1]);
        t.groundwater = static_cast<uint8_t>(v[2]);
        t.groundwaterMax = static_cast<uint8_t>(v[3]);
        t.deposit = static_cast<Deposit>(v[4]);
        t.depositAmount = static_cast<uint8_t>(v[5]);
        t.lines = static_cast<uint8_t>(v[6]);
        t.zone = static_cast<uint8_t>(v[7]);
        t.landValue = static_cast<uint8_t>(v[8]);
    }
    if (!in) return loadFail("step 5");

    struct RoadRec { int x, y, tier, axis, round; };
    struct BuildRec { std::string name; int x, y, tier, variant, pop, cap, fixed, zoned, maxTier, happy, extra; float stored; };
    size_t n = 0;
    in >> word >> n;
    if (word != "roads" || n > 4096) return loadFail("step 6");
    std::vector<RoadRec> roads(n);
    for (RoadRec& r : roads) in >> r.x >> r.y >> r.tier >> r.axis >> r.round;
    in >> word >> n;
    if (word != "buildings" || n > 4096) return loadFail("step 7");
    std::vector<BuildRec> builds(n);
    for (BuildRec& b : builds)
        in >> b.name >> b.x >> b.y >> b.tier >> b.variant >> b.pop >> b.cap >> b.fixed >> b.zoned >> b.maxTier >> b.happy >> b.extra >> b.stored;
    if (!in) return loadFail("step 8");

    // ---- apply ----
    materializeWorld();
    for (int i = 0; i < tileMapW; i++)
        for (int j = 0; j < tileMapH; j++) world[i][j] = tiles[static_cast<size_t>(i * tileMapH + j)];
    worldSeed = seed;
    worldSpawnX = sx;
    worldSpawnY = sy;
    if (!keepTime) {
        money = savedMoney;
        gameClock = GameClock();
        gameClock.restore(days, progress, speed);
        for (int s = 0; s < SECTOR_COUNT; s++) for (int w = 0; w < WEALTH_COUNT; w++) taxRates[s][w] = tax[s][w];
        economy.reset();
        economy.daysInDebt = debtDays;
        economy.bankrupt = bankrupt != 0;
        economy.dayCounter = dayCounter;
        economy.setCarry(carry);
        economy.loans = loans;
        bestPopulation = best;
        rankReached = rankSaved >= 0 ? std::min(rankSaved, CITY_RANK_COUNT - 1) : rankForPopulation(best);   // old saves: by population
        milestonesEnabled = version >= 2 ? ranked != 0 : true;
        undoStack.clear();
        notices.clear();
        history.clear();
        trade.reset();
        happinessSystem = HappinessSystem();
        lastDailyIncome = 0;
        simAccumulator = 0.f;
        setBudgetOpen(false);
    }
    roadNet.clear();
    traffic.reset();
    cars.clear();
    utilities.reset();
    services.reset();
    minimapDirty = true;

    for (const RoadRec& r : roads) {
        if (!inWorldBounds(r.x, r.y)) continue;
        RoadTile* t = new RoadTile(r.x, r.y, r.tier, r.axis);
        t->roundabout = r.round != 0;
        entities[r.x][r.y] = t;
    }
    for (const BuildRec& b : builds) {
        Zone* z = createStructure(b.name, b.x, b.y, b.tier, b.variant, b.extra);
        if (z == NULL) continue;
        z->capacity = b.cap;
        z->population = b.pop;
        z->fixedTier = b.fixed != 0;
        z->zoned = b.zoned != 0;
        z->maxTier = b.maxTier;
        z->overallHappiness = b.happy;
        if (UtilityZone* u = dynamic_cast<UtilityZone*>(z)) u->stored = b.stored;
        if (ServiceZone* sv = dynamic_cast<ServiceZone*>(z)) {
            if (sv->kind == SERVICE_LANDFILL) sv->stored = b.stored;
            else sv->depleteCarry = b.stored;
        }
    }
    for (int i = 0; i < tileMapW; i++)
        for (int j = 0; j < tileMapH; j++) {
            if (RoadTile* r = RoadTile::at(i, j)) r->check();
            Zone* z = dynamic_cast<Zone*>(entities[i][j]);
            if (z != NULL && z->isMultiTile && !z->isSubTile) z->checkValid();
        }

    roadNet.rebuild();
    economy.scan();
    utilities.refreshService();
    utilitiesDirty = false;
    services.update();
    traffic.update();
    happinessSystem.updateSimulation();
    ground.markDirty();
    ground.setSeasonLook(seasonLookAt(gameClock.getSeason(), gameClock.seasonProgress()));
    if (!keepTime) {
        sf::Vector2f spawn = tileScreenPos(worldSpawnX, worldSpawnY);
        view.setCenter(sf::Vector2f(spawn.x + tileSizeX / 2.f, spawn.y + tileSizeY / 2.f));
    }
    return true;
}
