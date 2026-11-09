#pragma once
#include <vector>
#include <cstdlib>
#include <cmath>
#include <string>
#include <algorithm>
#include <SFML/Graphics.hpp>
#include "../../src/_global.hpp"

enum VehicleDirection {
    VEH_DIR_SE = 0, // Moving (+1, 0): down-right along +X
    VEH_DIR_NW = 1, // Moving (-1, 0): up-left along -X
    VEH_DIR_SW = 2, // Moving (0, +1): down along +Y
    VEH_DIR_NE = 3  // Moving (0, -1): up along -Y
};

enum VehicleType {
    VEH_SEDAN = 0,
    VEH_TAXI  = 1,
    VEH_COUPE = 2,
    VEH_VAN   = 3
};

struct Vehicle {
    sf::Vector2i currentTile{-1, -1};
    sf::Vector2i targetTile{-1, -1};
    sf::Vector2i prevTile{-1, -1};
    float progress = 0.0f; // 0.0f to 1.0f
    float speed = 55.0f;   // pixels per second
    VehicleDirection dir = VEH_DIR_SE;
    VehicleType type = VEH_SEDAN;
    sf::Vector2f worldPos{0.f, 0.f};

    int getDepth() const {
        return (progress < 0.5f) ? (currentTile.x + currentTile.y) : (targetTile.x + targetTile.y);
    }
};

class VehicleSystem {
private:
    std::vector<Vehicle> vehicles;
    float spawnAccumulator = 0.0f;
    int maxVehicles = 40;

public:
    VehicleSystem() = default;

    void clear() {
        vehicles.clear();
        spawnAccumulator = 0.0f;
    }

    size_t getVehicleCount() const {
        return vehicles.size();
    }

    const std::vector<Vehicle>& getVehicles() const {
        return vehicles;
    }

    static sf::Vector2f getTileCenter(int x, int y) {
        float px = static_cast<float>(tileSizeX * x - x * (tileSizeX / 2));
        float py = static_cast<float>(tileSizeY * y + x * (tileSizeY / 2));
        return sf::Vector2f(px + static_cast<float>(tileSizeX) / 2.f,
                            py + static_cast<float>(tileSizeY) / 2.f);
    }

    static VehicleDirection computeDirection(const sf::Vector2i& from, const sf::Vector2i& to) {
        int dx = to.x - from.x;
        int dy = to.y - from.y;
        if (dx > 0 && dy == 0) return VEH_DIR_SE; // Axis 1 (+1, 0): down-right
        if (dx < 0 && dy == 0) return VEH_DIR_NW; // Axis 1 (-1, 0): up-left
        if (dx < 0 && dy > 0)  return VEH_DIR_SW; // Axis 2 (-1, +1): down-left
        if (dx > 0 && dy < 0)  return VEH_DIR_NE; // Axis 2 (+1, -1): up-right
        if (dy > 0) return VEH_DIR_SW;            // Vertical (0, +1) fallback: down
        if (dy < 0) return VEH_DIR_NE;            // Vertical (0, -1) fallback: up
        if (dx > 0) return VEH_DIR_SE;
        return VEH_DIR_NW;
    }

    static std::string getTextureKey(VehicleType type, VehicleDirection dir) {
        std::string tName;
        switch (type) {
            case VEH_TAXI:  tName = "car_taxi_"; break;
            case VEH_COUPE: tName = "car_coupe_"; break;
            case VEH_VAN:   tName = "car_van_"; break;
            default:        tName = "car_sedan_"; break;
        }

        std::string dName;
        switch (dir) {
            case VEH_DIR_NW: dName = "nw"; break;
            case VEH_DIR_SW: dName = "sw"; break;
            case VEH_DIR_NE: dName = "ne"; break;
            default:         dName = "se"; break;
        }

        std::string key = tName + dName;
        if (m_textures.count(key)) {
            return key;
        }
        // Fallback to generic car_<dir>
        std::string fallback = "car_" + dName;
        if (m_textures.count(fallback)) {
            return fallback;
        }
        return "car_se";
    }

    bool isRoad(int x, int y) const {
        if (x < 0 || x >= tileMapW || y < 0 || y >= tileMapH) return false;
        if (entities[x][y] == nullptr) return false;
        return (entities[x][y]->getName() == "DirtRoad");
    }

    std::vector<sf::Vector2i> getConnectedRoads(const sf::Vector2i& tile) const {
        std::vector<sf::Vector2i> neighbors;
        // Priority 1: Axis 1 (East/West: +1,0 and -1,0)
        // Priority 2: Axis 2 (SW/NE edge-sharing: -1,+1 and +1,-1)
        // Priority 3: Vertical fallback (South/North: 0,+1 and 0,-1)
        const int dx[6] = { 1, -1, -1,  1,  0,  0 };
        const int dy[6] = { 0,  0,  1, -1,  1, -1 };
        for (int i = 0; i < 6; i++) {
            int nx = tile.x + dx[i];
            int ny = tile.y + dy[i];
            if (isRoad(nx, ny)) {
                neighbors.push_back({nx, ny});
            }
        }
        return neighbors;
    }

    void spawnVehicle() {
        std::vector<sf::Vector2i> roadTiles;
        for (int x = 0; x < tileMapW; x++) {
            for (int y = 0; y < tileMapH; y++) {
                if (isRoad(x, y)) {
                    roadTiles.push_back({x, y});
                }
            }
        }
        if (roadTiles.empty()) return;

        // Shuffle candidate roads to find a free road with connected neighbors
        for (int attempts = 0; attempts < 15; attempts++) {
            int idx = std::rand() % roadTiles.size();
            sf::Vector2i startTile = roadTiles[idx];

            // Don't spawn on top of an existing vehicle
            bool occupied = false;
            for (const auto& v : vehicles) {
                if (v.currentTile == startTile || v.targetTile == startTile) {
                    occupied = true;
                    break;
                }
            }
            if (occupied) continue;

            auto neighbors = getConnectedRoads(startTile);
            if (neighbors.empty()) continue;

            sf::Vector2i nextTile = neighbors[std::rand() % neighbors.size()];

            Vehicle v;
            v.currentTile = startTile;
            v.targetTile = nextTile;
            v.prevTile = startTile;
            v.progress = 0.0f;
            v.speed = 45.0f + static_cast<float>(std::rand() % 25);
            v.dir = computeDirection(startTile, nextTile);
            v.type = static_cast<VehicleType>(std::rand() % 4);
            v.worldPos = getTileCenter(startTile.x, startTile.y);

            vehicles.push_back(v);
            break;
        }
    }

    void addVehicleDirect(const Vehicle& v) {
        vehicles.push_back(v);
    }

    void update(float deltaTime) {
        // 1. Maintain dynamic vehicle fleet count
        int totalRoads = 0;
        for (int x = 0; x < tileMapW; x++) {
            for (int y = 0; y < tileMapH; y++) {
                if (isRoad(x, y)) totalRoads++;
            }
        }

        int targetVehicles = std::min(maxVehicles, std::max(2, totalRoads / 5));
        spawnAccumulator += deltaTime;
        if (spawnAccumulator >= 1.5f) {
            spawnAccumulator = 0.0f;
            if (static_cast<int>(vehicles.size()) < targetVehicles) {
                spawnVehicle();
            }
        }

        // 2. Update existing vehicles
        for (size_t i = 0; i < vehicles.size(); ) {
            Vehicle& v = vehicles[i];

            // Despawn if current road or target road was demolished
            if (!isRoad(v.currentTile.x, v.currentTile.y) || !isRoad(v.targetTile.x, v.targetTile.y)) {
                vehicles.erase(vehicles.begin() + i);
                continue;
            }

            sf::Vector2f startPos = getTileCenter(v.currentTile.x, v.currentTile.y);
            sf::Vector2f endPos   = getTileCenter(v.targetTile.x, v.targetTile.y);
            sf::Vector2f delta = endPos - startPos;
            float segLen = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            if (segLen < 1.0f) segLen = 32.0f;

            v.progress += (v.speed * deltaTime) / segLen;

            if (v.progress >= 1.0f) {
                // Arrived at target tile
                v.prevTile = v.currentTile;
                v.currentTile = v.targetTile;
                v.progress = 0.0f;

                auto neighbors = getConnectedRoads(v.currentTile);
                if (neighbors.empty()) {
                    // No road connections left
                    vehicles.erase(vehicles.begin() + i);
                    continue;
                }

                // Filter out the tile it came from (to prevent constant ping-pong reversing)
                std::vector<sf::Vector2i> forwardOptions;
                for (const auto& nb : neighbors) {
                    if (nb != v.prevTile) {
                        forwardOptions.push_back(nb);
                    }
                }

                sf::Vector2i chosenNext;
                if (!forwardOptions.empty()) {
                    // Try to continue in same direction if possible (65% preference)
                    sf::Vector2i straightCandidate(
                        v.currentTile.x + (v.currentTile.x - v.prevTile.x),
                        v.currentTile.y + (v.currentTile.y - v.prevTile.y)
                    );
                    bool hasStraight = false;
                    for (const auto& opt : forwardOptions) {
                        if (opt == straightCandidate) {
                            hasStraight = true;
                            break;
                        }
                    }

                    if (hasStraight && (std::rand() % 100 < 65)) {
                        chosenNext = straightCandidate;
                    } else {
                        chosenNext = forwardOptions[std::rand() % forwardOptions.size()];
                    }
                } else {
                    // Dead end: turn around and go back
                    chosenNext = v.prevTile;
                }

                v.targetTile = chosenNext;
                v.dir = computeDirection(v.currentTile, v.targetTile);
            }

            // Interpolate position along segment
            startPos = getTileCenter(v.currentTile.x, v.currentTile.y);
            endPos   = getTileCenter(v.targetTile.x, v.targetTile.y);
            v.worldPos = startPos + v.progress * (endPos - startPos);

            // Subtle lane offset to the right side of the driving direction
            sf::Vector2f laneShift(0.f, 0.f);
            switch (v.dir) {
                case VEH_DIR_SE: laneShift = sf::Vector2f(-1.5f,  3.0f); break;
                case VEH_DIR_NW: laneShift = sf::Vector2f( 1.5f, -3.0f); break;
                case VEH_DIR_SW: laneShift = sf::Vector2f(-3.0f, -1.5f); break;
                case VEH_DIR_NE: laneShift = sf::Vector2f( 3.0f,  1.5f); break;
            }
            v.worldPos += laneShift;

            i++;
        }
    }

    void drawVehicle(sf::RenderWindow& window, const Vehicle& v) const {
        std::string texKey = getTextureKey(v.type, v.dir);
        if (!m_textures.count(texKey)) return;

        sf::Sprite sprite;
        sprite.setTexture(m_textures[texKey]);
        sf::Vector2u sz = m_textures[texKey].getSize();
        sprite.setOrigin(static_cast<float>(sz.x) / 2.f, static_cast<float>(sz.y) / 2.f + 2.f);
        sprite.setPosition(v.worldPos);
        sprite.setScale(1.15f, 1.15f);

        window.draw(sprite);
    }

    void draw(sf::RenderWindow& window) const {
        for (const auto& v : vehicles) {
            drawVehicle(window, v);
        }
    }

    void drawAtDepth(sf::RenderWindow& window, int depth) const {
        for (const auto& v : vehicles) {
            if (v.getDepth() == depth) {
                drawVehicle(window, v);
            }
        }
    }
};
