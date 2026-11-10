#pragma once
#include <vector>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "../../src/_global.hpp"
#include "./EconomyData.hpp"
#include "../world/WorldData.hpp"
#include "../world/RoadNetwork.hpp"
#include "../entities/_Zone.hpp"
#include "../entities/ServiceZone.hpp"

// -------------------------------------------------------------
// City services: coverage, garbage, fires, mines, buses, land value
// -------------------------------------------------------------
// update()  (after any build and every day) rebuilds the coverage maps
//           (police / fire / clinic / bus: 100 inside the radius, fading to
//           0 over its outer third), garbage collection and the mine count.
// runDay()  (once a game day) ignites, spreads, fights and finishes fires
//           and depletes the deposits under working mines. Fires only start
//           in cities of FIRE_GRACE_POP or more, using a position / day hash
//           (no std::rand, so tests stay deterministic).
// computeLandValue() writes world[][].landValue from the happiness coverage
//           maps, the service maps, nearby water and traffic.
//
// Garbage: every R / C / I building makes GARBAGE_PER_CAPITA per occupant a
// day, and it has to go somewhere. Trucks take it by road to the facilities
// on the building's road network: recycling centres first (clean), then
// incinerators (burn it, polluting, and make power), then landfills (store
// it until LANDFILL_SPACE is used up; a full landfill takes nothing more).
// Buildings are served in scan order. update() only plans the day;
// runDay() actually fills the landfills. Under UTILITY_GRACE_POP citizens
// nobody minds the garbage yet.

class ServiceSystem {
public:
    uint8_t policeMap[tileMapW][tileMapH];
    uint8_t fireMap[tileMapW][tileMapH];
    uint8_t clinicMap[tileMapW][tileMapH];
    uint8_t busMap[tileMapW][tileMapH];
    uint8_t garbageServed[tileMapW][tileMapH];   // at building roots: 1 = collected

    float garbageMade = 0.f, garbageCollected = 0.f, disposalCapacity = 0.f;
    float recycled = 0.f, burnt = 0.f, dumped = 0.f;     // planned for today
    float landfillSpaceLeft = 0.f;
    int landfills = 0, fullLandfills = 0;
    int unservedGarbage = 0;
    int activeMines = 0;
    int burning = 0;
    bool grace = true;

    struct FireEvent { int x, y; int kind; std::string name; };   // kind: 0 started, 1 put out, 2 destroyed, 3 landfill full
    std::vector<FireEvent> events;   // from the last runDay

    ServiceSystem() { reset(); }

    void reset() {
        std::memset(policeMap, 0, sizeof(policeMap));
        std::memset(fireMap, 0, sizeof(fireMap));
        std::memset(clinicMap, 0, sizeof(clinicMap));
        std::memset(busMap, 0, sizeof(busMap));
        std::memset(garbageServed, 0, sizeof(garbageServed));
        garbageMade = garbageCollected = disposalCapacity = 0.f;
        recycled = burnt = dumped = landfillSpaceLeft = 0.f;
        landfills = fullLandfills = 0;
        unservedGarbage = activeMines = burning = 0;
        grace = true;
        events.clear();
    }

    bool garbageOk(int x, int y) const { return grace || !inWorldBounds(x, y) || garbageServed[x][y] != 0; }

    static bool makesGarbage(Zone* z) {
        return z != NULL && !z->isSubTile && z->sector() >= 0 && z->getName() != "FarmZone" && z->getName() != "MineZone";
    }

    void update() {
        std::memset(policeMap, 0, sizeof(policeMap));
        std::memset(fireMap, 0, sizeof(fireMap));
        std::memset(clinicMap, 0, sizeof(clinicMap));
        std::memset(busMap, 0, sizeof(busMap));
        std::memset(garbageServed, 0, sizeof(garbageServed));
        activeMines = 0;
        burning = 0;

        // Garbage facilities per road component
        std::vector<std::vector<ServiceZone*>> pools(roadNet.outside.size());
        landfills = fullLandfills = 0;
        landfillSpaceLeft = 0.f;
        int pop = 0;
        forEachRoot([&](Zone* z, int x, int y) {
            if (makesGarbage(z)) pop += z->population;
            if (z->fireDays > 0) burning++;
            ServiceZone* s = dynamic_cast<ServiceZone*>(z);
            if (s == NULL || !s->isActive) return;
            switch (s->kind) {
                case SERVICE_POLICE:    stamp(policeMap, x, y, SERVICE_RADIUS[SERVICE_POLICE]); break;
                case SERVICE_FIRE:      stamp(fireMap, x, y, SERVICE_RADIUS[SERVICE_FIRE]); break;
                case SERVICE_CLINIC:    stamp(clinicMap, x, y, SERVICE_RADIUS[SERVICE_CLINIC]); break;
                case SERVICE_BUS_DEPOT: stamp(busMap, x, y, SERVICE_RADIUS[SERVICE_BUS_DEPOT]); break;
                case SERVICE_MINE:      if (s->population > 0) activeMines++; break;
                case SERVICE_LANDFILL:
                case SERVICE_INCINERATOR:
                case SERVICE_RECYCLING: {
                    s->handled = 0.f;
                    if (s->kind == SERVICE_LANDFILL) {
                        landfills++;
                        landfillSpaceLeft += std::max(0.f, LANDFILL_SPACE - s->stored);
                        if (s->stored >= LANDFILL_SPACE) fullLandfills++;
                    }
                    int c = componentOf(x, y);
                    if (c >= 0) pools[static_cast<size_t>(c)].push_back(s);
                    break;
                }
            }
        });
        grace = pop < UTILITY_GRACE_POP;

        // Recycling first, then burning, then dumping
        for (auto& pool : pools)
            std::stable_sort(pool.begin(), pool.end(), [](const ServiceZone* a, const ServiceZone* b) {
                return priority(a->kind) < priority(b->kind);
            });
        disposalCapacity = 0.f;
        for (const auto& pool : pools)
            for (const ServiceZone* f : pool) disposalCapacity += dailyRoom(f);

        garbageMade = garbageCollected = recycled = burnt = dumped = 0.f;
        unservedGarbage = 0;
        forEachRoot([&](Zone* z, int x, int y) {
            if (!makesGarbage(z)) return;
            float g = static_cast<float>(std::max(z->population, z->capacity / 10)) * GARBAGE_PER_CAPITA;
            garbageMade += g;
            int c = z->isActive ? componentOf(x, y) : -1;
            float room = 0.f;
            if (c >= 0)
                for (const ServiceZone* f : pools[static_cast<size_t>(c)]) room += dailyRoom(f) - f->handled;
            // Served when at least half of it is taken; the rest stays uncollected
            if (c < 0 || room < 0.5f * g) { unservedGarbage++; return; }
            float left = std::min(g, room);
            for (ServiceZone* f : pools[static_cast<size_t>(c)]) {
                float take = std::min(left, dailyRoom(f) - f->handled);
                if (take <= 0.f) continue;
                f->handled += take;
                left -= take;
                if (f->kind == SERVICE_RECYCLING) recycled += take;
                else if (f->kind == SERVICE_INCINERATOR) burnt += take;
                else dumped += take;
                if (left <= 0.f) break;
            }
            garbageCollected += std::min(g, room);
            garbageServed[x][y] = 1;
        });
    }

    // What a garbage facility can take today
    static float dailyRoom(const ServiceZone* f) {
        if (!f->isActive) return 0.f;
        switch (f->kind) {
            case SERVICE_RECYCLING:   return RECYCLING_RATE;
            case SERVICE_INCINERATOR: return INCINERATOR_RATE;
            case SERVICE_LANDFILL:    return std::max(0.f, std::min(LANDFILL_INTAKE, LANDFILL_SPACE - f->stored));
            default:                  return 0.f;
        }
    }

    // Fires and mines, once a game day. Returns true when buildings were lost.
    bool runDay(long day, int cityPop) {
        events.clear();
        bool destroyed = false;

        // Today's garbage goes into the landfills (planned by update())
        forEachRoot([&](Zone* z, int x, int y) {
            ServiceZone* s = dynamic_cast<ServiceZone*>(z);
            if (s == NULL || s->kind != SERVICE_LANDFILL || s->handled <= 0.f) return;
            const bool wasFull = s->stored >= LANDFILL_SPACE;
            s->stored = std::min(LANDFILL_SPACE, s->stored + s->handled);
            if (!wasFull && s->stored >= LANDFILL_SPACE) events.push_back({x, y, 3, s->getName()});
        });

        // Mines use up their deposit
        forEachRoot([&](Zone* z, int x, int y) {
            ServiceZone* s = dynamic_cast<ServiceZone*>(z);
            if (s == NULL || s->kind != SERVICE_MINE || s->population <= 0) return;
            s->depleteCarry += static_cast<float>(s->population) * MINE_DEPLETION;
            while (s->depleteCarry >= 1.f) {
                s->depleteCarry -= 1.f;
                for (const auto& o : FOOTPRINT_2X2) {
                    TileData& t = world[x + o[0]][y + o[1]];
                    if (t.deposit != Deposit::None && t.depositAmount > 0) { t.depositAmount--; break; }
                }
            }
        });

        if (cityPop < FIRE_GRACE_POP) {
            forEachRoot([&](Zone* z, int, int) { z->fireDays = 0; });
            return false;
        }

        // 1. Burning buildings: fire crews put them out, or they spread and burn down
        std::vector<sf::Vector2i> toDestroy, toIgnite;
        forEachRoot([&](Zone* z, int x, int y) {
            if (z->fireDays <= 0) return;
            if (fireMap[x][y] >= 50) {
                z->fireDays = 0;
                events.push_back({x, y, 1, z->getName()});
                return;
            }
            z->fireDays++;
            // Spread to buildings within 2 tiles (1 in FIRE_SPREAD a day each), not
            // to ones a fire station covers
            forEachRoot([&](Zone* n, int nx, int ny) {
                if (n == z || n->fireDays > 0 || !flammable(n) || fireMap[nx][ny] >= 50) return;
                if (std::abs(nx - x) <= 2 && std::abs(ny - y) <= 2 && hash(nx, ny, day, 7) % FIRE_SPREAD == 0) toIgnite.push_back({nx, ny});
            });
            if (z->fireDays > FIRE_BURN_DAYS) toDestroy.push_back({x, y});
        });
        for (const auto& p : toIgnite) {
            Zone* n = dynamic_cast<Zone*>(entities[p.x][p.y]);
            if (n != NULL && n->fireDays == 0) { n->fireDays = 1; events.push_back({p.x, p.y, 0, n->getName()}); }
        }
        // 2. New fires: rare, and a fire station nearby stops them before they take hold
        forEachRoot([&](Zone* z, int x, int y) {
            if (z->fireDays > 0 || !flammable(z)) return;
            if (hash(x, y, day, 1) % FIRE_CHANCE != 0) return;
            if (fireMap[x][y] >= 50) return;
            z->fireDays = 1;
            events.push_back({x, y, 0, z->getName()});
        });
        // 3. Burnt down: the lot is cleared (its zone stays and can regrow)
        for (const auto& p : toDestroy) {
            Zone* z = dynamic_cast<Zone*>(entities[p.x][p.y]);
            if (z == NULL) continue;
            events.push_back({p.x, p.y, 2, z->getName()});
            for (const auto& o : FOOTPRINT_2X2) {
                int tx = p.x + o[0], ty = p.y + o[1];
                delete entities[tx][ty];
                entities[tx][ty] = NULL;
            }
            destroyed = true;
        }
        update();
        return destroyed;
    }

    // Land value from the surroundings (0-100). Maps are the happiness
    // coverage maps; congestion is the traffic system's per-tile map.
    void computeLandValue(const int park[tileMapW][tileMapH], const int edu[tileMapW][tileMapH],
                          const int commerce[tileMapW][tileMapH], const int pollution[tileMapW][tileMapH],
                          const float congestion[tileMapW][tileMapH]) {
        static uint8_t nearWater[tileMapW][tileMapH];
        for (int i = 0; i < tileMapW; i++)
            for (int j = 0; j < tileMapH; j++) {
                bool w = false;
                for (int dx = -3; dx <= 3 && !w; dx++)
                    for (int dy = -3; dy <= 3; dy++)
                        if (inWorldBounds(i + dx, j + dy) && world[i + dx][j + dy].terrain == Terrain::Water) { w = true; break; }
                nearWater[i][j] = w ? 1 : 0;
            }
        for (int i = 0; i < tileMapW; i++)
            for (int j = 0; j < tileMapH; j++) {
                if (world[i][j].terrain == Terrain::Water) { world[i][j].landValue = 0; continue; }
                float v = 30.f + 0.2f * park[i][j] + 0.15f * edu[i][j] + 0.1f * commerce[i][j] +
                          0.1f * policeMap[i][j] + 0.1f * clinicMap[i][j] + (nearWater[i][j] ? 12.f : 0.f) -
                          0.5f * pollution[i][j];
                // Jammed roads next door
                float jam = 0.f;
                for (const auto& d : ROAD_DIR) {
                    int nx = i + d[0], ny = j + d[1];
                    if (inWorldBounds(nx, ny)) jam = std::max(jam, congestion[nx][ny]);
                }
                if (jam > 1.f) v -= std::min(15.f, 15.f * (jam - 1.f));
                world[i][j].landValue = static_cast<uint8_t>(std::max(0.f, std::min(100.f, v)));
            }
    }

private:
    template <typename Fn>
    static void forEachRoot(Fn fn) {
        for (int i = 0; i < tileMapW; i++)
            for (int j = 0; j < tileMapH; j++) {
                if (entities[i][j] == NULL) continue;
                Zone* z = dynamic_cast<Zone*>(entities[i][j]);
                if (z != NULL && z->isMultiTile && !z->isSubTile) fn(z, i, j);
            }
    }

    static int priority(int kind) { return kind == SERVICE_RECYCLING ? 0 : (kind == SERVICE_INCINERATOR ? 1 : 2); }

    static bool flammable(Zone* z) {
        const std::string n = z->getName();
        return n != "ParkZone" && n != "FireZone" && n != "FarmZone" && n != "UtilityZone";
    }

    static unsigned hash(int x, int y, long day, unsigned salt) {
        unsigned h = static_cast<unsigned>(x) * 73856093u ^ static_cast<unsigned>(y) * 19349663u ^
                     static_cast<unsigned>(day + 1) * 83492791u ^ salt * 2654435761u;
        h ^= h >> 13;
        h *= 0x5bd1e995u;
        return h ^ (h >> 15);
    }

    // Road component the building's border touches (garbage trucks drive on roads)
    static int componentOf(int x, int y) {
        for (const auto& o : BORDER_2X2) {
            int bx = x + o[0], by = y + o[1];
            if (roadTierOfName(tileKind(bx, by)) >= 0) {
                int c = roadNet.componentAt(bx, by);
                if (c >= 0) return c;
            }
        }
        return -1;
    }

    // 100 inside two thirds of the radius, fading to 0 at the edge (keeps the max)
    static void stamp(uint8_t map[tileMapW][tileMapH], int cx, int cy, int r) {
        for (int dx = -r; dx <= r; dx++)
            for (int dy = -r; dy <= r; dy++) {
                int x = cx + dx, y = cy + dy;
                if (!inWorldBounds(x, y)) continue;
                float d = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                if (d > static_cast<float>(r)) continue;
                float inner = static_cast<float>(r) * 2.f / 3.f;
                float v = d <= inner ? 100.f : 100.f * (static_cast<float>(r) - d) / (static_cast<float>(r) - inner);
                map[x][y] = std::max(map[x][y], static_cast<uint8_t>(v));
            }
    }
};

inline ServiceSystem services;
