#pragma once
#include <vector>
#include <queue>
#include <cstring>
#include <algorithm>
#include "../../src/_global.hpp"
#include "./EconomyData.hpp"
#include "../world/RoadNetwork.hpp"
#include "../entities/_Zone.hpp"
#include "./ServiceSystem.hpp"

// -------------------------------------------------------------
// Traffic: daily commute model
// -------------------------------------------------------------
// Once per game day every occupied home sends 60% of its residents to
// work. Each home runs one Dijkstra search over its road component
// (faster tiers are cheaper to drive) and fills the nearest jobs first:
// a job building takes as many commuters as it has workers. Commuters
// left over drive to the nearest map-edge road (jobs outside the city)
// when the network has an outside connection.
//
// Every trip adds 1 to the load of each road tile on its route.
// congestion = load / effective capacity, where a 3- or 4-way junction
// carries less (x0.7) unless it is a roundabout (x1.5).
//
// Outputs feed happiness (homes whose route is over capacity lose up to
// 20 points), commercial demand, the Traffic map view and the cars.

const float COMMUTER_SHARE = 0.6f;
const float JUNCTION_CAPACITY = 0.7f;
const float ROUNDABOUT_CAPACITY = 1.5f;

class TrafficSystem {
public:
    float load[tileMapW][tileMapH];
    float congestion[tileMapW][tileMapH];
    float homeCongestion[tileMapW][tileMapH]; // average route congestion, at each home's root
    float cityCongestion = 0.f;               // trip-weighted average over the city
    int totalTrips = 0;
    int outsideTrips = 0;

    TrafficSystem() { reset(); }

    void reset() {
        std::memset(load, 0, sizeof(load));
        std::memset(congestion, 0, sizeof(congestion));
        std::memset(homeCongestion, 0, sizeof(homeCongestion));
        cityCongestion = 0.f;
        totalTrips = outsideTrips = 0;
    }

    static int linkCount(const RoadTile* r) {
        int n = 0;
        for (int d = 0; d < 4; d++) n += (r->mask >> d) & 1;
        return n;
    }

    static float effectiveCapacity(const RoadTile* r) {
        float cap = static_cast<float>(r->capacity());
        if (linkCount(r) >= 3) cap *= r->roundabout ? ROUNDABOUT_CAPACITY : JUNCTION_CAPACITY;
        return cap;
    }

    // Score shown on homes and in townStats: 100 free-flowing, 0 gridlocked
    static int scoreFor(float cong) {
        return std::max(0, std::min(100, static_cast<int>(100.f - std::max(0.f, cong - 0.5f) * 100.f)));
    }

    // Happiness lost by a home whose route runs over capacity
    static int happinessPenalty(float cong) {
        return static_cast<int>(std::min(20.f, 20.f * std::max(0.f, cong - 1.f)));
    }

    void update() {
        reset();
        roadNet.rebuild();

        const int N = tileMapW * tileMapH;
        auto idx = [](int x, int y) { return x * tileMapH + y; };

        // Jobs reachable from each road tile (job buildings on its border)
        struct Job { int slots; };
        std::vector<Job> jobs;
        std::vector<std::vector<int>> jobsAt(static_cast<size_t>(N));
        struct Home { int x, y, commuters; };
        std::vector<Home> homes;

        for (int i = 0; i < tileMapW; i++) {
            for (int j = 0; j < tileMapH; j++) {
                if (entities[i][j] == NULL) continue;
                Zone* z = dynamic_cast<Zone*>(entities[i][j]);
                if (z == NULL || z->isSubTile || !z->isActive || z->population <= 0) continue;
                int s = z->sector();
                if (s == SECTOR_R) {
                    // Near a bus depot part of the commuters take the bus instead
                    float share = COMMUTER_SHARE * (services.busMap[i][j] >= 50 ? 1.f - BUS_RIDER_SHARE : 1.f);
                    int c = static_cast<int>(static_cast<float>(z->population) * share + 0.5f);
                    if (c > 0) homes.push_back({i, j, c});
                } else if (s >= 0) {
                    int id = static_cast<int>(jobs.size());
                    jobs.push_back({z->population});
                    for (const auto& o : BORDER_2X2) {
                        int x = i + o[0], y = j + o[1];
                        if (roadTierOfName(tileKind(x, y)) >= 0) jobsAt[static_cast<size_t>(idx(x, y))].push_back(id);
                    }
                }
            }
        }

        // Dijkstra buffers, reset only where touched
        std::vector<float> dist(static_cast<size_t>(N), 1e30f);
        std::vector<int> parent(static_cast<size_t>(N), -1);
        std::vector<int> touched;
        touched.reserve(static_cast<size_t>(N));
        typedef std::pair<float, int> QItem;

        struct Trip { int home, target, count; std::vector<int> path; };
        std::vector<Trip> trips;

        for (size_t h = 0; h < homes.size(); h++) {
            const Home& home = homes[h];
            for (int k : touched) { dist[static_cast<size_t>(k)] = 1e30f; parent[static_cast<size_t>(k)] = -1; }
            touched.clear();

            std::priority_queue<QItem, std::vector<QItem>, std::greater<QItem>> pq;
            for (const auto& o : BORDER_2X2) {
                int x = home.x + o[0], y = home.y + o[1];
                if (roadTierOfName(tileKind(x, y)) < 0) continue;
                int k = idx(x, y);
                if (dist[static_cast<size_t>(k)] <= 0.f) continue;
                dist[static_cast<size_t>(k)] = 0.f;
                touched.push_back(k);
                pq.push({0.f, k});
            }

            int left = home.commuters;
            int borderTile = -1;
            std::vector<std::pair<int, int>> targets; // (road tile, commuters)
            while (!pq.empty() && (left > 0 || borderTile < 0)) {
                QItem top = pq.top();
                pq.pop();
                int k = top.second;
                if (top.first > dist[static_cast<size_t>(k)]) continue;
                int x = k / tileMapH, y = k % tileMapH;

                if (left > 0) {
                    for (int id : jobsAt[static_cast<size_t>(k)]) {
                        int take = std::min(left, jobs[static_cast<size_t>(id)].slots);
                        if (take <= 0) continue;
                        jobs[static_cast<size_t>(id)].slots -= take;
                        left -= take;
                        targets.push_back({k, take});
                        if (left == 0) break;
                    }
                }
                if (borderTile < 0 && onMapBorder(x, y)) borderTile = k;

                int m = RoadTile::linkMask(x, y);
                for (int d = 0; d < 4; d++) {
                    if (!(m & (1 << d))) continue;
                    int nx = x + ROAD_DIR[d][0], ny = y + ROAD_DIR[d][1];
                    RoadTile* nr = RoadTile::at(nx, ny);
                    int nk = idx(nx, ny);
                    float nd = top.first + 1.f / ROAD_SPEED[nr->tier];
                    if (nd < dist[static_cast<size_t>(nk)]) {
                        if (dist[static_cast<size_t>(nk)] >= 1e29f) touched.push_back(nk);
                        dist[static_cast<size_t>(nk)] = nd;
                        parent[static_cast<size_t>(nk)] = k;
                        pq.push({nd, nk});
                    }
                }
            }
            if (left > 0 && borderTile >= 0) {
                targets.push_back({borderTile, left});
                outsideTrips += left;
            }

            for (const auto& t : targets) {
                Trip trip{static_cast<int>(h), t.first, t.second, {}};
                for (int k = t.first; k >= 0; k = parent[static_cast<size_t>(k)]) {
                    trip.path.push_back(k);
                    load[k / tileMapH][k % tileMapH] += static_cast<float>(t.second);
                }
                totalTrips += t.second;
                trips.push_back(std::move(trip));
            }
        }

        for (int i = 0; i < tileMapW; i++) {
            for (int j = 0; j < tileMapH; j++) {
                RoadTile* r = RoadTile::at(i, j);
                if (r != NULL) congestion[i][j] = load[i][j] / effectiveCapacity(r);
            }
        }

        // Route congestion per home (average over its tiles, weighted by trips)
        std::vector<float> homeSum(homes.size(), 0.f), homeW(homes.size(), 0.f);
        double citySum = 0.0;
        for (const Trip& t : trips) {
            float c = 0.f;
            for (int k : t.path) c += congestion[k / tileMapH][k % tileMapH];
            c /= static_cast<float>(std::max<size_t>(1, t.path.size()));
            homeSum[static_cast<size_t>(t.home)] += c * static_cast<float>(t.count);
            homeW[static_cast<size_t>(t.home)] += static_cast<float>(t.count);
            citySum += static_cast<double>(c) * t.count;
        }
        for (size_t h = 0; h < homes.size(); h++) {
            float c = homeW[h] > 0.f ? homeSum[h] / homeW[h] : 0.f;
            homeCongestion[homes[h].x][homes[h].y] = c;
        }
        cityCongestion = totalTrips > 0 ? static_cast<float>(citySum / totalTrips) : 0.f;
    }
};

inline TrafficSystem traffic;
