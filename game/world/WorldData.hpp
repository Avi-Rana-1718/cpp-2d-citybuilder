#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include "../../src/_global.hpp"

// -------------------------------------------------------------
// World data layer
// -------------------------------------------------------------
// Parallel to the entities[][] grid. entities[][] holds what is visibly
// standing on a tile; world[][] holds the natural terrain underneath it
// plus the soil and underground resources. Demolishing a structure
// restores the tile from world[][].terrain.

enum class Terrain : uint8_t {
    Grass,
    Sand,
    Water,
    Forest,
    Rock,
    ForestPath
};

enum class Deposit : uint8_t {
    None,
    Coal,
    Iron,
    Stone
};

struct TileData {
    Terrain terrain = Terrain::Grass;
    uint8_t fertility = 50;      // 0-100, surface soil quality (always visible)
    uint8_t groundwater = 0;     // 0-100, underground water table
    Deposit deposit = Deposit::None;
    uint8_t depositAmount = 0;   // 0-100, richness of the deposit
};

inline TileData world[tileMapW][tileMapH];
inline uint32_t worldSeed = 0;   // 0 = pick a random seed at scene init
inline int worldSpawnX = tileMapW / 2;
inline int worldSpawnY = tileMapH / 2;

inline bool inWorldBounds(int x, int y) {
    return x >= 0 && x < tileMapW && y >= 0 && y < tileMapH;
}

// Top-left of a tile's 64x32 isometric diamond in world space: X = 32x, Y = 32y + 16x
inline sf::Vector2f tileScreenPos(int x, int y) {
    return sf::Vector2f(static_cast<float>(tileSizeX * x - x * (tileSizeX / 2)),
                        static_cast<float>(tileSizeY * y + x * (tileSizeY / 2)));
}

// -------------------------------------------------------------
// Visible tile range
// -------------------------------------------------------------
// Inverse of tileScreenPos for a world-space view rectangle. A tile at
// (i, j) is visible when its 64x32 diamond box, extended upward by
// tallMargin for buildings/trees that rise above the tile, overlaps the
// rectangle. Since X = 32i and Y = 32j + 16i, the visible tiles form a
// slanted band: a fixed i range, and a j range that shifts with i.

struct VisibleTiles {
    int iMin = 0, iMax = -1;
    // Per-column j bounds, computed once per frame (index = i)
    int jMinCol[tileMapW] = {};
    int jMaxCol[tileMapW] = {};

    int jMin(int i) const { return jMinCol[i]; }
    int jMax(int i) const { return jMaxCol[i]; }
    bool contains(int i, int j) const {
        return i >= iMin && i <= iMax && j >= jMinCol[i] && j <= jMaxCol[i];
    }
    // Smallest / largest j over the whole i range (both bounds fall as i grows)
    int jLowest() const { return iMax >= iMin ? jMinCol[iMax] : 0; }
    int jHighest() const { return iMax >= iMin ? jMaxCol[iMin] : -1; }
};

inline VisibleTiles visibleTileRange(float viewL, float viewR, float viewT, float viewB, float tallMargin) {
    VisibleTiles v;
    v.iMin = std::max(0, static_cast<int>(std::floor((viewL - static_cast<float>(tileSizeX)) / 32.f)));
    v.iMax = std::min(tileMapW - 1, static_cast<int>(std::ceil(viewR / 32.f)));
    for (int i = v.iMin; i <= v.iMax; i++) {
        float top = viewT - tallMargin - static_cast<float>(tileSizeY) - 16.f * static_cast<float>(i);
        float bot = viewB - 16.f * static_cast<float>(i);
        v.jMinCol[i] = std::max(0, static_cast<int>(std::floor(top / 32.f)));
        v.jMaxCol[i] = std::min(tileMapH - 1, static_cast<int>(std::ceil(bot / 32.f)));
    }
    return v;
}

// -------------------------------------------------------------
// Natural ground vs structures
// -------------------------------------------------------------
// entities[x][y] holds only structures (buildings, roads). An empty
// slot (nullptr) means natural ground, described by world[x][y].terrain.
// Placement rules use tile "kind" names so both cases read the same.

inline const char* groundKindName(Terrain t) {
    switch (t) {
        case Terrain::Sand:       return "Sand";
        case Terrain::Water:      return "Water";
        case Terrain::Forest:     return "Forest";
        case Terrain::Rock:       return "Rock";
        case Terrain::ForestPath: return "ForestPath";
        case Terrain::Grass:
        default:                  return "Grass";
    }
}

// Structure name if something is built here, otherwise the natural ground kind
inline std::string tileKind(int x, int y) {
    if (!inWorldBounds(x, y)) return "";
    if (entities[x][y] != NULL) return entities[x][y]->getName();
    return groundKindName(world[x][y].terrain);
}

// Ground a road or building may be placed on directly
inline bool isClearGroundName(const std::string& kind) {
    return kind == "Grass" || kind == "Sand" || kind == "ForestPath";
}

// Natural tiles that must be bulldozed (or cannot be built on)
inline bool isNaturalObstacleName(const std::string& kind) {
    return kind == "Forest" || kind == "Rock" || kind == "Water";
}

// -------------------------------------------------------------
// 2x2 building footprint and its border
// -------------------------------------------------------------
// A 2x2 building rooted at (x, y) covers the rhombus below. In this
// projection a tile shares a full diamond edge with its E/W (+-1, 0)
// and NE/SW (+1,-1)/(-1,+1) neighbours; (0, +-1) only touch at a tip.
// BORDER_2X2 is every tile that shares an edge with the footprint.

const int FOOTPRINT_2X2[4][2] = {{0, 0}, {1, 0}, {-1, 1}, {0, 1}};
const int BORDER_2X2[8][2] = {
    {-1, 0}, {1, -1}, {2, -1}, {2, 0},   // north-west and north-east sides
    {-2, 1}, {-2, 2}, {-1, 2}, {1, 1}    // south-west and south-east sides
};

// True if any border tile of the building rooted at (x, y) has this kind
inline bool borderHasKind(int x, int y, const char* kind) {
    for (const auto& o : BORDER_2X2) {
        if (tileKind(x + o[0], y + o[1]) == kind) return true;
    }
    return false;
}

inline bool hasAdjacentRoad(int x, int y)  { return borderHasKind(x, y, "DirtRoad"); }
inline bool hasAdjacentWater(int x, int y) { return borderHasKind(x, y, "Water"); }

// Removes every structure; the map is then pure natural ground from world[][]
inline void materializeWorld() {
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            delete entities[i][j];
            entities[i][j] = NULL;
        }
    }
}

inline void resetWorldData() {
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            world[i][j] = TileData();
        }
    }
    worldSpawnX = tileMapW / 2;
    worldSpawnY = tileMapH / 2;
}

inline bool isLandTerrain(Terrain t) {
    return t != Terrain::Water;
}

// Terrain a 2x2 building or road can be placed on directly
inline bool isBuildableTerrain(Terrain t) {
    return t == Terrain::Grass || t == Terrain::Sand || t == Terrain::ForestPath;
}

inline std::string terrainName(Terrain t) {
    switch (t) {
        case Terrain::Grass:      return "Grassland";
        case Terrain::Sand:       return "Sandy Beach";
        case Terrain::Water:      return "Water";
        case Terrain::Forest:     return "Forest";
        case Terrain::Rock:       return "Rocky Hills";
        case Terrain::ForestPath: return "Forest Path";
    }
    return "Unknown";
}

inline std::string depositName(Deposit d) {
    switch (d) {
        case Deposit::None:  return "None";
        case Deposit::Coal:  return "Coal";
        case Deposit::Iron:  return "Iron Ore";
        case Deposit::Stone: return "Stone";
    }
    return "None";
}

// Average soil fertility under a true 2x2 isometric diamond footprint rooted at (x, y)
inline int footprintFertility(int x, int y) {
    const int offs[4][2] = {{0, 0}, {1, 0}, {-1, 1}, {0, 1}};
    int sum = 0;
    int n = 0;
    for (int k = 0; k < 4; k++) {
        int cx = x + offs[k][0];
        int cy = y + offs[k][1];
        if (!inWorldBounds(cx, cy)) continue;
        sum += world[cx][cy].fertility;
        n++;
    }
    return n > 0 ? sum / n : 50;
}

// Farm output multiplier from soil: 0 -> 0.4x, 50 -> 1.0x, 100 -> 1.6x
inline float fertilityMultiplier(int fertility) {
    return 0.4f + 0.012f * static_cast<float>(fertility);
}
