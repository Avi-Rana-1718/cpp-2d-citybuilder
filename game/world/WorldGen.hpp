#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <queue>
#include <random>
#include <vector>
#include "./WorldData.hpp"

// -------------------------------------------------------------
// Procedural world generator
// -------------------------------------------------------------
// Pure data: fills world[][] from a seed and never touches entities[][]
// or SFML, so it is deterministic and testable without textures.
// Uses its own std::mt19937 (never std::rand) so zone code cannot
// disturb the output for a given seed.

namespace worldgen {

const int W = tileMapW;
const int H = tileMapH;

inline uint32_t hash2D(int x, int y, uint32_t seed) {
    uint32_t h = static_cast<uint32_t>(x) * 374761393u
               + static_cast<uint32_t>(y) * 668265263u
               + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return h;
}

// Lattice value in [0, 1]
inline float lattice(int x, int y, uint32_t seed) {
    return static_cast<float>(hash2D(x, y, seed) & 0xFFFFFFu) / 16777215.f;
}

inline float smooth(float t) {
    return t * t * (3.f - 2.f * t);
}

// Smoothly interpolated value noise in [0, 1]
inline float valueNoise(float x, float y, uint32_t seed) {
    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    float tx = smooth(x - static_cast<float>(x0));
    float ty = smooth(y - static_cast<float>(y0));
    float a = lattice(x0, y0, seed);
    float b = lattice(x0 + 1, y0, seed);
    float c = lattice(x0, y0 + 1, seed);
    float d = lattice(x0 + 1, y0 + 1, seed);
    float top = a + (b - a) * tx;
    float bot = c + (d - c) * tx;
    return top + (bot - top) * ty;
}

// Fractal Brownian motion, normalised to [0, 1]
inline float fbm(float x, float y, int octaves, float baseFreq, uint32_t seed) {
    float sum = 0.f;
    float amp = 1.f;
    float norm = 0.f;
    float freq = baseFreq;
    for (int o = 0; o < octaves; o++) {
        sum += amp * valueNoise(x * freq, y * freq, seed + static_cast<uint32_t>(o) * 1013u);
        norm += amp;
        amp *= 0.5f;
        freq *= 2.f;
    }
    return sum / norm;
}

// Value at the given percentile (0..1) of a list
inline float percentile(std::vector<float> values, float p) {
    if (values.empty()) return 0.f;
    size_t idx = static_cast<size_t>(p * static_cast<float>(values.size() - 1));
    std::nth_element(values.begin(), values.begin() + idx, values.end());
    return values[idx];
}

inline uint8_t clampByte(float v) {
    if (v < 0.f) return 0;
    if (v > 100.f) return 100;
    return static_cast<uint8_t>(v);
}

const int DX4[4] = {1, -1, 0, 0};
const int DY4[4] = {0, 0, 1, -1};

class Generator {
    std::mt19937 rng;
    uint32_t seed;
    float height[W][H];
    float moisture[W][H];
    bool ocean[W][H];
    int waterDist[W][H];

    int randInt(int lo, int hi) { // inclusive, portable across standard libraries
        return lo + static_cast<int>(rng() % static_cast<uint32_t>(hi - lo + 1));
    }
    float randFloat() {
        return static_cast<float>(rng() & 0xFFFFFFu) / 16777215.f;
    }

public:
    explicit Generator(uint32_t s) : rng(s), seed(s) {}

    void run() {
        resetWorldData();
        buildHeightAndMoisture();
        placeWater();
        removeTinyLakes();
        carveLakes();
        markOcean();
        carveRivers();
        placeBeaches();
        placeRock();
        placeForests();
        carveForestPaths();
        chooseSpawn();
        computeWaterDistance();
        computeFertility();
        computeGroundwater();
        placeDeposits();
    }

private:
    // 1. Height map with a coastline falloff on one or two map edges
    void buildHeightAndMoisture() {
        bool side[4] = {false, false, false, false}; // 0:x=0 1:x=max 2:y=0 3:y=max
        int coastCount = randInt(1, 2);
        int first = randInt(0, 3);
        side[first] = true;
        if (coastCount == 2) {
            int second = (first + randInt(1, 3)) % 4;
            side[second] = true;
        }
        float coastWidth = 14.f + randFloat() * 8.f;

        for (int i = 0; i < W; i++) {
            for (int j = 0; j < H; j++) {
                float h = fbm(static_cast<float>(i), static_cast<float>(j), 4, 1.f / 18.f, seed);
                float wobble = (fbm(static_cast<float>(i), static_cast<float>(j), 2, 1.f / 10.f, seed + 7u) - 0.5f) * 8.f;
                float dist[4] = {
                    static_cast<float>(i), static_cast<float>(W - 1 - i),
                    static_cast<float>(j), static_cast<float>(H - 1 - j)
                };
                for (int s = 0; s < 4; s++) {
                    if (!side[s]) continue;
                    float t = 1.f - (dist[s] + wobble) / coastWidth;
                    if (t > 0.f) h -= t * 0.55f;
                }
                height[i][j] = h;
                moisture[i][j] = fbm(static_cast<float>(i), static_cast<float>(j), 3, 1.f / 12.f, seed + 101u);
            }
        }
    }

    // 2. Ocean + lakes: lowest ~18-26% of the map becomes water
    void placeWater() {
        std::vector<float> hs;
        hs.reserve(W * H);
        for (int i = 0; i < W; i++)
            for (int j = 0; j < H; j++) hs.push_back(height[i][j]);
        float seaLevel = percentile(hs, 0.18f + randFloat() * 0.08f);

        for (int i = 0; i < W; i++)
            for (int j = 0; j < H; j++)
                if (height[i][j] <= seaLevel) world[i][j].terrain = Terrain::Water;
    }

    // Flood-fill helper: collects 4-connected tiles matching pred from (sx, sy)
    template <typename Pred>
    std::vector<std::pair<int, int>> floodFill(int sx, int sy, std::vector<bool>& seen, Pred pred) {
        std::vector<std::pair<int, int>> out;
        std::queue<std::pair<int, int>> q;
        q.push({sx, sy});
        seen[sx * H + sy] = true;
        while (!q.empty()) {
            auto [x, y] = q.front();
            q.pop();
            out.push_back({x, y});
            for (int d = 0; d < 4; d++) {
                int nx = x + DX4[d];
                int ny = y + DY4[d];
                if (!inWorldBounds(nx, ny) || seen[nx * H + ny] || !pred(nx, ny)) continue;
                seen[nx * H + ny] = true;
                q.push({nx, ny});
            }
        }
        return out;
    }

    void removeTinyLakes() {
        std::vector<bool> seen(W * H, false);
        auto isWater = [&](int x, int y) { return world[x][y].terrain == Terrain::Water; };
        for (int i = 0; i < W; i++) {
            for (int j = 0; j < H; j++) {
                if (seen[i * H + j] || !isWater(i, j)) continue;
                auto body = floodFill(i, j, seen, isWater);
                if (body.size() < 4) {
                    for (auto& p : body) world[p.first][p.second].terrain = Terrain::Grass;
                }
            }
        }
    }

    // 2b. Inland lakes: 1-2 irregular blobs on high-and-dry land away from the coast
    void carveLakes() {
        computeWaterDistance();
        std::vector<std::pair<int, int>> candidates;
        for (int i = 8; i < W - 8; i++)
            for (int j = 8; j < H - 8; j++)
                if (world[i][j].terrain != Terrain::Water && waterDist[i][j] >= 10)
                    candidates.push_back({i, j});

        int lakeCount = randInt(1, 2);
        for (int l = 0; l < lakeCount && !candidates.empty(); l++) {
            auto [cx, cy] = candidates[static_cast<size_t>(randInt(0, static_cast<int>(candidates.size()) - 1))];
            float radius = 2.f + randFloat() * 2.f;
            uint32_t lakeSeed = seed + 606u + static_cast<uint32_t>(l) * 31u;
            for (int i = cx - 6; i <= cx + 6; i++) {
                for (int j = cy - 6; j <= cy + 6; j++) {
                    if (!inWorldBounds(i, j)) continue;
                    float d = std::sqrt(static_cast<float>((i - cx) * (i - cx) + (j - cy) * (j - cy)));
                    float edge = radius + (fbm(static_cast<float>(i), static_cast<float>(j), 2, 1.f / 4.f, lakeSeed) - 0.5f) * 3.f;
                    if (d <= edge) world[i][j].terrain = Terrain::Water;
                }
            }
            // Keep later lakes away from this one
            candidates.erase(std::remove_if(candidates.begin(), candidates.end(), [&](const std::pair<int, int>& c) {
                return std::abs(c.first - cx) + std::abs(c.second - cy) < 16;
            }), candidates.end());
        }
    }

    // Ocean = water bodies touching the map edge (lakes are the rest)
    void markOcean() {
        for (int i = 0; i < W; i++)
            for (int j = 0; j < H; j++) ocean[i][j] = false;

        std::vector<bool> seen(W * H, false);
        auto isWater = [&](int x, int y) { return world[x][y].terrain == Terrain::Water; };
        for (int i = 0; i < W; i++) {
            for (int j = 0; j < H; j++) {
                bool edge = (i == 0 || j == 0 || i == W - 1 || j == H - 1);
                if (!edge || seen[i * H + j] || !isWater(i, j)) continue;
                auto body = floodFill(i, j, seen, isWater);
                for (auto& p : body) ocean[p.first][p.second] = true;
            }
        }
    }

    // 3. Rivers: walk downhill from high ground until reaching water
    void carveRivers() {
        std::vector<std::pair<int, int>> sources;
        std::vector<float> landHeights;
        for (int i = 0; i < W; i++)
            for (int j = 0; j < H; j++)
                if (world[i][j].terrain != Terrain::Water) landHeights.push_back(height[i][j]);
        float highLevel = percentile(landHeights, 0.75f);

        for (int i = 3; i < W - 3; i++)
            for (int j = 3; j < H - 3; j++)
                if (world[i][j].terrain != Terrain::Water && height[i][j] >= highLevel)
                    sources.push_back({i, j});
        if (sources.empty()) return;

        int riverCount = randInt(1, 3);
        for (int r = 0; r < riverCount; r++) {
            auto [x, y] = sources[static_cast<size_t>(randInt(0, static_cast<int>(sources.size()) - 1))];
            std::vector<std::pair<int, int>> path;
            std::vector<bool> visited(W * H, false);
            bool reachedWater = false;

            for (int step = 0; step < 140; step++) {
                path.push_back({x, y});
                visited[x * H + y] = true;

                int bestX = -1, bestY = -1;
                float bestH = 1e9f;
                for (int d = 0; d < 4; d++) {
                    int nx = x + DX4[d];
                    int ny = y + DY4[d];
                    if (!inWorldBounds(nx, ny) || visited[nx * H + ny]) continue;
                    if (world[nx][ny].terrain == Terrain::Water) {
                        reachedWater = true;
                        break;
                    }
                    float h = height[nx][ny] + randFloat() * 0.04f;
                    if (h < bestH) {
                        bestH = h;
                        bestX = nx;
                        bestY = ny;
                    }
                }
                if (reachedWater || bestX < 0) break;
                x = bestX;
                y = bestY;
            }

            // Only keep rivers that actually drain into water
            if (reachedWater && path.size() >= 6) {
                for (auto& p : path) world[p.first][p.second].terrain = Terrain::Water;
            }
        }
    }

    // 4. Sand along the ocean shoreline
    void placeBeaches() {
        for (int i = 0; i < W; i++) {
            for (int j = 0; j < H; j++) {
                if (world[i][j].terrain != Terrain::Grass) continue;
                for (int d = 0; d < 4; d++) {
                    int nx = i + DX4[d];
                    int ny = j + DY4[d];
                    if (inWorldBounds(nx, ny) && ocean[nx][ny]) {
                        world[i][j].terrain = Terrain::Sand;
                        break;
                    }
                }
            }
        }
    }

    // 5. Rocky hills on the highest ~8% of land
    void placeRock() {
        std::vector<float> landHeights;
        for (int i = 0; i < W; i++)
            for (int j = 0; j < H; j++)
                if (world[i][j].terrain == Terrain::Grass) landHeights.push_back(height[i][j]);
        if (landHeights.empty()) return;
        float rockLevel = percentile(landHeights, 0.92f);
        for (int i = 0; i < W; i++)
            for (int j = 0; j < H; j++)
                if (world[i][j].terrain == Terrain::Grass && height[i][j] >= rockLevel)
                    world[i][j].terrain = Terrain::Rock;
    }

    // 6. Forests on the wettest ~22% of grassland
    void placeForests() {
        std::vector<float> wet;
        for (int i = 0; i < W; i++)
            for (int j = 0; j < H; j++)
                if (world[i][j].terrain == Terrain::Grass) wet.push_back(moisture[i][j]);
        if (wet.empty()) return;
        float forestLevel = percentile(wet, 0.78f);
        for (int i = 0; i < W; i++)
            for (int j = 0; j < H; j++)
                if (world[i][j].terrain == Terrain::Grass && moisture[i][j] >= forestLevel)
                    world[i][j].terrain = Terrain::Forest;
    }

    bool isForestEdge(int x, int y) const {
        if (world[x][y].terrain != Terrain::Forest) return false;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX4[d];
            int ny = y + DY4[d];
            if (inWorldBounds(nx, ny) && world[nx][ny].terrain == Terrain::Grass) return true;
        }
        return false;
    }

    // 7. Trails winding between forest edges
    void carveForestPaths() {
        std::vector<std::pair<int, int>> edges;
        for (int i = 0; i < W; i++)
            for (int j = 0; j < H; j++)
                if (isForestEdge(i, j)) edges.push_back({i, j});
        if (edges.size() < 2) return;

        int pathCount = randInt(2, 4);
        for (int p = 0; p < pathCount; p++) {
            auto start = edges[static_cast<size_t>(randInt(0, static_cast<int>(edges.size()) - 1))];

            // Pick a target edge 8-25 tiles away
            std::pair<int, int> target = {-1, -1};
            for (int attempt = 0; attempt < 40; attempt++) {
                auto cand = edges[static_cast<size_t>(randInt(0, static_cast<int>(edges.size()) - 1))];
                int dist = std::abs(cand.first - start.first) + std::abs(cand.second - start.second);
                if (dist >= 8 && dist <= 25) {
                    target = cand;
                    break;
                }
            }
            if (target.first < 0) continue;

            int x = start.first;
            int y = start.second;
            for (int step = 0; step < 60; step++) {
                if (world[x][y].terrain == Terrain::Forest) world[x][y].terrain = Terrain::ForestPath;
                if (x == target.first && y == target.second) break;

                int remX = target.first - x;
                int remY = target.second - y;
                bool moveX;
                if (remX == 0) moveX = false;
                else if (remY == 0) moveX = true;
                else moveX = randInt(0, std::abs(remX) + std::abs(remY) - 1) < std::abs(remX);

                int nx = x, ny = y;
                if (randInt(0, 3) == 0) { // wander sideways
                    if (moveX) ny += randInt(0, 1) ? 1 : -1;
                    else nx += randInt(0, 1) ? 1 : -1;
                } else if (moveX) {
                    nx += (remX > 0) ? 1 : -1;
                } else {
                    ny += (remY > 0) ? 1 : -1;
                }

                if (!inWorldBounds(nx, ny)) continue;
                Terrain t = world[nx][ny].terrain;
                if (t == Terrain::Water || t == Terrain::Rock) break;
                x = nx;
                y = ny;
            }
        }
    }

    // 8. Spawn: best buildable area near the centre with water close by
    void chooseSpawn() {
        computeWaterDistance();

        const int half = 4; // spawn area is 8x8 tiles: [c-4, c+3]
        int bestX = W / 2, bestY = H / 2;
        float bestScore = -1e9f;
        for (int cx = 16; cx <= W - 16; cx += 2) {
            for (int cy = 16; cy <= H - 16; cy += 2) {
                float score = 0.f;
                for (int i = cx - 6; i < cx + 6; i++) {
                    for (int j = cy - 6; j < cy + 6; j++) {
                        Terrain t = world[i][j].terrain;
                        if (t == Terrain::Grass || t == Terrain::ForestPath) score += 1.f;
                        else if (t == Terrain::Water && i >= cx - half && i < cx + half && j >= cy - half && j < cy + half) score -= 6.f;
                    }
                }
                if (waterDist[cx][cy] <= 8) score += 40.f;
                score -= 0.5f * static_cast<float>(std::abs(cx - W / 2) + std::abs(cy - H / 2));
                if (score > bestScore) {
                    bestScore = score;
                    bestX = cx;
                    bestY = cy;
                }
            }
        }

        worldSpawnX = bestX;
        worldSpawnY = bestY;

        for (int i = bestX - half; i < bestX + half; i++)
            for (int j = bestY - half; j < bestY + half; j++)
                world[i][j].terrain = Terrain::Grass;

        // Guarantee water within 8 tiles: dig a small pond just outside the spawn area
        computeWaterDistance();
        if (waterDist[bestX][bestY] > 8) {
            int px = bestX + half + 2;
            int py = bestY;
            for (int i = px - 1; i <= px + 1; i++)
                for (int j = py - 1; j <= py + 1; j++)
                    if (inWorldBounds(i, j)) world[i][j].terrain = Terrain::Water;
        }
    }

    // BFS distance (4-neighbour) from every tile to the nearest water tile
    void computeWaterDistance() {
        std::queue<std::pair<int, int>> q;
        for (int i = 0; i < W; i++) {
            for (int j = 0; j < H; j++) {
                if (world[i][j].terrain == Terrain::Water) {
                    waterDist[i][j] = 0;
                    q.push({i, j});
                } else {
                    waterDist[i][j] = 9999;
                }
            }
        }
        while (!q.empty()) {
            auto [x, y] = q.front();
            q.pop();
            for (int d = 0; d < 4; d++) {
                int nx = x + DX4[d];
                int ny = y + DY4[d];
                if (!inWorldBounds(nx, ny) || waterDist[nx][ny] <= waterDist[x][y] + 1) continue;
                waterDist[nx][ny] = waterDist[x][y] + 1;
                q.push({nx, ny});
            }
        }
    }

    // 9. Fertility: wet soil and river/lake banks are best
    void computeFertility() {
        for (int i = 0; i < W; i++) {
            for (int j = 0; j < H; j++) {
                Terrain t = world[i][j].terrain;
                if (t == Terrain::Water || t == Terrain::Sand || t == Terrain::Rock) {
                    world[i][j].fertility = 0;
                    continue;
                }
                float f = moisture[i][j] * 75.f;
                f += std::max(0.f, 30.f - 5.f * static_cast<float>(waterDist[i][j]));
                f += (lattice(i, j, seed + 303u) - 0.5f) * 12.f;
                world[i][j].fertility = clampByte(f);
            }
        }
    }

    // 10. Groundwater: lowlands and tiles near surface water hold more
    void computeGroundwater() {
        for (int i = 0; i < W; i++) {
            for (int j = 0; j < H; j++) {
                Terrain t = world[i][j].terrain;
                if (t == Terrain::Water) {
                    world[i][j].groundwater = 0;
                    continue;
                }
                float g = (1.f - std::min(1.f, std::max(0.f, height[i][j]))) * 55.f;
                g += std::max(0.f, 40.f - 4.f * static_cast<float>(waterDist[i][j]));
                g += (fbm(static_cast<float>(i), static_cast<float>(j), 2, 1.f / 8.f, seed + 404u) - 0.5f) * 30.f;
                if (t == Terrain::Rock) g *= 0.5f;
                world[i][j].groundwater = clampByte(g);
            }
        }
    }

    void stampBlob(int cx, int cy, int radius, Deposit type) {
        for (int i = cx - radius; i <= cx + radius; i++) {
            for (int j = cy - radius; j <= cy + radius; j++) {
                if (!inWorldBounds(i, j)) continue;
                if (world[i][j].terrain == Terrain::Water) continue;
                if (world[i][j].deposit != Deposit::None) continue;
                float d = std::sqrt(static_cast<float>((i - cx) * (i - cx) + (j - cy) * (j - cy)));
                if (d > static_cast<float>(radius) + 0.25f) continue;
                float amount = 100.f * (1.f - d / static_cast<float>(radius + 1));
                amount += (lattice(i, j, seed + 505u) - 0.5f) * 20.f;
                if (amount < 8.f) continue;
                world[i][j].deposit = type;
                world[i][j].depositAmount = clampByte(amount);
            }
        }
    }

    bool nearRock(int x, int y, int r) const {
        for (int i = x - r; i <= x + r; i++)
            for (int j = y - r; j <= y + r; j++)
                if (inWorldBounds(i, j) && world[i][j].terrain == Terrain::Rock) return true;
        return false;
    }

    // 11. Ore & stone clusters: stone in hills, iron at hill feet, coal in lowlands
    void placeDeposits() {
        std::vector<std::pair<int, int>> rockTiles, footTiles, lowTiles, landTiles;
        std::vector<float> landHeights;
        for (int i = 0; i < W; i++)
            for (int j = 0; j < H; j++)
                if (world[i][j].terrain != Terrain::Water) landHeights.push_back(height[i][j]);
        float lowLevel = percentile(landHeights, 0.5f);

        for (int i = 0; i < W; i++) {
            for (int j = 0; j < H; j++) {
                Terrain t = world[i][j].terrain;
                if (t == Terrain::Water) continue;
                landTiles.push_back({i, j});
                if (t == Terrain::Rock) rockTiles.push_back({i, j});
                else if (nearRock(i, j, 3)) footTiles.push_back({i, j});
                else if (height[i][j] <= lowLevel && t != Terrain::Sand) lowTiles.push_back({i, j});
            }
        }
        if (landTiles.empty()) return;

        auto pick = [&](const std::vector<std::pair<int, int>>& pool) {
            const auto& src = pool.empty() ? landTiles : pool;
            return src[static_cast<size_t>(randInt(0, static_cast<int>(src.size()) - 1))];
        };

        // Rarest first, so larger blobs cannot swallow them
        int ironBlobs = randInt(2, 4);
        for (int b = 0; b < ironBlobs; b++) {
            auto c = pick(footTiles);
            stampBlob(c.first, c.second, randInt(1, 2), Deposit::Iron);
        }
        int stoneBlobs = randInt(3, 5);
        for (int b = 0; b < stoneBlobs; b++) {
            auto c = pick(rockTiles);
            stampBlob(c.first, c.second, randInt(2, 3), Deposit::Stone);
        }
        int coalBlobs = randInt(3, 5);
        for (int b = 0; b < coalBlobs; b++) {
            auto c = pick(lowTiles);
            stampBlob(c.first, c.second, randInt(2, 3), Deposit::Coal);
        }
    }
};

} // namespace worldgen

// Generates terrain, soil and resources into world[][] for the given seed.
inline void generateWorld(uint32_t seed) {
    worldSeed = seed;
    auto gen = std::make_unique<worldgen::Generator>(seed); // ~64 KB of scratch maps, keep off the stack
    gen->run();
}
