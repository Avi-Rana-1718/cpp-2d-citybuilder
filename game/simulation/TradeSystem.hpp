#pragma once
#include <algorithm>
#include <string>
#include "../../src/_global.hpp"
#include "./EconomyData.hpp"
#include "../world/WorldData.hpp"
#include "../world/RoadNetwork.hpp"
#include "../entities/_Zone.hpp"
#include "../entities/ServiceZone.hpp"

// -------------------------------------------------------------
// Trade: goods and food, exported and imported
// -------------------------------------------------------------
// Once a day:
//   produced  goods: industrial workers (tier makes them more productive),
//                    miners (raw materials, half value); food: farm workers
//                    x soil x season
//   used      goods: residents and shop workers; food: residents
//   surplus   is exported for money, a shortfall imported at a premium,
//             both limited by the trade capacity: road tiles on the map
//             edge that the city's roads reach (TRADE_PER_EDGE_ROAD each,
//             x2 asphalt, x3 avenue) plus harbours (TRADE_PER_HARBOUR)
// Anything that can't be imported is a shortage (shops lose demand);
// anything that can't be exported piles up (industry loses demand).
struct TradeSystem {
    float goodsMade = 0.f, goodsUsed = 0.f, foodMade = 0.f, foodUsed = 0.f;
    float goodsExported = 0.f, goodsImported = 0.f, foodExported = 0.f, foodImported = 0.f;
    float exportIncome = 0.f, importCost = 0.f;
    float capacity = 0.f;          // units a day the city can ship in or out
    float shortage = 0.f;          // 0..1: share of needs nobody could deliver
    float glut = 0.f;              // 0..1: share of production nobody could ship

    void reset() { *this = TradeSystem(); }

    void runDay() {
        goodsMade = goodsUsed = foodMade = foodUsed = 0.f;
        int harbours = 0;
        for (int i = 0; i < tileMapW; i++)
            for (int j = 0; j < tileMapH; j++) {
                if (entities[i][j] == NULL) continue;
                Zone* z = dynamic_cast<Zone*>(entities[i][j]);
                if (z == NULL || z->isSubTile || !z->isActive) continue;
                const std::string n = z->getName();
                const float pop = static_cast<float>(z->population);
                if (n == "HarbourZone") harbours++;
                if (n == "FarmZone") foodMade += pop * FOOD_PER_FARMER * z->incomeMultiplier();
                else if (n == "MineZone") goodsMade += pop * GOODS_PER_WORKER[0] * 0.5f;
                else if (n == "IndustrialZone") goodsMade += pop * GOODS_PER_WORKER[std::max(0, std::min(2, z->tier - 1))];
                else if (n == "ResidentalZone") { goodsUsed += pop * GOODS_PER_RESIDENT; foodUsed += pop * FOOD_PER_RESIDENT; }
                else if (n == "CommericalZone") goodsUsed += pop * GOODS_PER_SHOP_WORKER;
            }
        // Each road leaving the map carries more the better it is (dirt 1x, asphalt 2x, avenue 3x)
        float edgeLanes = 0.f;
        for (int i = 0; i < tileMapW; i++)
            for (int j = 0; j < tileMapH; j++)
                if (onMapBorder(i, j) && roadNet.tileOutside(i, j))
                    if (RoadTile* r = RoadTile::at(i, j)) edgeLanes += static_cast<float>(r->tier + 1);
        capacity = edgeLanes * TRADE_PER_EDGE_ROAD + static_cast<float>(harbours) * TRADE_PER_HARBOUR;

        // Goods first (worth more), then food, sharing the capacity
        float room = capacity;
        auto settle = [&](float made, float used, float& exported, float& imported, float& unmet, float& unshipped) {
            exported = imported = unmet = unshipped = 0.f;
            if (made >= used) {
                exported = std::min(made - used, room);
                unshipped = made - used - exported;
            } else {
                imported = std::min(used - made, room);
                unmet = used - made - imported;
            }
            room -= exported + imported;
        };
        float goodsUnmet, goodsStuck, foodUnmet, foodStuck;
        settle(goodsMade, goodsUsed, goodsExported, goodsImported, goodsUnmet, goodsStuck);
        settle(foodMade, foodUsed, foodExported, foodImported, foodUnmet, foodStuck);
        exportIncome = goodsExported * GOODS_PRICE + foodExported * FOOD_PRICE;
        importCost = (goodsImported * GOODS_PRICE + foodImported * FOOD_PRICE) * IMPORT_MARKUP;
        const float need = goodsUsed + foodUsed;
        const float made = goodsMade + foodMade;
        shortage = need > 0.f ? std::min(1.f, (goodsUnmet + foodUnmet) / need) : 0.f;
        glut = made > 0.f ? std::min(1.f, (goodsStuck + foodStuck) / made) : 0.f;
    }
};

inline TradeSystem trade;
