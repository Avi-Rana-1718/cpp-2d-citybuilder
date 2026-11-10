#pragma once
// Play_Scene member definitions (included at the end of Play_Scene.hpp).
// Scripted towns: --bench-city, --bench-town, --starter and their placement helpers

// Fills the map with a dense grid city for benchmarking (--bench-city).
// Road rows run east-west every 3 rows; each 2x2 building sits on the row
// below a road so the road shares its north-east edge. Natural obstacles
// in the way are cleared first. Returns the number of buildings placed.
int Play_Scene::buildBenchCity() {
    const int savedMoney = money;
    money = 100000000;

    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            if (isNaturalObstacleName(tileKind(i, j))) world[i][j].terrain = Terrain::Grass;
        }
    }
    ground.markDirty();

    for (int y = 1; y + 2 < tileMapH; y += 3) {
        for (int x = 0; x < tileMapW; x++) placeTile(x, y, TOOL_ROAD);
    }

    const BuildTool types[6] = {TOOL_RES_COTTAGES, TOOL_COM_GROCERY, TOOL_IND_WORKSHOP,
                                TOOL_RES_APARTMENTS, TOOL_FARM_BARN, TOOL_SCHOOL};
    int built = 0;
    int k = 0;
    for (int y = 1; y + 2 < tileMapH; y += 3) {
        for (int x = 1; x + 1 < tileMapW; x += 2) {
            BuildTool t = types[k++ % 6];
            if (canPlaceTile(x, y + 1, t)) {
                placeTile(x, y + 1, t);
                built++;
            }
        }
    }

    money = savedMoney;
    return built;
}

// A small starter town around the spawn (--bench-town): 2 road rows of
// ~19 tiles with ~16 buildings (mostly homes, a few shops / workshops, a
// farm, a school and a park). Used to check early-game economy balance.
int Play_Scene::buildBenchTown() {
    const int savedMoney = money;
    money = 100000000;
    int x0 = std::max(2, worldSpawnX - 9), x1 = std::min(tileMapW - 3, worldSpawnX + 9);
    int y0 = std::max(1, worldSpawnY - 4);
    for (int i = x0 - 2; i <= x1 + 2; i++)
        for (int j = y0; j <= y0 + 6; j++)
            if (inWorldBounds(i, j) && isNaturalObstacleName(tileKind(i, j))) world[i][j].terrain = Terrain::Grass;
    ground.markDirty();

    for (int y = y0; y <= y0 + 3; y += 3)
        for (int x = x0; x <= x1; x++) placeTile(x, y, TOOL_ROAD);
    connectBenchRoads(x0, x1, y0);

    const BuildTool plan[] = {
        TOOL_RES_COTTAGES, TOOL_RES_COTTAGES, TOOL_COM_GROCERY, TOOL_RES_COTTAGES, TOOL_IND_WORKSHOP,
        TOOL_RES_COTTAGES, TOOL_RES_COTTAGES, TOOL_SCHOOL, TOOL_RES_COTTAGES, TOOL_COM_GROCERY,
        TOOL_RES_COTTAGES, TOOL_IND_WORKSHOP, TOOL_RES_COTTAGES, TOOL_FARM_BARN, TOOL_RES_COTTAGES,
        TOOL_PARK, TOOL_RES_COTTAGES, TOOL_COM_GROCERY, TOOL_RES_COTTAGES, TOOL_IND_WORKSHOP};
    const int planSize = static_cast<int>(sizeof(plan) / sizeof(plan[0]));
    int built = 0, k = 0;
    for (int y = y0; y <= y0 + 3 && k < planSize; y += 3) {
        for (int x = x0 + 1; x + 1 <= x1 && k < planSize; x += 2) {
            if (canPlaceTile(x, y + 1, plan[k])) { placeTile(x, y + 1, plan[k]); built++; }
            k++;
        }
    }
    if (!starterCity) {
        // Enough power and water for the town to pass UTILITY_GRACE_POP
        for (BuildTool t : {TOOL_TRANSFORMER, TOOL_POWER_WIND, TOOL_POWER_WIND, TOOL_POWER_WIND,
                            TOOL_WATER_WELL, TOOL_WATER_WELL, TOOL_WATER_WELL, TOOL_WATER_WELL, TOOL_LANDFILL})
            placeUtilityNear(t, x0, x1, y0);
    }
    money = savedMoney;
    return built;
}

// Bench town roads: joins the two rows east of the town with a NE-SW link,
// then runs a road out to the closest map edge (cheapest of four straight
// routes, clearing forest / rock and bridging short water on the way).
void Play_Scene::connectBenchRoads(int x0, int x1, int y0) {
    int ex = std::min(tileMapW - 1, x1 + 3);
    std::vector<sf::Vector2i> link;
    for (int x = x1 + 1; x <= ex; x++) link.push_back({x, y0});
    for (int k = 1; k <= 3 && x1 + 3 - k >= 0; k++) link.push_back({x1 + 3 - k, y0 + k});
    for (const auto& p : link)
        if (inWorldBounds(p.x, p.y) && isNaturalObstacleName(tileKind(p.x, p.y)) && tileKind(p.x, p.y) != "Water")
            world[p.x][p.y].terrain = Terrain::Grass;
    ground.markDirty();
    buildRoadPath(link, TOOL_ROAD);

    // Candidate straight routes to an edge: west, east, north-east, south-west
    std::vector<std::vector<sf::Vector2i>> routes(4);
    for (int x = x0; x >= 0; x--) routes[0].push_back({x, y0});
    for (int x = ex; x < tileMapW; x++) routes[1].push_back({x, y0});
    for (int x = ex, y = y0; x < tileMapW && y >= 0; x++, y--) routes[2].push_back({x, y});
    for (int x = x0, y = y0 + 3; x >= 0 && y < tileMapH; x--, y++) routes[3].push_back({x, y});
    auto score = [&](const std::vector<sf::Vector2i>& r) {
        int water = 0, s = 0;
        for (const auto& p : r) {
            std::string k = tileKind(p.x, p.y);
            if (k == "Water") { water++; s += 20; }
            else s += 1;
        }
        bool endsInWater = !r.empty() && tileKind(r.back().x, r.back().y) == "Water";
        return (water > BRIDGE_MAX_SPAN || endsInWater) ? 1 << 30 : s;
    };
    int best = 0;
    for (int r = 1; r < 4; r++)
        if (score(routes[r]) < score(routes[best])) best = r;
    for (const auto& p : routes[best]) {
        std::string k = tileKind(p.x, p.y);
        if (k == "Forest" || k == "Rock") world[p.x][p.y].terrain = Terrain::Grass;
    }
    ground.markDirty();
    buildRoadPath(routes[best], TOOL_ROAD);
}

// --starter: dresses the bench town up to show every road feature, then
// runs 60 days so homes fill and traffic / cars appear. The main street
// becomes asphalt, the east link an avenue, and a short NE spur off the
// middle of the main street makes a 3-way junction that gets a roundabout.
void Play_Scene::buildStarterShowcase() {
    const int x0 = std::max(2, worldSpawnX - 9), x1 = std::min(tileMapW - 3, worldSpawnX + 9);
    const int y0 = std::max(1, worldSpawnY - 4);
    const int savedMoney = money;
    money = 100000000;

    std::vector<sf::Vector2i> mainStreet, link;
    for (int x = x0; x <= x1; x++) mainStreet.push_back({x, y0});
    buildRoadPath(mainStreet, TOOL_ROAD_ASPHALT);
    const int ex = std::min(tileMapW - 1, x1 + 3);
    for (int x = x1 + 1; x <= ex; x++) link.push_back({x, y0});
    for (int k = 1; k <= 3; k++) link.push_back({x1 + 3 - k, y0 + k});
    buildRoadPath(link, TOOL_ROAD_AVENUE);

    // NE spur from the middle of the main street, then a roundabout on the junction
    const int mx = (x0 + x1) / 2;
    std::vector<sf::Vector2i> spur;
    for (int k = 1; k <= 8 && inWorldBounds(mx + k, y0 - k); k++) spur.push_back({mx + k, y0 - k});
    for (const auto& p : spur)
        if (isNaturalObstacleName(tileKind(p.x, p.y)) && tileKind(p.x, p.y) != "Water") world[p.x][p.y].terrain = Terrain::Grass;
    ground.markDirty();
    buildRoadPath(spur, TOOL_ROAD_ASPHALT);
    if (canPlaceTile(mx, y0, TOOL_ROUNDABOUT)) placeTile(mx, y0, TOOL_ROUNDABOUT);

    // Off the road: a wind farm on a power line and a well on a pipe
    placeLinkedUtility(TOOL_POWER_WIND, x0 + 7, y0 - 7, TOOL_POWER_LINE);
    placeLinkedUtility(TOOL_WATER_WELL, x1 - 2, y0 - 11, TOOL_WATER_PIPE);
    // On the road: coal plant + transformer, an underground pump with a
    // treatment plant, and a water tower
    for (BuildTool t : {TOOL_TRANSFORMER, TOOL_TRANSFORMER, TOOL_POWER_COAL, TOOL_WATER_PUMP, TOOL_WATER_TREATMENT, TOOL_WATER_TOWER,
                        TOOL_FIRE_STATION, TOOL_POLICE})
        placeUtilityNear(t, x0, x1, y0);
    // Clinic, landfill and bus depot along the spur
    for (BuildTool t : {TOOL_CLINIC, TOOL_BUS_DEPOT, TOOL_LANDFILL, TOOL_RECYCLING, TOOL_SEWAGE_PLANT, TOOL_INCINERATOR})
        placeNearest(t, mx + 4, y0 - 4, 12);

    // Zones along the spur: medium-density homes on its north-west side,
    // low-density shops on its south-east side (spur tiles have x + y = mx + y0)
    for (int x = mx - 4; x <= mx + 11; x++) {
        for (int y = y0 - 11; y <= y0 - 3; y++) {
            if (!inWorldBounds(x, y) || x + y == mx + y0) continue;
            const std::string k = tileKind(x, y);
            if (k == "Forest" || k == "Rock") world[x][y].terrain = Terrain::Grass;
            BuildTool t = (x + y < mx + y0) ? TOOL_ZONE_R_MED : TOOL_ZONE_C_LOW;
            if (canPlaceTile(x, y, t)) placeTile(x, y, t);
        }
    }
    ground.markDirty();

    // A showcase, not a balanced start: the full road and utility set costs
    // far more upkeep than 18 buildings pay, so it gets a sandbox treasury
    (void)savedMoney;
    money = STARTER_SANDBOX_MONEY;
    utilities.refreshService();
    traffic.update();
    for (int d = 0; d < 60 && !economy.bankrupt; d++) advanceTime(GameClock::SECONDS_PER_DAY);
    std::printf("Starter city: pop %d, money %d, traffic %.2f, roundabout at %d,%d\n",
                townPopulation[0], money, traffic.cityCongestion, mx, y0);
    for (int i = 0; i < tileMapW; i++)   // positions for --inspect screenshots
        for (int j = 0; j < tileMapH; j++)
            if (UtilityZone* u = dynamic_cast<UtilityZone*>(entities[i][j])) {
                if (!u->isSubTile) std::printf("  %s at %d,%d\n", utilityTitle(u->kind), i, j);
            } else if (ServiceZone* sv = dynamic_cast<ServiceZone*>(entities[i][j])) {
                if (!sv->isSubTile) std::printf("  %s at %d,%d\n", serviceTitle(sv->kind), i, j);
            }
}

// Puts a utility on the north side of road row y (roots (x, y - 2), which
// border the row). Water sources take the spot with the most groundwater,
// everything else the westernmost free lot. Clears forest / rock first.
bool Play_Scene::placeUtilityNear(BuildTool tool, int x0, int x1, int y) {
    const int kind = utilityToolKind(tool);
    const bool wantsWater = kind == UTIL_WELL || kind == UTIL_PUMP;
    int bestX = -1;
    float bestScore = -1e9f;
    for (int x = x0 + 1; x <= x1; x++) {
        const int ry = y - 2;
        bool clear = inWorldBounds(x - 1, ry) && inWorldBounds(x + 1, ry + 1);
        for (const auto& o : FOOTPRINT_2X2) {
            if (!clear) break;
            const std::string k = tileKind(x + o[0], ry + o[1]);
            clear = (isClearGroundName(k) || k == "Forest" || k == "Rock") && !hasLine(x + o[0], ry + o[1], LINE_POWER);
        }
        if (!clear) continue;
        // Wells / pumps: most groundwater; transformers: middle of the row; the rest: west first
        float score = wantsWater ? UtilityZone::groundwaterAround(x, ry, kind == UTIL_WELL ? WELL_RADIUS : PUMP_RADIUS)
                    : kind == UTIL_TRANSFORMER ? -static_cast<float>(std::abs(2 * x - x0 - x1))
                    : -static_cast<float>(x);
        if (score > bestScore) { bestScore = score; bestX = x; }
    }
    if (bestX < 0) return false;
    for (const auto& o : FOOTPRINT_2X2) {
        int tx = bestX + o[0], ty = y - 2 + o[1];
        if (isNaturalObstacleName(tileKind(tx, ty)) && tileKind(tx, ty) != "Water") world[tx][ty].terrain = Terrain::Grass;
    }
    ground.markDirty();
    if (!canPlaceTile(bestX, y - 2, tool)) return false;
    placeTile(bestX, y - 2, tool);
    return true;
}

bool Play_Scene::placeNearest(BuildTool tool, int cx, int cy, int r) {
    int best = 1 << 30, bx = -1, by = -1;
    for (int x = cx - r; x <= cx + r; x++)
        for (int y = cy - r; y <= cy + r; y++) {
            int d = (x - cx) * (x - cx) + (y - cy) * (y - cy);
            if (d < best && inWorldBounds(x, y) && canPlaceTile(x, y, tool)) { best = d; bx = x; by = y; }
        }
    if (bx < 0) return false;
    placeTile(bx, by, tool);
    return true;
}

bool Play_Scene::placeLinkedUtility(BuildTool tool, int rx, int ry, BuildTool lineTool) {
    for (const auto& o : FOOTPRINT_2X2) {
        int tx = rx + o[0], ty = ry + o[1];
        if (!inWorldBounds(tx, ty)) return false;
        if (isNaturalObstacleName(tileKind(tx, ty)) && tileKind(tx, ty) != "Water") world[tx][ty].terrain = Terrain::Grass;
    }
    ground.markDirty();
    if (!canPlaceTile(rx, ry, tool)) return false;
    placeTile(rx, ry, tool);

    // Shortest edge-connected line from the building's border to any road
    // (BFS; forest / rock on the way is cleared, water and blocked tiles avoided)
    const uint8_t bit = lineToolBit(lineTool);
    auto passable = [&](int x, int y) {
        if (!inWorldBounds(x, y) || hasLine(x, y, bit)) return inWorldBounds(x, y);
        const std::string k = tileKind(x, y);
        if (k == "Water") return false;
        if (k == "Forest" || k == "Rock") return true;
        return canPlaceTile(x, y, lineTool);
    };
    static sf::Vector2i from[tileMapW][tileMapH];
    static bool seen[tileMapW][tileMapH];
    for (auto& col : seen) for (bool& b : col) b = false;
    std::vector<sf::Vector2i> queue;
    for (const auto& o : BORDER_2X2) {
        int bx = rx + o[0], by = ry + o[1];
        if (inWorldBounds(bx, by) && !seen[bx][by]) {
            seen[bx][by] = true;
            from[bx][by] = {-1, -1};
            queue.push_back({bx, by});
        }
    }
    for (size_t head = 0; head < queue.size(); head++) {
        sf::Vector2i p = queue[head];
        if (RoadTile::at(p.x, p.y) != NULL) {
            // Lay the line back from the road to the building
            for (sf::Vector2i c = from[p.x][p.y]; c.x >= 0; c = from[c.x][c.y]) {
                std::string k = tileKind(c.x, c.y);
                if (k == "Forest" || k == "Rock") world[c.x][c.y].terrain = Terrain::Grass;
                if (canPlaceTile(c.x, c.y, lineTool)) placeTile(c.x, c.y, lineTool);
            }
            ground.markDirty();
            return true;
        }
        if (!passable(p.x, p.y) || queue.size() > 2000) continue;
        for (const auto& d : ROAD_DIR) {
            int nx = p.x + d[0], ny = p.y + d[1];
            if (!inWorldBounds(nx, ny) || seen[nx][ny]) continue;
            seen[nx][ny] = true;
            from[nx][ny] = p;
            queue.push_back({nx, ny});
        }
    }
    return false;
}
