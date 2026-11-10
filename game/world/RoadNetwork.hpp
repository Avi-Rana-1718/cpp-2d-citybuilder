#pragma once
#include <vector>
#include <cstdlib>
#include <SFML/System/Vector2.hpp>
#include "./WorldData.hpp"
#include "../entities/roads/RoadTile.hpp"

// -------------------------------------------------------------
// Road network: drag paths and connected components
// -------------------------------------------------------------

// Road drag between two tiles as an L of two edge-sharing legs: first
// along (+-1, 0) (SE / NW), then along (+-1, -+1) (NE / SW). Every step
// shares a diamond edge with the previous tile, so the result is always
// one connected road, and it always ends exactly on `end`.
//   end - start = a * (1, 0) + b * (1, -1)  ->  b = -dy, a = dx + dy
inline std::vector<sf::Vector2i> roadDragPath(sf::Vector2i start, sf::Vector2i end) {
    std::vector<sf::Vector2i> tiles;
    int dx = end.x - start.x, dy = end.y - start.y;
    int a = dx + dy, b = -dy;
    sf::Vector2i p = start;
    tiles.push_back(p);
    int sa = a >= 0 ? 1 : -1, sb = b >= 0 ? 1 : -1;
    for (int k = 0; k < std::abs(a); k++) { p.x += sa; tiles.push_back(p); }
    for (int k = 0; k < std::abs(b); k++) { p.x += sb; p.y -= sb; tiles.push_back(p); }
    return tiles;
}

inline bool onMapBorder(int x, int y) {
    return x == 0 || y == 0 || x == tileMapW - 1 || y == tileMapH - 1;
}

// Connected road components and whether each reaches the map border
// (the outside connection). Cheap (one BFS over 64x64), so it is simply
// rebuilt whenever roads change and at the start of every game day.
struct RoadNetwork {
    int comp[tileMapW][tileMapH];      // -1 = not a road
    std::vector<char> outside;         // per component
    std::vector<int> size;             // tiles per component

    RoadNetwork() { clear(); }

    void clear() {
        for (auto& col : comp)
            for (int& c : col) c = -1;
        outside.clear();
        size.clear();
    }

    void rebuild() {
        clear();
        std::vector<int> stack;
        stack.reserve(tileMapW * tileMapH);
        for (int i = 0; i < tileMapW; i++) {
            for (int j = 0; j < tileMapH; j++) {
                if (comp[i][j] >= 0 || RoadTile::at(i, j) == NULL) continue;
                int id = static_cast<int>(outside.size());
                outside.push_back(0);
                size.push_back(0);
                comp[i][j] = id;
                stack.push_back(i * tileMapH + j);
                while (!stack.empty()) {
                    int k = stack.back();
                    stack.pop_back();
                    int x = k / tileMapH, y = k % tileMapH;
                    size[id]++;
                    if (onMapBorder(x, y)) outside[id] = 1;
                    int m = RoadTile::linkMask(x, y);
                    for (int d = 0; d < 4; d++) {
                        if (!(m & (1 << d))) continue;
                        int nx = x + ROAD_DIR[d][0], ny = y + ROAD_DIR[d][1];
                        if (comp[nx][ny] >= 0) continue;
                        comp[nx][ny] = id;
                        stack.push_back(nx * tileMapH + ny);
                    }
                }
            }
        }
    }

    int componentAt(int x, int y) const { return inWorldBounds(x, y) ? comp[x][y] : -1; }
    bool tileOutside(int x, int y) const {
        int c = componentAt(x, y);
        return c >= 0 && outside[static_cast<size_t>(c)];
    }
    // A 2x2 building reaches the outside through any land road on its border
    bool buildingOutside(int rootX, int rootY) const {
        for (const auto& o : BORDER_2X2) {
            int x = rootX + o[0], y = rootY + o[1];
            if (roadTierOfName(tileKind(x, y)) >= 0 && tileOutside(x, y)) return true;
        }
        return false;
    }
};

inline RoadNetwork roadNet;
