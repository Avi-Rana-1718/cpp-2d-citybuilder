#pragma once
#include <vector>
#include <cstdlib>
#include <cmath>
#include <string>
#include <SFML/Graphics.hpp>
#include "../../src/_global.hpp"
#include "../entities/_Zone.hpp"
#include "../entities/FarmZone.hpp"
#include "../entities/IndustrialZone.hpp"
#include "../entities/ResidentalZone.hpp"
#include "../entities/CommericalZone.hpp"
#include "../entities/ParkZone.hpp"
#include "../entities/HarbourZone.hpp"

struct SmokeParticle {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float age = 0.0f;
    float lifetime = 2.0f;
    float scale = 0.8f;
    int tier = 1;
};

class BuildingAnimationSystem {
private:
    float windmillAngle = 0.0f;
    float smokeSpawnTimer = 0.0f;
    float fountainTimer = 0.0f;
    int fountainFrame = 0;
    float beaconTimer = 0.0f;
    float harbourTimer = 0.0f;

    std::vector<SmokeParticle> smokeParticles;

public:
    BuildingAnimationSystem() = default;

    void clear() {
        smokeParticles.clear();
        windmillAngle = 0.0f;
        smokeSpawnTimer = 0.0f;
        fountainTimer = 0.0f;
        fountainFrame = 0;
        beaconTimer = 0.0f;
        harbourTimer = 0.0f;
    }

    float getWindmillAngle() const { return windmillAngle; }
    int getFountainFrame() const { return fountainFrame; }
    bool isBeaconOn() const { return beaconTimer < 0.25f; }
    size_t getParticleCount() const { return smokeParticles.size(); }

    void update(float deltaTime) {
        // 1. Windmill continuous rotation (~90 deg/s)
        windmillAngle += 90.0f * deltaTime;
        if (windmillAngle >= 360.0f) {
            windmillAngle -= 360.0f;
        }

        // 2. Fountain water animation (3-frame cycle every 0.16s)
        fountainTimer += deltaTime;
        if (fountainTimer >= 0.16f) {
            fountainTimer = 0.0f;
            fountainFrame = (fountainFrame + 1) % 3;
        }

        // 3. Skyscraper Beacon strobe cycle (1.2s total, 0.25s ON)
        beaconTimer += deltaTime;
        if (beaconTimer >= 1.2f) {
            beaconTimer = 0.0f;
        }

        // 4. Harbour strobe cycle (1.5s total)
        harbourTimer += deltaTime;
        if (harbourTimer >= 1.5f) {
            harbourTimer = 0.0f;
        }

        // 5. Spawn Industrial Smoke Particles
        smokeSpawnTimer += deltaTime;
        if (smokeSpawnTimer >= 0.35f) {
            smokeSpawnTimer = 0.0f;

            for (int x = 0; x < tileMapW; x++) {
                for (int y = 0; y < tileMapH; y++) {
                    if (entities[x][y] == nullptr) continue;
                    Zone* z = dynamic_cast<Zone*>(entities[x][y]);
                    if (z == nullptr || z->isSubTile) continue;

                    if (z->getName() == "IndustrialZone" && z->isActive) {
                        float px = z->position->x;
                        float py = z->position->y;

                        SmokeParticle p;
                        if (z->tier == 1) {
                            // T1 scrap workshop / foundry: smokestack at top-left
                            p.pos = sf::Vector2f(px + 8.f, py + 2.f);
                            p.vel = sf::Vector2f(3.5f + static_cast<float>(std::rand() % 4),
                                                -22.0f - static_cast<float>(std::rand() % 6));
                            p.tier = 1;
                        } else if (z->tier == 2) {
                            // T2 factory smokestacks near center
                            p.pos = sf::Vector2f(px + 46.f, py + 8.f);
                            p.vel = sf::Vector2f(4.0f + static_cast<float>(std::rand() % 5),
                                                -26.0f - static_cast<float>(std::rand() % 8));
                            p.tier = 2;
                        } else {
                            // T3 clean-fab exhaust vent
                            p.pos = sf::Vector2f(px + 52.f, py + 20.f);
                            p.vel = sf::Vector2f(2.5f + static_cast<float>(std::rand() % 3),
                                                -18.0f - static_cast<float>(std::rand() % 5));
                            p.tier = 3;
                        }

                        p.age = 0.0f;
                        p.lifetime = 1.8f + static_cast<float>(std::rand() % 6) / 10.f;
                        p.scale = 0.7f;
                        smokeParticles.push_back(p);
                    }
                }
            }
        }

        // 6. Update existing smoke particles
        for (size_t i = 0; i < smokeParticles.size(); ) {
            SmokeParticle& p = smokeParticles[i];
            p.age += deltaTime;
            if (p.age >= p.lifetime) {
                smokeParticles.erase(smokeParticles.begin() + i);
                continue;
            }

            p.pos += p.vel * deltaTime;
            p.scale += 0.35f * deltaTime;
            i++;
        }
    }

    void draw(sf::RenderWindow& window) const {
        // 1. Windmill Blades Overlay on FarmZone (variant 2)
        if (m_textures.count("windmill_blades")) {
            sf::Sprite blades;
            blades.setTexture(m_textures["windmill_blades"]);
            sf::Vector2u sz = m_textures["windmill_blades"].getSize();
            blades.setOrigin(static_cast<float>(sz.x) / 2.f, static_cast<float>(sz.y) / 2.f);
            blades.setRotation(windmillAngle);
            blades.setScale(1.2f, 1.2f);

            for (int x = 0; x < tileMapW; x++) {
                for (int y = 0; y < tileMapH; y++) {
                    if (entities[x][y] == nullptr) continue;
                    Zone* z = dynamic_cast<Zone*>(entities[x][y]);
                    if (z == nullptr || z->isSubTile) continue;

                    if (z->getName() == "FarmZone") {
                        FarmZone* fz = dynamic_cast<FarmZone*>(z);
                        if (fz != nullptr && fz->variant == 2) {
                            // Windmill axle hub position
                            float px = z->position->x;
                            float py = z->position->y;
                            blades.setPosition(px + 36.f, py + 6.f);
                            window.draw(blades);
                        }
                    }
                }
            }
        }

        // 2. Animated Park Fountain Water
        std::string fountainKey = "fountain_" + std::to_string(fountainFrame);
        if (m_textures.count(fountainKey)) {
            sf::Sprite fountainSprite;
            fountainSprite.setTexture(m_textures[fountainKey]);
            sf::Vector2u sz = m_textures[fountainKey].getSize();
            fountainSprite.setOrigin(static_cast<float>(sz.x) / 2.f, static_cast<float>(sz.y) / 2.f);
            fountainSprite.setScale(1.3f, 1.3f);

            for (int x = 0; x < tileMapW; x++) {
                for (int y = 0; y < tileMapH; y++) {
                    if (entities[x][y] == nullptr) continue;
                    Zone* z = dynamic_cast<Zone*>(entities[x][y]);
                    if (z == nullptr || z->isSubTile) continue;

                    if (z->getName() == "ParkZone") {
                        float px = z->position->x;
                        float py = z->position->y;
                        fountainSprite.setPosition(px + 48.f, py + 22.f);
                        window.draw(fountainSprite);
                    }
                }
            }
        }

        // 3. Skyscraper Rooftop Red Warning Beacons
        if (isBeaconOn() && m_textures.count("beacon_light")) {
            sf::Sprite beacon;
            beacon.setTexture(m_textures["beacon_light"]);
            sf::Vector2u sz = m_textures["beacon_light"].getSize();
            beacon.setOrigin(static_cast<float>(sz.x) / 2.f, static_cast<float>(sz.y) / 2.f);
            beacon.setScale(1.4f, 1.4f);

            for (int x = 0; x < tileMapW; x++) {
                for (int y = 0; y < tileMapH; y++) {
                    if (entities[x][y] == nullptr) continue;
                    Zone* z = dynamic_cast<Zone*>(entities[x][y]);
                    if (z == nullptr || z->isSubTile) continue;

                    float px = z->position->x;
                    float py = z->position->y;

                    // Residential T3 Spire Beacon
                    if (z->getName() == "ResidentalZone" && z->tier == 3) {
                        beacon.setPosition(px + 48.f, py - 30.f);
                        window.draw(beacon);
                    }
                    // Commercial T3 Spire Beacon
                    else if (z->getName() == "CommericalZone" && z->tier == 3) {
                        beacon.setPosition(px + 46.f, py - 54.f);
                        window.draw(beacon);
                    }
                }
            }
        }

        // 4. Harbour Crane Warning Light
        if ((harbourTimer < 0.35f) && m_textures.count("harbour_crane_light")) {
            sf::Sprite craneLight;
            craneLight.setTexture(m_textures["harbour_crane_light"]);
            sf::Vector2u sz = m_textures["harbour_crane_light"].getSize();
            craneLight.setOrigin(static_cast<float>(sz.x) / 2.f, static_cast<float>(sz.y) / 2.f);
            craneLight.setScale(1.3f, 1.3f);

            for (int x = 0; x < tileMapW; x++) {
                for (int y = 0; y < tileMapH; y++) {
                    if (entities[x][y] == nullptr) continue;
                    Zone* z = dynamic_cast<Zone*>(entities[x][y]);
                    if (z == nullptr || z->isSubTile) continue;

                    if (z->getName() == "HarbourZone") {
                        float px = z->position->x;
                        float py = z->position->y;
                        craneLight.setPosition(px + 40.f, py - 12.f);
                        window.draw(craneLight);
                    }
                }
            }
        }

        // 5. Billowing Industrial Smoke Particles
        for (const auto& p : smokeParticles) {
            float normAge = p.age / p.lifetime;
            int frameIdx = std::min(3, static_cast<int>(normAge * 4.0f));
            std::string key = "smoke_puff_" + std::to_string(frameIdx);
            if (!m_textures.count(key)) continue;

            sf::Sprite pSprite;
            pSprite.setTexture(m_textures[key]);
            sf::Vector2u sz = m_textures[key].getSize();
            pSprite.setOrigin(static_cast<float>(sz.x) / 2.f, static_cast<float>(sz.y) / 2.f);
            pSprite.setPosition(p.pos);
            pSprite.setScale(p.scale, p.scale);

            // Opacity fade as particle rises
            int alpha = static_cast<int>(230.f * (1.0f - normAge));
            if (alpha < 0) alpha = 0;

            if (p.tier == 1) {
                // Darker smoke
                pSprite.setColor(sf::Color(140, 140, 150, alpha));
            } else if (p.tier == 2) {
                // Grey/white steam
                pSprite.setColor(sf::Color(190, 195, 205, alpha));
            } else {
                // Clean white mist
                pSprite.setColor(sf::Color(230, 240, 250, alpha));
            }

            window.draw(pSprite);
        }
    }
};
