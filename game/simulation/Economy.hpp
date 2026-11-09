#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "../../src/_global.hpp"
#include "./EconomyData.hpp"
#include "./HappinessSystem.hpp"
#include "../entities/ResidentalZone.hpp"
#include "../entities/CommericalZone.hpp"
#include "../entities/IndustrialZone.hpp"

// -------------------------------------------------------------
// City economy: demand, occupancy, taxes, upkeep, loans, debt
// -------------------------------------------------------------
// Runs once per game day (Play_Scene::runNewDay):
//   1. scan     - occupants and capacity per (sector, wealth) bucket
//   2. demand   - 9 values in -100..100 from jobs / customers / workers,
//                 adjusted for tax, happiness, education and pollution
//   3. occupy   - road-connected buildings fill on positive demand,
//                 empty on negative demand; full buildings may upgrade
//                 when the next income level has demand
//   4. settle   - taxes in, upkeep and loan payments out, debt clock

struct Loan {
    int remainingDays = LOAN_TERM_DAYS;
    float dailyPayment = 0.f;
};

struct DayReport {
    float tax[SECTOR_COUNT][WEALTH_COUNT] = {};
    float taxTotal = 0.f;
    int upkeepRoads = 0, upkeepSchools = 0, upkeepChurches = 0, upkeepParks = 0, upkeepHarbours = 0;
    int roadTiles = 0;
    float loanPayments = 0.f;
    float expenseTotal = 0.f;
    int net = 0; // whole dollars credited (negative = money left the treasury)
};

class EconomySystem {
public:
    float demand[SECTOR_COUNT][WEALTH_COUNT] = {};
    int occupants[SECTOR_COUNT][WEALTH_COUNT] = {};
    int capacity[SECTOR_COUNT][WEALTH_COUNT] = {};

    // Inputs to demand, kept for the budget panel
    float avgEducation = 0.f;   // school coverage over homes, 0..100
    float avgPollution = 0.f;   // industrial pollution over homes, 0..60
    int happiness = 50;
    int activeHarbours = 0;

    std::vector<Loan> loans;
    int daysInDebt = 0;
    bool bankrupt = false;
    DayReport lastDay;
    long dayCounter = 0;

private:
    float carry = 0.f;  // fractional dollars carried between days

    static float clampDemand(float v) { return std::max(-100.f, std::min(100.f, v)); }

    // Normalised gap: +100 when supply is all missing, -100 when nothing wants it
    static float gap(float want, float have, float floor) {
        return 100.f * (want - have) / std::max(floor, want + have);
    }

    // Deterministic "1 in n" per building and day (no std::rand, so tests are stable)
    bool chance(int x, int y, int n) const {
        unsigned h = static_cast<unsigned>(x) * 73856093u ^ static_cast<unsigned>(y) * 19349663u ^
                     static_cast<unsigned>(dayCounter) * 83492791u;
        return (h % static_cast<unsigned>(n)) == 0;
    }

    template <typename Fn>
    void forEachBuilding(Fn fn) {
        for (int i = 0; i < tileMapW; i++) {
            for (int j = 0; j < tileMapH; j++) {
                if (entities[i][j] == NULL) continue;
                Zone* z = dynamic_cast<Zone*>(entities[i][j]);
                if (z != NULL && !z->isSubTile) fn(z, i, j);
            }
        }
    }

public:
    void reset() {
        *this = EconomySystem();
    }

    // ---- 1. scan ----
    void scan() {
        for (int s = 0; s < SECTOR_COUNT; s++)
            for (int w = 0; w < WEALTH_COUNT; w++) occupants[s][w] = capacity[s][w] = 0;
        activeHarbours = 0;

        forEachBuilding([&](Zone* z, int, int) {
            int s = z->sector();
            if (s < 0) return;
            int w = z->wealth();
            capacity[s][w] += z->capacity;
            occupants[s][w] += z->population;
            if (z->getName() == "HarbourZone" && z->isActive) activeHarbours++;
        });

        for (int s = 0; s < SECTOR_COUNT; s++) {
            townPopulation[s] = 0;
            townCapacity[s] = 0;
            for (int w = 0; w < WEALTH_COUNT; w++) {
                townPopulation[s] += occupants[s][w];
                townCapacity[s] += capacity[s][w];
            }
        }
    }

    // Education / pollution averaged over homes (from the happiness coverage maps)
    void sampleServices(const HappinessSystem& hs) {
        long edu = 0, pol = 0;
        int homes = 0;
        forEachBuilding([&](Zone* z, int i, int j) {
            if (z->sector() != SECTOR_R) return;
            edu += hs.educationMap[i][j];
            pol += hs.pollutionMap[i][j];
            homes++;
        });
        avgEducation = homes > 0 ? static_cast<float>(edu) / static_cast<float>(homes) : 0.f;
        avgPollution = homes > 0 ? static_cast<float>(pol) / static_cast<float>(homes) : 0.f;
        happiness = hs.cityHappiness;
    }

    // ---- 2. demand ----
    void computeDemand() {
        const float RES_BASE[3] = {35.f, 15.f, 5.f};
        const float COM_BASE[3] = {10.f, 5.f, 0.f};
        const float IND_BASE[3] = {20.f, 8.f, 0.f};
        const float TAX_SENS[3] = {2.5f, 3.5f, 5.f};      // demand lost per % above neutral
        const float HAPPY_SENS[3] = {0.4f, 0.7f, 1.0f};   // per point of happiness away from 55
        const float POLL_SENS[3] = {0.2f, 0.5f, 1.0f};    // homes only

        float exportBoost = std::min(30.f, 10.f * static_cast<float>(activeHarbours));

        for (int w = 0; w < WEALTH_COUNT; w++) {
            float residents = static_cast<float>(occupants[SECTOR_R][w]);
            float workers = residents * 0.6f;
            float jobs = static_cast<float>(capacity[SECTOR_C][w] + capacity[SECTOR_I][w]);

            // Residents come for open positions (built job capacity);
            // businesses hire until their customers / workforce share is met,
            // so surplus job buildings stay empty instead of blocking everyone.
            float r = RES_BASE[w] + gap(jobs, workers, 60.f);
            float c = COM_BASE[w] + gap(residents * 0.25f, static_cast<float>(occupants[SECTOR_C][w]), 40.f);
            float i = IND_BASE[w] + gap(workers * 0.5f, static_cast<float>(occupants[SECTOR_I][w]), 40.f);
            if (w <= WEALTH_MID) i += exportBoost;

            // Tax: each sector reacts to its own slider
            r -= (static_cast<float>(taxRates[SECTOR_R][w]) - TAX_NEUTRAL) * TAX_SENS[w];
            c -= (static_cast<float>(taxRates[SECTOR_C][w]) - TAX_NEUTRAL) * TAX_SENS[w];
            i -= (static_cast<float>(taxRates[SECTOR_I][w]) - TAX_NEUTRAL) * TAX_SENS[w];

            // Happiness: residents feel it fully, businesses half
            float mood = (static_cast<float>(happiness) - 55.f) * HAPPY_SENS[w];
            r += mood;
            c += mood * 0.5f;
            i += mood * 0.5f;

            // Pollution near homes
            r -= avgPollution * POLL_SENS[w];

            demand[SECTOR_R][w] = r;
            demand[SECTOR_C][w] = c;
            demand[SECTOR_I][w] = i;
        }

        // Education: medium and high earners, and high-tech industry
        demand[SECTOR_R][WEALTH_MID] -= std::max(0.f, 40.f - avgEducation) * 0.8f;
        demand[SECTOR_R][WEALTH_HIGH] -= std::max(0.f, 70.f - avgEducation) * 1.0f;
        demand[SECTOR_I][WEALTH_HIGH] -= std::max(0.f, 60.f - avgEducation) * 0.8f;

        for (int s = 0; s < SECTOR_COUNT; s++) {
            float sum = 0.f;
            for (int w = 0; w < WEALTH_COUNT; w++) {
                demand[s][w] = clampDemand(demand[s][w]);
                sum += demand[s][w];
            }
            townDemand[s] = static_cast<int>(sum / 3.f);
        }
    }

    // Main reason a bucket's demand is held back (for the budget panel)
    std::string limitingFactor(int s, int w) const {
        if (demand[s][w] > 25.f) return "Strong demand";
        if (taxRates[s][w] >= 13) return "Taxes too high";
        if (s == SECTOR_R && w == WEALTH_HIGH && avgEducation < 70.f) return "Needs more schools";
        if (s == SECTOR_R && w == WEALTH_MID && avgEducation < 40.f) return "Needs schools";
        if (s == SECTOR_I && w == WEALTH_HIGH && avgEducation < 60.f) return "Needs educated workers";
        if (s == SECTOR_R && avgPollution > 25.f && w > WEALTH_LOW) return "Too much pollution";
        if (happiness < 45) return "Citizens unhappy";
        if (s == SECTOR_R) return "Not enough jobs";
        if (s == SECTOR_C) return "Not enough customers";
        return "Not enough workers";
    }

    // ---- 3. occupy ----
    void updateOccupancy() {
        forEachBuilding([&](Zone* z, int x, int y) {
            int s = z->sector();
            if (s < 0 || z->capacity <= 0) return;
            int cap = z->capacity;

            if (!z->isActive) {
                // No road: people leave
                z->population = std::max(0, z->population - std::max(1, cap / 5));
                return;
            }

            float d = demand[s][z->wealth()];
            if (d > 0.f) {
                int in = std::max(1, static_cast<int>(std::round(cap * (0.04f + 0.12f * d / 100.f))));
                z->population = std::min(cap, z->population + in);
            } else if (d < -15.f) {
                int out = std::max(1, static_cast<int>(std::round(cap * 0.08f * (-d) / 100.f)));
                z->population = std::max(0, z->population - out);
            }

            changeTier(z, x, y);
            z->population = std::min(z->population, z->capacity);
        });
    }

    // Full, liked buildings move up an income level when that level has demand;
    // emptying buildings in a collapsing bucket move down.
    void changeTier(Zone* z, int x, int y) {
        int s = z->sector();
        if (s < 0 || z->getName() == "FarmZone" || z->getName() == "HarbourZone") return;
        int tier = z->tier;
        bool full = z->population >= static_cast<int>(0.9f * static_cast<float>(z->capacity));
        int liked = (s == SECTOR_R) ? z->overallHappiness : happiness;

        if (tier < 3 && full && demand[s][tier] > 25.f && liked >= (tier == 1 ? 60 : 75) && chance(x, y, 5)) {
            if (auto* r = dynamic_cast<ResidentalZone*>(z)) r->upgradeTier(tier + 1);
            else if (auto* c = dynamic_cast<CommericalZone*>(z)) c->upgradeTier(tier + 1);
            else if (auto* i = dynamic_cast<IndustrialZone*>(z)) i->upgradeTier(tier + 1);
        } else if (tier > 1 && demand[s][tier - 1] < -40.f &&
                   z->population < static_cast<int>(0.4f * static_cast<float>(z->capacity)) && chance(x, y, 5)) {
            if (auto* r = dynamic_cast<ResidentalZone*>(z)) r->downgradeTier(tier - 1);
            else if (auto* c = dynamic_cast<CommericalZone*>(z)) c->downgradeTier(tier - 1);
            else if (auto* i = dynamic_cast<IndustrialZone*>(z)) i->downgradeTier(tier - 1);
        }
    }

    // ---- 4. settle ----
    DayReport settle() {
        DayReport rep;
        forEachBuilding([&](Zone* z, int, int) {
            int s = z->sector();
            if (s >= 0) {
                float t = z->dailyTax();
                rep.tax[s][z->wealth()] += t;
                rep.taxTotal += t;
            }
            int u = z->dailyUpkeep();
            if (u <= 0) return;
            const std::string name = z->getName();
            if (name == "DirtRoad") { rep.upkeepRoads += u; rep.roadTiles++; }
            else if (name == "SchoolZone") rep.upkeepSchools += u;
            else if (name == "ChurchZone") rep.upkeepChurches += u;
            else if (name == "ParkZone") rep.upkeepParks += u;
            else if (name == "HarbourZone") rep.upkeepHarbours += u;
        });

        for (auto& loan : loans) {
            rep.loanPayments += loan.dailyPayment;
            loan.remainingDays--;
        }
        loans.erase(std::remove_if(loans.begin(), loans.end(),
                                   [](const Loan& l) { return l.remainingDays <= 0; }), loans.end());

        rep.expenseTotal = static_cast<float>(rep.upkeepRoads + rep.upkeepSchools + rep.upkeepChurches +
                                              rep.upkeepParks + rep.upkeepHarbours) + rep.loanPayments;

        float total = rep.taxTotal - rep.expenseTotal + carry;
        int whole = static_cast<int>(std::floor(total));
        carry = total - static_cast<float>(whole);
        money += whole;
        rep.net = whole;

        // Debt clock: 180 consecutive days below zero = bankruptcy
        if (money < 0) {
            daysInDebt++;
            if (daysInDebt >= DEBT_LIMIT_DAYS) bankrupt = true;
        } else {
            daysInDebt = 0;
        }

        lastDay = rep;
        return rep;
    }

    // One game day
    DayReport runDay(const HappinessSystem& hs) {
        dayCounter++;
        scan();
        sampleServices(hs);
        computeDemand();
        updateOccupancy();
        scan();
        return settle();
    }

    // ---- loans ----
    bool canTakeLoan() const { return static_cast<int>(loans.size()) < LOAN_MAX_ACTIVE && !bankrupt; }

    bool takeLoan() {
        if (!canTakeLoan()) return false;
        Loan l;
        l.remainingDays = LOAN_TERM_DAYS;
        l.dailyPayment = static_cast<float>(LOAN_AMOUNT) * (1.f + LOAN_INTEREST) / static_cast<float>(LOAN_TERM_DAYS);
        loans.push_back(l);
        money += LOAN_AMOUNT;
        return true;
    }

    float loanBalance() const {
        float b = 0.f;
        for (const auto& l : loans) b += l.dailyPayment * static_cast<float>(l.remainingDays);
        return b;
    }

    float dailyLoanPayments() const {
        float p = 0.f;
        for (const auto& l : loans) p += l.dailyPayment;
        return p;
    }

    int daysUntilBankrupt() const { return std::max(0, DEBT_LIMIT_DAYS - daysInDebt); }
};
