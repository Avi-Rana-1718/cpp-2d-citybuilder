#pragma once
#include <vector>
#include <cstdlib>
#include <cmath>
#include <string>
#include <algorithm>
#include <SFML/Graphics.hpp>
#include "../../src/_global.hpp"
#include "../entities/_Zone.hpp"

struct Citizen {
    sf::Vector2f worldPos{0.f, 0.f};
    sf::Vector2f targetPos{0.f, 0.f};
    int currentTileX = 0;
    int currentTileY = 0;
    int targetTileX = 0;
    int targetTileY = 0;

    int citizenType = 0; // 0: Casual, 1: Business, 2: Hoodie, 3: Student
    int walkFrame = 0;   // 0 or 1
    float animTimer = 0.0f;
    float pauseTimer = 0.0f;
    float speed = 24.0f; // px/sec
    bool isWalking = true;
    bool inCivic = false;

    int getDepth() const {
        return currentTileX + currentTileY;
    }
};

class PedestrianSystem {
private:
    std::vector<Citizen> citizens;
    float spawnAccumulator = 0.0f;
    int maxCitizens = 50;

public:
    PedestrianSystem() = default;

    void clear() {
        citizens.clear();
        spawnAccumulator = 0.0f;
    }

    size_t getCitizenCount() const {
        return citizens.size();
    }

    const std::vector<Citizen>& getCitizens() const {
        return citizens;
    }

    static sf::Vector2f getTileBaseCenter(int x, int y) {
        float px = static_cast<float>(tileSizeX * x - x * (tileSizeX / 2));
        float py = static_cast<float>(tileSizeY * y + x * (tileSizeY / 2));
        return sf::Vector2f(px + static_cast<float>(tileSizeX) / 2.f,
                            py + static_cast<float>(tileSizeY) / 2.f);
    }

    bool isWalkable(int x, int y) const {
        if (x < 0 || x >= tileMapW || y < 0 || y >= tileMapH) return false;
        if (entities[x][y] == nullptr) return false;
        std::string n = entities[x][y]->getName();
        if (n == "DirtRoad") return true;
        if (n == "ParkZone" || n == "SchoolZone" || n == "ChurchZone") return true;
        if (n == "SubZone") return true;
        return false;
    }

    bool isCivicGround(int x, int y) const {
        if (x < 0 || x >= tileMapW || y < 0 || y >= tileMapH) return false;
        if (entities[x][y] == nullptr) return false;
        std::string n = entities[x][y]->getName();
        return (n == "ParkZone" || n == "SchoolZone" || n == "ChurchZone");
    }

    sf::Vector2f generateTileWaypoint(int tx, int ty) const {
        sf::Vector2f center = getTileBaseCenter(tx, ty);
        if (isCivicGround(tx, ty)) {
            // Roam within park / civic grounds
            float ox = static_cast<float>((std::rand() % 24) - 12);
            float oy = static_cast<float>((std::rand() % 14) - 7);
            return center + sf::Vector2f(ox, oy);
        } else {
            // Road sidewalk curb offset
            // Offset towards the upper-left or lower-right sidewalk curb
            bool side = (std::rand() % 2 == 0);
            float ox = side ? -10.f : 10.f;
            float oy = side ? -5.f : 5.f;
            return center + sf::Vector2f(ox, oy);
        }
    }

    void spawnCitizen() {
        std::vector<sf::Vector2i> spawnCandidates;
        for (int x = 0; x < tileMapW; x++) {
            for (int y = 0; y < tileMapH; y++) {
                if (isWalkable(x, y)) {
                    spawnCandidates.push_back({x, y});
                }
            }
        }
        if (spawnCandidates.empty()) return;

        sf::Vector2i st = spawnCandidates[std::rand() % spawnCandidates.size()];
        Citizen c;
        c.currentTileX = st.x;
        c.currentTileY = st.y;
        c.targetTileX = st.x;
        c.targetTileY = st.y;
        c.worldPos = generateTileWaypoint(st.x, st.y);
        c.targetPos = c.worldPos;
        c.speed = 22.0f + static_cast<float>(std::rand() % 12);
        c.walkFrame = std::rand() % 2;
        c.animTimer = 0.0f;
        c.pauseTimer = static_cast<float>(std::rand() % 8) / 10.f;
        c.isWalking = false;
        c.inCivic = isCivicGround(st.x, st.y);

        // Citizen type selection based on local context
        if (entities[st.x][st.y] != nullptr) {
            std::string n = entities[st.x][st.y]->getName();
            if (n == "SchoolZone") {
                c.citizenType = 3; // Student
            } else if (n == "CommericalZone") {
                c.citizenType = 1; // Business suit
            } else {
                c.citizenType = (std::rand() % 2 == 0) ? 0 : 2; // Casual / Hoodie
            }
        } else {
            c.citizenType = std::rand() % 4;
        }

        citizens.push_back(c);
    }

    void addCitizenDirect(const Citizen& c) {
        citizens.push_back(c);
    }

    void update(float deltaTime) {
        // 1. Maintain citizen population
        int totalWalkables = 0;
        for (int x = 0; x < tileMapW; x++) {
            for (int y = 0; y < tileMapH; y++) {
                if (isWalkable(x, y)) totalWalkables++;
            }
        }

        int targetCitizens = std::min(maxCitizens, std::max(3, totalWalkables / 4));
        spawnAccumulator += deltaTime;
        if (spawnAccumulator >= 1.2f) {
            spawnAccumulator = 0.0f;
            if (static_cast<int>(citizens.size()) < targetCitizens) {
                spawnCitizen();
            }
        }

        // 2. Update citizen movements
        for (size_t i = 0; i < citizens.size(); ) {
            Citizen& c = citizens[i];

            // If tile became invalid (e.g., bulldozed to water), remove
            if (!isWalkable(c.currentTileX, c.currentTileY)) {
                citizens.erase(citizens.begin() + i);
                continue;
            }

            if (c.pauseTimer > 0.f) {
                c.pauseTimer -= deltaTime;
                c.isWalking = false;
            } else {
                c.isWalking = true;
                // Move towards targetPos
                sf::Vector2f diff = c.targetPos - c.worldPos;
                float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
                if (dist <= c.speed * deltaTime || dist < 1.0f) {
                    c.worldPos = c.targetPos;
                    c.currentTileX = c.targetTileX;
                    c.currentTileY = c.targetTileY;
                    c.pauseTimer = 0.4f + static_cast<float>(std::rand() % 10) / 10.f;
                    c.isWalking = false;

                    // Pick next destination tile (Axis 1 East/West, Axis 2 SW/NE edge-sharing, and vertical fallback)
                    std::vector<sf::Vector2i> nextTiles;
                    const int dx[6] = {1, -1, -1,  1,  0,  0};
                    const int dy[6] = {0,  0,  1, -1,  1, -1};
                    for (int k = 0; k < 6; k++) {
                        int nx = c.currentTileX + dx[k];
                        int ny = c.currentTileY + dy[k];
                        if (isWalkable(nx, ny)) {
                            nextTiles.push_back({nx, ny});
                        }
                    }

                    if (!nextTiles.empty()) {
                        sf::Vector2i nextT = nextTiles[std::rand() % nextTiles.size()];
                        c.targetTileX = nextT.x;
                        c.targetTileY = nextT.y;
                        c.targetPos = generateTileWaypoint(nextT.x, nextT.y);
                    } else {
                        // Stay within current tile
                        c.targetPos = generateTileWaypoint(c.currentTileX, c.currentTileY);
                    }
                } else {
                    c.worldPos += (diff / dist) * (c.speed * deltaTime);
                    // Walk animation cycle
                    c.animTimer += deltaTime;
                    if (c.animTimer >= 0.18f) {
                        c.animTimer = 0.0f;
                        c.walkFrame = (c.walkFrame + 1) % 2;
                    }
                }
            }

            i++;
        }
    }

    void drawCitizen(sf::RenderWindow& window, const Citizen& c) const {
        std::string key = "citizen_" + std::to_string(c.citizenType) + "_walk_" + std::to_string(c.walkFrame);
        if (!m_textures.count(key)) {
            key = "citizen_walk_" + std::to_string(c.walkFrame);
        }
        if (!m_textures.count(key)) return;

        sf::Sprite sprite;
        sprite.setTexture(m_textures[key]);
        sf::Vector2u sz = m_textures[key].getSize();
        sprite.setOrigin(static_cast<float>(sz.x) / 2.f, static_cast<float>(sz.y) - 2.f);
        sprite.setPosition(c.worldPos);
        sprite.setScale(1.25f, 1.25f);

        window.draw(sprite);
    }

    void draw(sf::RenderWindow& window) const {
        for (const auto& c : citizens) {
            drawCitizen(window, c);
        }
    }

    void drawAtDepth(sf::RenderWindow& window, int depth) const {
        for (const auto& c : citizens) {
            if (c.getDepth() == depth) {
                drawCitizen(window, c);
            }
        }
    }
};
