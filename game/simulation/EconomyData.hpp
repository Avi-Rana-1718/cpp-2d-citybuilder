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
    {4.f, 12.f, 24.f},    // residents
    {5.5f, 16.f, 32.f},   // shop / office workers
    {5.f, 15.f, 30.f},    // industrial workers
};

// Daily running cost of public structures
const int UPKEEP_ROAD = 1;
const int UPKEEP_SCHOOL = 25;
const int UPKEEP_CHURCH = 20;
const int UPKEEP_PARK = 10;
const int UPKEEP_HARBOUR = 40;

// Road tiers: Dirt, Asphalt, Avenue (index = tier)
const int ROAD_TIER_COUNT = 3;
const int ROAD_COST[ROAD_TIER_COUNT] = {10, 30, 80};
const int ROAD_UPKEEP[ROAD_TIER_COUNT] = {UPKEEP_ROAD, 2, 5};
const int ROAD_CAPACITY[ROAD_TIER_COUNT] = {40, 120, 300};      // commuter trips per day
const float ROAD_SPEED[ROAD_TIER_COUNT] = {1.f, 1.5f, 2.f};
const int BRIDGE_COST_MULT = 5;     // bridge tile = 5x the tier's road cost
const int BRIDGE_UPKEEP_MULT = 3;
const int BRIDGE_MAX_SPAN = 8;      // longest run of water a bridge may cross
const int ROUNDABOUT_COST = 150;
const int UPKEEP_ROUNDABOUT = 2;    // on top of the road's own upkeep

// Utilities (power & water). Index = UtilityKind.
//   Power:  plants feed a grid; its transformers deliver it to the buildings on it
//   Water:  wells give clean water, pumps raw water that treatment plants
//           clean; towers store spare water. Wells and pumps drain the
//           groundwater reserve around them.
enum UtilityKind {
    UTIL_COAL, UTIL_WIND, UTIL_SOLAR, UTIL_TRANSFORMER,
    UTIL_WELL, UTIL_PUMP, UTIL_TREATMENT, UTIL_TOWER,
    UTIL_SEWAGE_OUTFLOW, UTIL_SEWAGE_PLANT,
    UTIL_KIND_COUNT
};
const int UTIL_COST[UTIL_KIND_COUNT]   = {1500, 400, 800, 300, 150, 600, 900, 350, 250, 1200};
const int UTIL_UPKEEP[UTIL_KIND_COUNT] = {20, 3, 8, 4, 1, 8, 12, 5, 2, 8};
// MW for plants, MW throughput for transformers, water units / day for
// wells and pumps (at 100% groundwater), treatment capacity, tower storage
// ... sewage outflow / treatment plant: sewage handled per day
const float UTIL_OUTPUT[UTIL_KIND_COUNT] = {400.f, 60.f, 120.f, 150.f, 80.f, 400.f, 400.f, 300.f, 300.f, 300.f};
const int SEWAGE_GRACE_POP = 400;         // smaller towns get by with septic tanks
const float COAL_DEPOSIT_BONUS = 1.25f;
const int WELL_RADIUS = 2;            // groundwater a well draws from
const int PUMP_RADIUS = 4;
const float GROUNDWATER_DRAIN = 0.05f;    // reserve points per water unit delivered (spread over the radius)
const float GROUNDWATER_RECHARGE = 0.15f; // reserve points per tile per day, towards its natural level
const int LINE_COST_POWER = 8;            // per tile
const int LINE_COST_PIPE = 6;
const int LINE_TILES_PER_UPKEEP = 10;     // $1 a day per 10 line tiles
const int UTILITY_GRACE_POP = 150;        // smaller towns need no utilities yet
// Daily use per occupant (resident / worker) by sector, x tier factor x city factor
const float POWER_PER_CAPITA[SECTOR_COUNT] = {0.25f, 0.5f, 0.8f};
const float WATER_PER_CAPITA[SECTOR_COUNT] = {0.2f, 0.1f, 0.3f};
const float UTIL_TIER_FACTOR[WEALTH_COUNT] = {1.f, 1.3f, 1.7f};
const float UTIL_CITY_SCALE = 3000.f;     // city factor = 1 + population / this

// City services (2x2, need a road). Index = ServiceKind.
//   Police / fire / clinic cover homes in a radius (safety, fires, health).
//   Landfills collect garbage from buildings on their road network.
//   Mines employ workers on a coal / iron / stone deposit and deplete it.
//   Bus depots take part of the commuters in their radius off the roads.
enum ServiceKind {
    SERVICE_POLICE, SERVICE_FIRE, SERVICE_CLINIC, SERVICE_LANDFILL, SERVICE_MINE, SERVICE_BUS_DEPOT,
    SERVICE_INCINERATOR, SERVICE_RECYCLING,
    SERVICE_KIND_COUNT
};
const int SERVICE_COST[SERVICE_KIND_COUNT]   = {400, 400, 500, 600, 800, 700, 1400, 1100};
const int SERVICE_UPKEEP[SERVICE_KIND_COUNT] = {12, 12, 15, 8, 0, 15, 15, 20};
const int SERVICE_RADIUS[SERVICE_KIND_COUNT] = {7, 7, 7, 0, 0, 8, 0, 0};
// Garbage disposal: garbage never just vanishes. Landfills store it until
// they are full; incinerators burn it (polluting, but it makes power);
// recycling centres process it cleanly.
const float LANDFILL_SPACE = 15000.f;      // a landfill's total capacity
const float LANDFILL_INTAKE = 300.f;       // garbage a landfill accepts a day
const float INCINERATOR_RATE = 200.f;      // garbage burnt a day
const float INCINERATOR_POWER = 0.5f;      // power units per garbage burnt
const float RECYCLING_RATE = 120.f;        // garbage recycled a day
const int BUSES_PER_DEPOT = 3;
const int SERVICE_MINE_JOBS = 30;
const float MINE_DEPLETION = 0.02f;        // deposit points per miner per day
const float GARBAGE_PER_CAPITA = 0.1f;     // per occupant per day
const int FIRE_GRACE_POP = 300;            // towns with fewer residents have no fires
const int FIRE_CHANCE = 4000;              // 1 in N per building per day
const int FIRE_SPREAD = 6;                 // 1 in N per day for each building within 2 tiles of a fire
const int FIRE_BURN_DAYS = 4;              // an unchecked fire destroys the building after this
const float BUS_RIDER_SHARE = 0.3f;        // commuters near a bus depot that leave the car at home
const int CRIME_POP = 500;                 // from here, homes without police feel unsafe
// Land value (0-100): upgrades need it
const int LAND_VALUE_TIER2 = 40;
const int LAND_VALUE_TIER3 = 60;

// Trade (TradeSystem): goods and food made / used per occupant a day, prices
const float GOODS_PER_WORKER[WEALTH_COUNT] = {1.0f, 1.4f, 2.0f};   // industry by tier
const float FOOD_PER_FARMER = 2.0f;        // x soil x season
const float GOODS_PER_RESIDENT = 0.1f;
const float GOODS_PER_SHOP_WORKER = 0.2f;
const float FOOD_PER_RESIDENT = 0.15f;
const float GOODS_PRICE = 0.5f;            // $ per unit exported
const float FOOD_PRICE = 0.35f;
const float IMPORT_MARKUP = 1.3f;          // imports cost 30% more than exports earn
const float TRADE_PER_EDGE_ROAD = 200.f;   // units a day through each dirt road leaving the map (x2 asphalt, x3 avenue)
const float TRADE_PER_HARBOUR = 500.f;

const int ZONE_COST_PER_TILE = 5;         // painting a zone
const int ZONE_GROWTH_DEMAND_STEP = 30;    // one extra building a day per 30 demand
const int ZONE_GROWTH_MAX_PER_DAY = 3;     // per sector

const int VARIANT_COUNT = 4;              // looks per R / C / I tier (v3 / v4 from tools/gen_variants.py)
const int STARTING_MONEY = 20000;
const int STARTER_SANDBOX_MONEY = 150000;   // --starter showcase town

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
