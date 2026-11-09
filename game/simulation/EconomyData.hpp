#pragma once
#include <string>

// -------------------------------------------------------------
// Economy data shared by buildings and the economy system
// -------------------------------------------------------------
// Kept free of entity includes so every building can use it.
//
// Every R/C/I building belongs to a (sector, wealth) bucket:
//   sector: 0 Residential, 1 Commercial, 2 Industrial
//   wealth: 0 low ($), 1 medium ($$), 2 high ($$$) = building tier - 1
// Farms are low-wealth industry, harbours are medium-wealth commerce.

enum Sector { SECTOR_R = 0, SECTOR_C = 1, SECTOR_I = 2, SECTOR_COUNT = 3 };
enum Wealth { WEALTH_LOW = 0, WEALTH_MID = 1, WEALTH_HIGH = 2, WEALTH_COUNT = 3 };

const int TAX_MIN = 0;
const int TAX_MAX = 20;
const int TAX_DEFAULT = 9;
const int TAX_NEUTRAL = 7; // demand is unaffected by tax at this rate

// Tax slider per (sector, wealth), in percent
inline int taxRates[SECTOR_COUNT][WEALTH_COUNT] = {
    {TAX_DEFAULT, TAX_DEFAULT, TAX_DEFAULT},
    {TAX_DEFAULT, TAX_DEFAULT, TAX_DEFAULT},
    {TAX_DEFAULT, TAX_DEFAULT, TAX_DEFAULT},
};

inline void resetTaxRates() {
    for (auto& row : taxRates)
        for (int& r : row) r = TAX_DEFAULT;
}

// Gross daily income per occupant (resident or worker) before tax
const float INCOME_PER_CAPITA[SECTOR_COUNT][WEALTH_COUNT] = {
    {3.f, 6.f, 12.f},    // residents
    {4.f, 8.f, 16.f},    // shop / office workers
    {3.5f, 7.f, 14.f},   // industrial workers
};

// Daily running cost of public structures
const int UPKEEP_ROAD = 1;
const int UPKEEP_SCHOOL = 25;
const int UPKEEP_CHURCH = 20;
const int UPKEEP_PARK = 10;
const int UPKEEP_HARBOUR = 40;

const int STARTING_MONEY = 20000;

// Loans and bankruptcy
const int LOAN_AMOUNT = 10000;
const float LOAN_INTEREST = 0.10f;   // total interest over the term
const int LOAN_TERM_DAYS = 360;
const int LOAN_MAX_ACTIVE = 3;
const int DEBT_LIMIT_DAYS = 180;     // 6 months of 30 days in debt = bankruptcy

inline std::string sectorLetter(int s) {
    return s == SECTOR_R ? "R" : (s == SECTOR_C ? "C" : "I");
}
inline std::string wealthSigns(int w) {
    return w == WEALTH_LOW ? "$" : (w == WEALTH_MID ? "$$" : "$$$");
}
inline std::string sectorName(int s) {
    return s == SECTOR_R ? "Residential" : (s == SECTOR_C ? "Commercial" : "Industrial");
}
inline std::string wealthName(int w) {
    return w == WEALTH_LOW ? "Low income" : (w == WEALTH_MID ? "Medium income" : "High income");
}
