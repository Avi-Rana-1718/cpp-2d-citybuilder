#pragma once
#include <vector>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include "../../src/_global.hpp"
#include "./EconomyData.hpp"
#include "./GameClock.hpp"
#include "../world/WorldData.hpp"
#include "../world/RoadNetwork.hpp"
#include "../entities/_Zone.hpp"
#include "../entities/UtilityZone.hpp"
#include "../entities/ServiceZone.hpp"

// -------------------------------------------------------------
// Power & water
// -------------------------------------------------------------
// Utilities travel along *conductors*: every road tile carries both, power
// lines carry power and underground pipes carry water (world[][].lines).
// Conductor tiles that share an edge form a network. A building joins
// every network that touches its footprint or its edge border, and in
// doing so links them, so a building touching two networks merges them
// into one *grid*. Power and water have separate grids.
//
// Power:  plants (coal / wind / solar) feed their grid, and transformers on
//         the grid step it down for buildings. A grid delivers the smaller
//         of its plant output and its transformer capacity, to any building
//         on it (no distance limit). No transformer = no power.
// Water:  wells give clean water, underground pumps give raw water. Raw
//         water up to the grid's treatment capacity is cleaned; the rest is
//         still delivered but untreated (a health hazard). Towers keep the
//         clean water left over at the end of a day and cover later
//         shortfalls. Wells and pumps draw down the groundwater reserve in
//         their radius; it recharges slowly towards its natural level.
//
// Sewage: every building's waste water (as much as its water use) runs back
//         through the water network (roads + pipes) to sewage outflows (cheap,
//         must touch water, pollute around them) or treatment plants. Without
//         sewers a home loses health; towns under SEWAGE_GRACE_POP use septic
//         tanks. Incinerators (a garbage facility) also feed their power grid.
//
// Utility buildings need no road, only a conductor. Consumers (homes,
// shops, industry, harbours) are served in scan order until their grid
// runs out. While the whole city is smaller than UTILITY_GRACE_POP
// everyone counts as served.

struct UtilityGrid {
    float powerSupply = 0.f, powerDemand = 0.f, powerUsed = 0.f, transformerCap = 0.f;
    float clean = 0.f, raw = 0.f, treatCap = 0.f, towerStored = 0.f, towerCap = 0.f;
    float waterDemand = 0.f, cleanUsed = 0.f, towerUsed = 0.f, untreatedUsed = 0.f;
    float towerFill = 0.f;   // spare clean water pumped into the towers today
    float sewageCap = 0.f, sewageUsed = 0.f, sewageDemand = 0.f;

    float treated() const { return std::min(raw, treatCap); }
    float cleanPool() const { return clean + treated(); }
    float untreatedPool() const { return raw - treated(); }
};

// Tiles that carry a utility: any road (both utilities) or the matching line
inline bool conductsUtility(int x, int y, uint8_t line) {
    return inWorldBounds(x, y) && (RoadTile::at(x, y) != NULL || (world[x][y].lines & line));
}

class UtilitySystem {
public:
    enum Flag : uint8_t { F_CONSUMER = 1, F_POWER = 2, F_WATER = 4, F_UNTREATED = 8, F_SEWAGE = 16 };

    uint8_t flags[tileMapW][tileMapH];      // at building roots
    int powerGridOf[tileMapW][tileMapH];    // at building roots (-1 = not connected)
    int waterGridOf[tileMapW][tileMapH];
    std::vector<UtilityGrid> powerGrids;    // power fields used
    std::vector<UtilityGrid> waterGrids;    // water fields used

    // City totals (HUD, inspect card, balance printout)
    float powerSupply = 0.f, powerDemand = 0.f, powerDelivered = 0.f;
    float waterSupply = 0.f, waterDemand = 0.f, waterDelivered = 0.f, waterStored = 0.f, waterUntreated = 0.f;
    int consumers = 0, unpowered = 0, unwatered = 0, untreatedHomes = 0;
    float unpoweredShare[SECTOR_COUNT] = {}, unwateredShare[SECTOR_COUNT] = {};
    float untreatedShare = 0.f;   // homes drinking untreated water
    float sewageDemand = 0.f, sewageHandled = 0.f, sewageCapacity = 0.f;
    int unsewered = 0;
    float unseweredShare = 0.f;   // homes without sewers
    int cityPopulation = 0;
    bool grace = true;            // city below UTILITY_GRACE_POP
    bool sewageGrace = true;      // city below SEWAGE_GRACE_POP

    UtilitySystem() { reset(); }

    void reset() {
        std::memset(flags, 0, sizeof(flags));
        for (auto& col : powerGridOf) for (int& g : col) g = -1;
        for (auto& col : waterGridOf) for (int& g : col) g = -1;
        for (auto& col : gwCarry) for (float& c : col) c = 0.f;
        powerGrids.clear();
        waterGrids.clear();
        powerSupply = powerDemand = powerDelivered = 0.f;
        waterSupply = waterDemand = waterDelivered = waterStored = waterUntreated = 0.f;
        consumers = unpowered = unwatered = untreatedHomes = 0;
        for (int s = 0; s < SECTOR_COUNT; s++) unpoweredShare[s] = unwateredShare[s] = 0.f;
        untreatedShare = 0.f;
        sewageDemand = sewageHandled = sewageCapacity = 0.f;
        unsewered = 0;
        unseweredShare = 0.f;
        cityPopulation = 0;
        grace = true;
        sewageGrace = true;
    }

    // ---- per building (pass the root) ----
    bool isConsumer(int x, int y) const { return inWorldBounds(x, y) && (flags[x][y] & F_CONSUMER); }
    bool powered(int x, int y) const { return !isConsumer(x, y) || (flags[x][y] & F_POWER); }
    bool watered(int x, int y) const { return !isConsumer(x, y) || (flags[x][y] & F_WATER); }
    bool untreated(int x, int y) const { return isConsumer(x, y) && (flags[x][y] & F_UNTREATED); }
    bool served(int x, int y) const { return powered(x, y) && watered(x, y); }
    bool sewered(int x, int y) const { return !isConsumer(x, y) || (flags[x][y] & F_SEWAGE); }
    const UtilityGrid* powerGridAt(int x, int y) const {
        int g = inWorldBounds(x, y) ? powerGridOf[x][y] : -1;
        return g >= 0 ? &powerGrids[static_cast<size_t>(g)] : nullptr;
    }
    const UtilityGrid* waterGridAt(int x, int y) const {
        int g = inWorldBounds(x, y) ? waterGridOf[x][y] : -1;
        return g >= 0 ? &waterGrids[static_cast<size_t>(g)] : nullptr;
    }
    // The grid a utility building works on (power kinds: power grid)
    const UtilityGrid* gridFor(const UtilityZone* u) const {
        return isPowerUtility(u->kind) ? powerGridAt(u->posX, u->posY) : waterGridAt(u->posX, u->posY);
    }

    static bool consumes(const Zone* z) {
        return z != NULL && !z->isSubTile && z->sector() >= 0 && const_cast<Zone*>(z)->getName() != "FarmZone";
    }
    // Daily need: occupants (at least 10% of capacity, so empty lots can be
    // served) x per-capita use x tier factor x city-size factor
    static float demandOf(const Zone* z, bool water, int cityPop) {
        if (!consumes(z)) return 0.f;
        int s = z->sector();
        float people = std::max(static_cast<float>(z->population), 0.1f * static_cast<float>(z->capacity));
        float per = water ? WATER_PER_CAPITA[s] : POWER_PER_CAPITA[s];
        return people * per * UTIL_TIER_FACTOR[z->wealth()] * cityFactor(cityPop);
    }
    static float cityFactor(int cityPop) { return 1.f + static_cast<float>(cityPop) / UTIL_CITY_SCALE; }
    static float solarSeasonFactor(Season s) {
        switch (s) {
            case Season::Summer: return 1.2f;
            case Season::Autumn: return 0.8f;
            case Season::Winter: return 0.5f;
            default: return 1.f;
        }
    }
    // What a utility building would produce right now (MW, water units,
    // transformer / treatment capacity or tower storage)
    static float outputOf(const UtilityZone* u) {
        const float base = UTIL_OUTPUT[u->kind];
        switch (u->kind) {
            case UTIL_COAL:  return base * (u->onCoal() ? COAL_DEPOSIT_BONUS : 1.f);
            case UTIL_SOLAR: return base * solarSeasonFactor(gameClock.getSeason());
            case UTIL_WELL:
            case UTIL_PUMP:  return base * UtilityZone::groundwaterAround(u->posX, u->posY, u->drawRadius());
            default:         return base;
        }
    }

    // Recomputes who is served without draining anything (after building)
    void refreshService() { compute(false); }
    // Once per game day: also drains / recharges groundwater and fills towers
    void runDay() { compute(true); }

private:
    float gwCarry[tileMapW][tileMapH];   // fractional groundwater change not yet applied

    static int findRoot(std::vector<int>& p, int a) {
        while (p[static_cast<size_t>(a)] != a) {
            p[static_cast<size_t>(a)] = p[static_cast<size_t>(p[static_cast<size_t>(a)])];
            a = p[static_cast<size_t>(a)];
        }
        return a;
    }

    template <typename Fn>
    static void forEachRoot(Fn fn) {
        for (int i = 0; i < tileMapW; i++)
            for (int j = 0; j < tileMapH; j++) {
                if (entities[i][j] == NULL) continue;
                Zone* z = dynamic_cast<Zone*>(entities[i][j]);
                if (z != NULL && z->isMultiTile && !z->isSubTile) fn(z, i, j);
            }
    }

    // Conductor networks for one utility (BFS over shared edges); returns the count
    static int labelNetworks(uint8_t line, int net[tileMapW][tileMapH]) {
        for (int i = 0; i < tileMapW; i++)
            for (int j = 0; j < tileMapH; j++) net[i][j] = -1;
        int count = 0;
        std::vector<sf::Vector2i> stack;
        for (int i = 0; i < tileMapW; i++)
            for (int j = 0; j < tileMapH; j++) {
                if (net[i][j] >= 0 || !conductsUtility(i, j, line)) continue;
                net[i][j] = count;
                stack.push_back({i, j});
                while (!stack.empty()) {
                    sf::Vector2i p = stack.back();
                    stack.pop_back();
                    for (const auto& d : ROAD_DIR) {
                        int nx = p.x + d[0], ny = p.y + d[1];
                        if (inWorldBounds(nx, ny) && net[nx][ny] < 0 && conductsUtility(nx, ny, line)) {
                            net[nx][ny] = count;
                            stack.push_back({nx, ny});
                        }
                    }
                }
                count++;
            }
        return count;
    }

    // Grids for one utility: networks merged through the buildings that touch
    // them (footprint or edge border). Writes gridOf at building roots.
    void buildGrids(uint8_t line, int gridOf[tileMapW][tileMapH], std::vector<UtilityGrid>& grids) {
        static int net[tileMapW][tileMapH];
        const int n = labelNetworks(line, net);
        std::vector<int> parent(static_cast<size_t>(n));
        for (int k = 0; k < n; k++) parent[static_cast<size_t>(k)] = k;
        for (int i = 0; i < tileMapW; i++)
            for (int j = 0; j < tileMapH; j++) gridOf[i][j] = -1;
        forEachRoot([&](Zone*, int x, int y) {
            int first = -1;
            auto touch = [&](int tx, int ty) {
                if (!inWorldBounds(tx, ty) || net[tx][ty] < 0) return;
                int c = net[tx][ty];
                if (first < 0) first = c;
                else parent[static_cast<size_t>(findRoot(parent, c))] = findRoot(parent, first);
            };
            for (const auto& o : FOOTPRINT_2X2) touch(x + o[0], y + o[1]);
            for (const auto& o : BORDER_2X2) touch(x + o[0], y + o[1]);
            gridOf[x][y] = first; // network for now, mapped to a grid below
        });
        std::vector<int> gridIndex(static_cast<size_t>(n), -1);
        grids.clear();
        forEachRoot([&](Zone*, int x, int y) {
            int c = gridOf[x][y];
            if (c < 0) return;
            int r = findRoot(parent, c);
            if (gridIndex[static_cast<size_t>(r)] < 0) {
                gridIndex[static_cast<size_t>(r)] = static_cast<int>(grids.size());
                grids.push_back(UtilityGrid());
            }
            gridOf[x][y] = gridIndex[static_cast<size_t>(r)];
        });
    }

    void compute(bool day) {
        std::memset(flags, 0, sizeof(flags));

        // 1. Grids (power: roads + power lines, water: roads + pipes)
        buildGrids(LINE_POWER, powerGridOf, powerGrids);
        buildGrids(LINE_PIPE, waterGridOf, waterGrids);
        int pop = 0;
        forEachRoot([&](Zone* z, int, int) { if (consumes(z)) pop += z->population; });
        cityPopulation = pop;
        grace = pop < UTILITY_GRACE_POP;
        sewageGrace = pop < SEWAGE_GRACE_POP;

        // 2. Supply
        std::vector<UtilityZone*> sources;   // wells and pumps, for the drain
        std::vector<UtilityZone*> towers;
        forEachRoot([&](Zone* z, int x, int y) {
            // Incinerators turn today's garbage into power on their grid
            if (ServiceZone* sv = dynamic_cast<ServiceZone*>(z)) {
                if (sv->kind == SERVICE_INCINERATOR && sv->isActive && powerGridOf[x][y] >= 0)
                    powerGrids[static_cast<size_t>(powerGridOf[x][y])].powerSupply += sv->handled * INCINERATOR_POWER;
                return;
            }
            UtilityZone* u = dynamic_cast<UtilityZone*>(z);
            if (u == NULL) return;
            u->output = outputOf(u);
            const bool power = isPowerUtility(u->kind);
            int g = power ? powerGridOf[x][y] : waterGridOf[x][y];
            u->linked = g >= 0;
            if (!u->isActive || g < 0) return;
            UtilityGrid& G = power ? powerGrids[static_cast<size_t>(g)] : waterGrids[static_cast<size_t>(g)];
            switch (u->kind) {
                case UTIL_COAL: case UTIL_WIND: case UTIL_SOLAR: G.powerSupply += u->output; break;
                case UTIL_TRANSFORMER: G.transformerCap += u->output; break;
                case UTIL_WELL:      G.clean += u->output; sources.push_back(u); break;
                case UTIL_PUMP:      G.raw += u->output; sources.push_back(u); break;
                case UTIL_TREATMENT: G.treatCap += u->output; break;
                case UTIL_TOWER:     G.towerCap += u->output; u->stored = std::min(u->stored, u->output);
                                     G.towerStored += u->stored; towers.push_back(u); break;
                case UTIL_SEWAGE_OUTFLOW:
                case UTIL_SEWAGE_PLANT: G.sewageCap += u->output; break;
            }
        });

        // 3. Consumers, in scan order, until the grid runs dry
        int perSector[SECTOR_COUNT] = {}, noPower[SECTOR_COUNT] = {}, noWater[SECTOR_COUNT] = {};
        int homes = 0, homesUnsewered = 0;
        consumers = unpowered = unwatered = untreatedHomes = unsewered = 0;
        forEachRoot([&](Zone* z, int x, int y) {
            if (!consumes(z)) return;
            flags[x][y] = F_CONSUMER;
            const int pg = powerGridOf[x][y], wg = waterGridOf[x][y];
            int s = z->sector();
            consumers++;
            perSector[s]++;
            if (s == SECTOR_R) homes++;
            bool hasPower = false, hasWater = false, dirty = false, hasSewer = false;
            if (z->isActive && pg >= 0) {
                UtilityGrid& G = powerGrids[static_cast<size_t>(pg)];
                const float p = demandOf(z, false, pop);
                G.powerDemand += p;
                // While both the plants and the transformers keep up
                if (std::min(G.powerSupply, G.transformerCap) - G.powerUsed >= p) {
                    G.powerUsed += p;
                    hasPower = true;
                }
            }
            if (z->isActive && wg >= 0) {
                UtilityGrid& G = waterGrids[static_cast<size_t>(wg)];
                const float w = demandOf(z, true, pop);
                G.waterDemand += w;
                // Clean first, then the towers, then untreated raw water
                if (G.cleanPool() - G.cleanUsed >= w) { G.cleanUsed += w; hasWater = true; }
                else if (G.towerStored - G.towerUsed >= w) { G.towerUsed += w; hasWater = true; }
                else if (G.untreatedPool() - G.untreatedUsed >= w) { G.untreatedUsed += w; hasWater = true; dirty = true; }
                // Waste water back out through the sewers
                G.sewageDemand += w;
                if (G.sewageCap - G.sewageUsed >= w) { G.sewageUsed += w; hasSewer = true; }
            }
            if (!hasSewer) { unsewered++; if (s == SECTOR_R) homesUnsewered++; }
            if (sewageGrace) hasSewer = true;
            if (!hasPower) { unpowered++; noPower[s]++; }
            if (!hasWater) { unwatered++; noWater[s]++; }
            if (dirty && s == SECTOR_R) untreatedHomes++;
            if (grace) hasPower = hasWater = true;
            flags[x][y] |= (hasPower ? F_POWER : 0) | (hasWater ? F_WATER : 0) | (dirty ? F_UNTREATED : 0) | (hasSewer ? F_SEWAGE : 0);
        });
        for (int s = 0; s < SECTOR_COUNT; s++) {
            unpoweredShare[s] = perSector[s] > 0 ? static_cast<float>(noPower[s]) / static_cast<float>(perSector[s]) : 0.f;
            unwateredShare[s] = perSector[s] > 0 ? static_cast<float>(noWater[s]) / static_cast<float>(perSector[s]) : 0.f;
        }
        untreatedShare = homes > 0 ? static_cast<float>(untreatedHomes) / static_cast<float>(homes) : 0.f;
        unseweredShare = homes > 0 ? static_cast<float>(homesUnsewered) / static_cast<float>(homes) : 0.f;

        // Plants whose grid has no transformer cannot deliver anything
        forEachRoot([&](Zone* z, int x, int y) {
            UtilityZone* u = dynamic_cast<UtilityZone*>(z);
            if (u == NULL) return;
            const UtilityGrid* g = powerGridAt(x, y);
            u->noTransformer = (u->kind == UTIL_COAL || u->kind == UTIL_WIND || u->kind == UTIL_SOLAR) &&
                               g != nullptr && g->transformerCap <= 0.f;
        });

        if (day) {
            fillTowers(towers);
            drainGroundwater(sources);
            rechargeGroundwater();
        }

        // 4. City totals
        powerSupply = powerDemand = powerDelivered = 0.f;
        waterSupply = waterDemand = waterDelivered = waterStored = waterUntreated = 0.f;
        for (const auto& G : powerGrids) {
            powerSupply += std::min(G.powerSupply, G.transformerCap);
            powerDemand += G.powerDemand;
            powerDelivered += G.powerUsed;
        }
        sewageDemand = sewageHandled = sewageCapacity = 0.f;
        for (const auto& G : waterGrids) {
            sewageDemand += G.sewageDemand;
            sewageHandled += G.sewageUsed;
            sewageCapacity += G.sewageCap;
            waterSupply += G.clean + G.raw;
            waterDemand += G.waterDemand;
            waterDelivered += G.cleanUsed - G.towerFill + G.towerUsed + G.untreatedUsed;
            waterUntreated += G.untreatedUsed;
            waterStored += G.towerStored - G.towerUsed;
        }
        // Buildings cut off from a grid still want utilities
        forEachRoot([&](Zone* z, int x, int y) {
            if (!consumes(z)) return;
            if (powerGridOf[x][y] < 0 || !z->isActive) powerDemand += demandOf(z, false, pop);
            if (waterGridOf[x][y] < 0 || !z->isActive) { waterDemand += demandOf(z, true, pop); sewageDemand += demandOf(z, true, pop); }
        });
    }

    // Towers: pay out what they covered today, then soak up spare clean water
    void fillTowers(std::vector<UtilityZone*>& towers) {
        for (size_t g = 0; g < waterGrids.size(); g++) {
            UtilityGrid& G = waterGrids[g];
            if (G.towerCap <= 0.f) continue;
            float level = G.towerStored - G.towerUsed;
            float spare = std::max(0.f, G.cleanPool() - G.cleanUsed);
            float fill = std::min(spare, G.towerCap - level);
            G.cleanUsed += fill;   // pumped into the tower: drains the source like any delivery
            G.towerFill = fill;
            level += fill;
            G.towerStored = level;
            G.towerUsed = 0.f;
            for (UtilityZone* t : towers)
                if (waterGridOf[t->posX][t->posY] == static_cast<int>(g)) t->stored = level * t->output / G.towerCap;
        }
    }

    // Each source drains GROUNDWATER_DRAIN points per unit it actually
    // delivered, spread over the land tiles in its radius
    void drainGroundwater(const std::vector<UtilityZone*>& sources) {
        for (UtilityZone* u : sources) {
            const UtilityGrid& G = waterGrids[static_cast<size_t>(waterGridOf[u->posX][u->posY])];
            float wellUsed = std::min(G.clean, G.cleanUsed);
            float treatedUsed = G.cleanUsed - wellUsed;
            float util = u->kind == UTIL_WELL ? (G.clean > 0.f ? wellUsed / G.clean : 0.f)
                                              : (G.raw > 0.f ? (treatedUsed + G.untreatedUsed) / G.raw : 0.f);
            float delivered = u->output * std::min(1.f, util);
            const int r = u->drawRadius();
            std::vector<sf::Vector2i> land;
            for (int dx = -r; dx <= r; dx++)
                for (int dy = -r; dy <= r; dy++) {
                    int tx = u->posX + dx, ty = u->posY + dy;
                    if (dx * dx + dy * dy <= r * r && inWorldBounds(tx, ty) && world[tx][ty].terrain != Terrain::Water)
                        land.push_back({tx, ty});
                }
            if (land.empty()) continue;
            float per = delivered * GROUNDWATER_DRAIN / static_cast<float>(land.size());
            for (const auto& t : land) gwCarry[t.x][t.y] -= per;
        }
    }

    // Recharge towards the natural level: faster in spring and next to
    // lakes / rivers, slower in summer. Then apply whole points.
    void rechargeGroundwater() {
        Season s = gameClock.getSeason();
        float rate = GROUNDWATER_RECHARGE * (s == Season::Spring ? 2.f : (s == Season::Summer ? 0.5f : 1.f));
        for (int i = 0; i < tileMapW; i++)
            for (int j = 0; j < tileMapH; j++) {
                TileData& t = world[i][j];
                if (t.terrain == Terrain::Water) continue;
                if (t.groundwater < t.groundwaterMax) {
                    bool nearWater = false;
                    for (int dx = -3; dx <= 3 && !nearWater; dx++)
                        for (int dy = -3; dy <= 3; dy++)
                            if (inWorldBounds(i + dx, j + dy) && world[i + dx][j + dy].terrain == Terrain::Water) { nearWater = true; break; }
                    gwCarry[i][j] += rate * (nearWater ? 2.f : 1.f);
                }
                float& c = gwCarry[i][j];
                while (c >= 1.f && t.groundwater < t.groundwaterMax) { t.groundwater++; c -= 1.f; }
                while (c <= -1.f && t.groundwater > 0) { t.groundwater--; c += 1.f; }
                c = std::max(-1.f, std::min(1.f, c));
                if (t.groundwater >= t.groundwaterMax && c > 0.f) c = 0.f;
            }
    }
};

inline UtilitySystem utilities;
