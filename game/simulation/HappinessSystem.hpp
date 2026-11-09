#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include "../../src/_global.hpp"
#include "./EconomyData.hpp"

#include "../entities/ResidentalZone.hpp"
#include "../entities/CommericalZone.hpp"
#include "../entities/IndustrialZone.hpp"
#include "../entities/FarmZone.hpp"
#include "../entities/SchoolZone.hpp"
#include "../entities/ChurchZone.hpp"
#include "../entities/ParkZone.hpp"
#include "../entities/HarbourZone.hpp"

class HappinessSystem {
public:
    int cityHappiness = 50; // 0 - 100%

    // Spatial coverage maps (20x20)
    int educationMap[tileMapW][tileMapH];
    int churchMap[tileMapW][tileMapH];
    int parkMap[tileMapW][tileMapH];
    int pollutionMap[tileMapW][tileMapH];
    int commerceMap[tileMapW][tileMapH];

    HappinessSystem() {
        resetCoverage();
    }

    void resetCoverage() {
        std::memset(educationMap, 0, sizeof(educationMap));
        std::memset(churchMap, 0, sizeof(churchMap));
        std::memset(parkMap, 0, sizeof(parkMap));
        std::memset(pollutionMap, 0, sizeof(pollutionMap));
        std::memset(commerceMap, 0, sizeof(commerceMap));
    }

    // High performance spatial coverage pass
    void updateCoverage() {
        resetCoverage();

        for (int x = 0; x < tileMapW; x++) {
            for (int y = 0; y < tileMapH; y++) {
                if (entities[x][y] == NULL || !entities[x][y]->isActive) continue;

                Zone* z = dynamic_cast<Zone*>(entities[x][y]);
                if (z != NULL && z->isSubTile) continue;

                std::string name = entities[x][y]->getName();

                if (name == "SchoolZone") {
                    applyRadius(educationMap, x, y, 5, 100);
                } else if (name == "ChurchZone") {
                    applyRadius(churchMap, x, y, 4, 100);
                } else if (name == "ParkZone") {
                    applyRadius(parkMap, x, y, 4, 100);
                } else if (name == "IndustrialZone") {
                    applyRadius(pollutionMap, x, y, 3, 60);
                } else if (name == "CommericalZone") {
                    applyRadius(commerceMap, x, y, 3, 100);
                }
            }
        }
    }

    void updateSimulation() {
        updateCoverage();

        int totalResidentialHappiness = 0;
        int activeResidentialCount = 0;

        int totalJobs = townCapacity[1] + townCapacity[2];
        int totalWorkers = townPopulation[0];
        int jobRatioScore = 100;
        if (totalWorkers > 0) {
            jobRatioScore = std::min(100, (totalJobs * 100) / totalWorkers);
        }

        // Residents judge the average of their three tax sliders (9% -> 76)
        float avgResTax = (taxRates[SECTOR_R][0] + taxRates[SECTOR_R][1] + taxRates[SECTOR_R][2]) / 3.f;
        int taxSatisfaction = std::max(0, std::min(100, static_cast<int>(100.f - (avgResTax - 5.f) * 6.f)));

        int eduCount = 0;
        int safetyCount = 0;
        int healthCount = 0;
        int recCount = 0;

        for (int x = 0; x < tileMapW; x++) {
            for (int y = 0; y < tileMapH; y++) {
                if (entities[x][y] == NULL || !entities[x][y]->isActive) continue;

                if (entities[x][y]->getName() == "ResidentalZone") {
                    ResidentalZone* rz = dynamic_cast<ResidentalZone*>(entities[x][y]);
                    if (rz == NULL) continue;

                    activeResidentialCount++;

                    int edu = educationMap[x][y];
                    int faith = churchMap[x][y];
                    int parks = parkMap[x][y];
                    int pollution = pollutionMap[x][y];
                    int access = commerceMap[x][y];

                    int health = std::max(0, std::min(100, 70 + parks - pollution));
                    int safety = std::min(100, 60 + faith / 2);
                    int recreation = std::min(100, parks + (access / 2));

                    // Per-zone happiness score (weighted average)
                    int zoneHappy = static_cast<int>(
                        jobRatioScore   * 0.25f +
                        edu             * 0.20f +
                        health          * 0.20f +
                        safety          * 0.15f +
                        taxSatisfaction * 0.20f
                    );
                    zoneHappy = std::max(5, std::min(100, zoneHappy));

                    rz->overallHappiness = zoneHappy;
                    rz->happinessIndex[2] = edu;
                    rz->happinessIndex[3] = safety;
                    rz->happinessIndex[4] = health;
                    rz->happinessIndex[5] = recreation;
                    rz->happinessIndex[8] = jobRatioScore;

                    totalResidentialHappiness += zoneHappy;
                    eduCount += edu;
                    safetyCount += safety;
                    healthCount += health;
                    recCount += recreation;

                    // Tier changes are decided by EconomySystem (they need demand too)
                }
            }
        }

        // Global Town Happiness
        if (activeResidentialCount > 0) {
            cityHappiness = totalResidentialHappiness / activeResidentialCount;
            townStats[2] = eduCount / activeResidentialCount;
            townStats[3] = safetyCount / activeResidentialCount;
            townStats[4] = healthCount / activeResidentialCount;
            townStats[5] = recCount / activeResidentialCount;
        } else {
            cityHappiness = 50;
        }

        townStats[0] = taxSatisfaction;
        townStats[1] = townPopulation[0] + townPopulation[1] + townPopulation[2];
        townStats[8] = jobRatioScore;
    }

private:
    void applyRadius(int map[tileMapW][tileMapH], int cx, int cy, int radius, int value) {
        int rSq = radius * radius;
        for (int dx = -radius; dx <= radius; dx++) {
            int nx = cx + dx;
            if (nx < 0 || nx >= tileMapW) continue;

            for (int dy = -radius; dy <= radius; dy++) {
                int ny = cy + dy;
                if (ny < 0 || ny >= tileMapH) continue;

                if (dx * dx + dy * dy <= rSq) {
                    map[nx][ny] = std::max(map[nx][ny], value);
                }
            }
        }
    }
};
