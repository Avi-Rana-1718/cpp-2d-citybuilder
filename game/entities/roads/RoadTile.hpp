#pragma once
#include "./_Road.hpp"
#include "../../world/WorldData.hpp"
#include "../../simulation/EconomyData.hpp"

// -------------------------------------------------------------
// Road tile: one 1x1 piece of the road network
// -------------------------------------------------------------
// Roads connect only across a shared diamond edge. The four edge
// directions and their mask bits:
//   NE (x+1, y-1) = 1   SE (x+1, y) = 2   SW (x-1, y+1) = 4   NW (x-1, y) = 8
// The mask picks one of 16 frames in the tier's sprite sheet
// (tools/gen_road_sprites.py). Bridges carry the road across water
// along one axis only: axis 0 = NE-SW, axis 1 = SE-NW.

const int ROAD_DIR[4][2] = {{1, -1}, {1, 0}, {-1, 1}, {-1, 0}};
inline int roadDirAxis(int d) { return d % 2; }

inline const char* roadSheetKey(int tier) {
    return tier == 2 ? "road_avenue" : (tier == 1 ? "road_asphalt" : "road_dirt");
}

class RoadTile : public Road {
public:
    int tier = 0;          // 0 Dirt, 1 Asphalt, 2 Avenue
    int bridgeAxis = -1;   // -1 = on land
    bool roundabout = false;
    int mask = 0;          // connected edges, see ROAD_DIR

    RoadTile(int posX, int posY, int tier, int bridgeAxis = -1);

    bool isBridge() const { return bridgeAxis >= 0; }
    bool allowsDir(int d) const { return bridgeAxis < 0 || roadDirAxis(d) == bridgeAxis; }

    // Trips per day before congestion (junction effects are applied by the traffic system)
    int capacity() const { return ROAD_CAPACITY[tier]; }
    int dailyUpkeep() const override {
        int u = ROAD_UPKEEP[tier];
        if (isBridge()) u *= BRIDGE_UPKEEP_MULT;
        if (roundabout) u += UPKEEP_ROUNDABOUT;
        return u;
    }

    void setTier(int t);
    void update(float deltaTime) override;
    void check();                   // recompute mask and sprite frame
    int frame() const { return sprite ? sprite->getTextureRect().left / 32 : -1; }

    static RoadTile* at(int x, int y) {
        if (!inWorldBounds(x, y) || entities[x][y] == NULL) return NULL;
        return dynamic_cast<RoadTile*>(entities[x][y]);
    }
    // Edges of (x, y) that join another road tile both sides agree on
    static int linkMask(int x, int y) {
        RoadTile* r = at(x, y);
        if (r == NULL) return 0;
        int m = 0;
        for (int d = 0; d < 4; d++) {
            RoadTile* n = at(x + ROAD_DIR[d][0], y + ROAD_DIR[d][1]);
            if (n != NULL && r->allowsDir(d) && n->allowsDir(d)) m |= 1 << d;
        }
        return m;
    }
};

inline RoadTile::RoadTile(int posX, int posY, int t, int axis) : Road(posX, posY) {
    tier = std::max(0, std::min(ROAD_TIER_COUNT - 1, t));
    bridgeAxis = axis;
    setName(isBridge() ? "Bridge" : roadTierName(tier));

    delete shape;
    shape = NULL;
    delete text;
    text = NULL;

    position->x = tileSizeX * posX - posX * (tileSizeX / 2);
    position->y = tileSizeY * posY + posX * (tileSizeY / 2);

    sprite = new sf::Sprite;
    sprite->setScale(sf::Vector2f(2, 2));
    check();
}

inline void RoadTile::setTier(int t) {
    tier = std::max(0, std::min(ROAD_TIER_COUNT - 1, t));
    if (!isBridge()) setName(roadTierName(tier));
    check();
}

// Roads have no per-frame logic: they are drawn by the culled ground pass
// and re-tiled only when a neighbour changes (Play_Scene::refreshRoadsAround).
inline void RoadTile::update(float deltaTime) {
    setDeltaTime(deltaTime);
}

inline void RoadTile::check() {
    mask = linkMask(posX, posY);
    if (sprite == NULL) return;
    if (isBridge()) {
        // Deck sheet: 32x24 frames, (tier * 2 + axis); always a straight piece
        sprite->setTexture(m_textures["road_bridge"]);
        sprite->setTextureRect(sf::IntRect((tier * 2 + bridgeAxis) * 32, 0, 32, 24));
    } else {
        sprite->setTexture(m_textures[roundabout ? "roundabout" : roadSheetKey(tier)]);
        sprite->setTextureRect(sf::IntRect(mask * 32, 0, 32, 16));
    }
}

// Tier shorthands (tests and older code build roads with these)
class DirtRoad : public RoadTile {
public:
    DirtRoad(int posX, int posY) : RoadTile(posX, posY, 0) {}
};
class AsphaltRoad : public RoadTile {
public:
    AsphaltRoad(int posX, int posY) : RoadTile(posX, posY, 1) {}
};
class Avenue : public RoadTile {
public:
    Avenue(int posX, int posY) : RoadTile(posX, posY, 2) {}
};
