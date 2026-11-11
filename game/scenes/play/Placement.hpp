#pragma once
// Play_Scene member definitions (included at the end of Play_Scene.hpp).
// Placement: costs, placement rules, building / demolishing, road drags, zone growth

int Play_Scene::getToolCost(BuildTool tool) const {
    return toolCost(tool);
}

int Play_Scene::getPlacementCost(int x, int y, BuildTool tool) const {
    if (tool == TOOL_BULLDOZE) return getBulldozeCost(x, y);
    if (isRoadTool(tool)) {
        if (RoadTile* r = RoadTile::at(x, y)) {
            int diff = std::max(0, ROAD_COST[roadToolTier(tool)] - ROAD_COST[r->tier]);
            return r->isBridge() ? diff * BRIDGE_COST_MULT : diff;
        }
    }
    return getToolCost(tool);
}

int Play_Scene::getBulldozeCost(int x, int y) const {
    const std::string kind = tileKind(x, y);
    if (kind == "Forest") return 15;
    if (kind == "Rock") return 60;
    return getToolCost(TOOL_BULLDOZE);
}

bool Play_Scene::canPlaceTile(int x, int y, BuildTool tool, std::string* outReason) const {
    if (tool == TOOL_NONE) return false;
    if (economy.bankrupt) {
        if (outReason) *outReason = "City is bankrupt";
        return false;
    }

    // City ranks unlock tools (normal play only; zones may still grow anything)
    if (milestonesEnabled && !spawningZoned && toolRank(tool) > cityRank()) {
        const CityRank& need = CITY_RANKS[std::min(CITY_RANK_COUNT - 1, toolRank(tool))];
        if (outReason) *outReason = std::string("Unlocks when Chime becomes a ") + need.name + " (" + std::to_string(need.population) + " people)";
        return false;
    }
    int cost = spawningZoned ? 0 : getPlacementCost(x, y, tool);
    if (money < cost) {
        if (outReason) *outReason = "Insufficient Funds ($" + std::to_string(cost) + " needed)";
        return false;
    }

    // Natural obstacles that must be bulldozed before building
    auto terrainBlockReason = [](const std::string& name) -> std::string {
        if (name == "Forest") return "Forest - Clear First (Bulldoze)";
        if (name == "Rock") return "Rocky Terrain - Clear First (Bulldoze)";
        return "";
    };

    // 1. Bulldoze Tool
    if (tool == TOOL_BULLDOZE) {
        if (x < 0 || x >= tileMapW || y < 0 || y >= tileMapH) return false;
        if (isClearGroundName(tileKind(x, y)) && world[x][y].lines == 0 && world[x][y].zone == 0) {
            if (outReason) *outReason = "Already Clear Ground";
            return false;
        }
        return true;
    }

    // Zones (rectangle drag): mark land for buildings to grow on
    if (isZoneTool(tool) || tool == TOOL_DEZONE) {
        if (!inWorldBounds(x, y)) return false;
        if (tool == TOOL_DEZONE) {
            if (world[x][y].zone == 0) {
                if (outReason) *outReason = "Already Unzoned";
                return false;
            }
            return true;
        }
        const std::string name = tileKind(x, y);
        if (world[x][y].zone == zoneToolValue(tool)) {
            if (outReason) *outReason = "Already Zoned";
            return false;
        }
        if (name == "Water" || RoadTile::at(x, y) != NULL) {
            if (outReason) *outReason = "Already a road or water - cannot zone";
            return false;
        }
        if (!terrainBlockReason(name).empty()) {
            if (outReason) *outReason = terrainBlockReason(name);
            return false;
        }
        return true;
    }

    // Power lines (overground) and pipes (underground): 1x1, dragged
    if (isLineTool(tool)) {
        if (!inWorldBounds(x, y)) return false;
        const uint8_t bit = lineToolBit(tool);
        const std::string name = tileKind(x, y);
        if (world[x][y].lines & bit) {
            if (outReason) *outReason = tool == TOOL_POWER_LINE ? "Already a Power Line" : "Already a Pipe";
            return false;
        }
        if (RoadTile::at(x, y) != NULL) {
            if (outReason) *outReason = "Roads Already Carry Power and Water";
            return false;
        }
        if (name == "Water") {
            if (outReason) *outReason = "Lines Cannot Cross Water - use a road bridge";
            return false;
        }
        if (!terrainBlockReason(name).empty()) {
            if (outReason) *outReason = terrainBlockReason(name);
            return false;
        }
        // Pipes run under buildings; power lines need open ground
        if (tool == TOOL_POWER_LINE && !isClearGroundName(name)) {
            if (outReason) *outReason = "Blocked by Building - Bulldoze First";
            return false;
        }
        return true;
    }

    // 2. Road Tools (1x1): build on clear ground, or upgrade a lower-tier road
    if (isRoadTool(tool)) {
        if (x < 0 || x >= tileMapW || y < 0 || y >= tileMapH) return false;
        std::string name = tileKind(x, y);
        if (RoadTile* r = RoadTile::at(x, y)) {
            if (r->tier >= roadToolTier(tool)) {
                if (outReason) *outReason = "Road Already Built";
                return false;
            }
            return true; // upgrade
        }
        if (name == "Water") {
            if (outReason) *outReason = "Cannot Build Road on Water - drag straight across for a bridge (max " +
                                        std::to_string(BRIDGE_MAX_SPAN) + ")";
            return false;
        }
        if (!terrainBlockReason(name).empty()) {
            if (outReason) *outReason = terrainBlockReason(name);
            return false;
        }
        if (!isClearGroundName(name)) {
            if (outReason) *outReason = "Blocked by Structure — Bulldoze First";
            return false;
        }
        return true;
    }

    // Roundabout: replaces a 3- or 4-way junction on land
    if (tool == TOOL_ROUNDABOUT) {
        RoadTile* r = RoadTile::at(x, y);
        if (r == NULL) {
            if (outReason) *outReason = "Roundabouts Go on a Road Junction";
            return false;
        }
        if (r->roundabout) {
            if (outReason) *outReason = "Already a Roundabout";
            return false;
        }
        int links = 0;
        for (int d = 0; d < 4; d++) links += (r->mask >> d) & 1;
        if (r->isBridge() || links < 3) {
            if (outReason) *outReason = "Roundabout Needs a 3- or 4-way Junction";
            return false;
        }
        return true;
    }

    // 3. Water Tool (1x1)
    if (tool == TOOL_WATER) {
        if (x < 0 || x >= tileMapW || y < 0 || y >= tileMapH) return false;
        std::string name = tileKind(x, y);
        if (name == "Water") {
            if (outReason) *outReason = "Already Water";
            return false;
        }
        if (!terrainBlockReason(name).empty()) {
            if (outReason) *outReason = terrainBlockReason(name);
            return false;
        }
        if (!isClearGroundName(name)) {
            if (outReason) *outReason = "Area Occupied — Bulldoze First";
            return false;
        }
        return true;
    }

    // 4. 2x2 Structures (Residential, Commercial, Industrial, Farm, School, Church, Park, Harbour)
    // True 2x2 Isometric Diamond: (x, y), (x + 1, y), (x - 1, y + 1), (x, y + 1)
    if (x < 1 || x + 1 >= tileMapW || y < 0 || y + 1 >= tileMapH) {
        if (outReason) *outReason = "Out of Map Bounds";
        return false;
    }

    const sf::Vector2i diamondOffsets[4] = {{0, 0}, {1, 0}, {-1, 1}, {0, 1}};

    // Check all 4 footprint tiles for obstacles
    for (int k = 0; k < 4; k++) {
        int cx = x + diamondOffsets[k].x;
        int cy = y + diamondOffsets[k].y;
        if (cx < 0 || cx >= tileMapW || cy < 0 || cy >= tileMapH) return false;
        std::string name = tileKind(cx, cy);
        if (isRoadName(name)) {
            if (outReason) *outReason = "Blocked by Road!";
            return false;
        }
        if (name == "Water") {
            if (outReason) *outReason = "Blocked by Water!";
            return false;
        }
        if (hasLine(cx, cy, LINE_POWER)) {
            if (outReason) *outReason = "Power Line in the Way - Bulldoze First";
            return false;
        }
        if (!terrainBlockReason(name).empty()) {
            if (outReason) *outReason = terrainBlockReason(name);
            return false;
        }
        if (!isClearGroundName(name)) {
            if (outReason) *outReason = "Lot Occupied — Bulldoze First!";
            return false;
        }
    }

    // Access: road / water on a tile sharing an edge with the footprint
    bool touchesRoad = hasAdjacentRoad(x, y);
    bool touchesWater = hasAdjacentWater(x, y);

    if (tool == TOOL_HARBOUR) {
        if (!touchesWater) {
            if (outReason) *outReason = "Harbour Must Touch Shoreline Water!";
            return false;
        }
        if (!touchesRoad) {
            if (outReason) *outReason = "Harbour Requires Road Connection!";
            return false;
        }
        return true;
    }

    // Sewage outflows empty into a lake, river or the sea
    if (tool == TOOL_SEWAGE_OUTFLOW && !touchesWater) {
        if (outReason) *outReason = "Sewage Outflow Must Touch Water";
        return false;
    }
    // Power & water buildings connect through roads or lines, so a road is optional
    if (isUtilityTool(tool)) return true;

    // Mines dig into a coal / iron / stone deposit under the footprint
    if (tool == TOOL_MINE) {
        bool deposit = false;
        for (int k = 0; k < 4; k++) {
            int cx = x + diamondOffsets[k].x, cy = y + diamondOffsets[k].y;
            if (world[cx][cy].deposit != Deposit::None && world[cx][cy].depositAmount > 0) deposit = true;
        }
        if (!deposit) {
            if (outReason) *outReason = "Mines Need a Coal, Iron or Stone Deposit (Deposits view)";
            return false;
        }
    }

    // All other 2x2 buildings require road connection
    if (!touchesRoad) {
        if (outReason) *outReason = "Requires Road Connection!";
        return false;
    }

    return true;
}

void Play_Scene::placeTile(int x, int y, BuildTool tool) {
    if (x < 0 || x >= tileMapW || y < 0 || y >= tileMapH) return;
    if (tool == TOOL_NONE) return;

    std::string reason = "";
    if (!canPlaceTile(x, y, tool, &reason)) {
        showToast("Can't build here: " + reason, true);
        return;
    }

    int cost = spawningZoned ? 0 : getPlacementCost(x, y, tool);
    minimapDirty = true;

    auto demolishAt = [&](int tx, int ty) {
        if (tx < 0 || tx >= tileMapW || ty < 0 || ty >= tileMapH) return;
        if (entities[tx][ty] == NULL) {
            // Clearing natural ground (forest, rock, water) turns it into grassland
            if (isNaturalObstacleName(tileKind(tx, ty))) {
                world[tx][ty].terrain = Terrain::Grass;
                ground.markDirty();
            }
            return;
        }
        Zone* z = dynamic_cast<Zone*>(entities[tx][ty]);
        if (z != NULL && z->isMultiTile) {
            int rx = z->isSubTile ? z->rootX : tx;
            int ry = z->isSubTile ? z->rootY : ty;
            const sf::Vector2i dOffsets[4] = {{0, 0}, {1, 0}, {-1, 1}, {0, 1}};
            for (int k = 0; k < 4; k++) {
                int cx = rx + dOffsets[k].x;
                int cy = ry + dOffsets[k].y;
                if (cx >= 0 && cx < tileMapW && cy >= 0 && cy < tileMapH) {
                    delete entities[cx][cy];
                    entities[cx][cy] = NULL;
                }
            }
        } else {
            // Structure removed: the natural ground underneath shows again
            delete entities[tx][ty];
            entities[tx][ty] = NULL;
        }
    };

    if (tool == TOOL_BULLDOZE) {
        bool wasRoad = isRoadName(tileKind(x, y));
        utilitiesDirty = true;
        // Bare ground with lines: the power line goes first, then the pipe, then the zone
        if (entities[x][y] == NULL && (world[x][y].lines != 0 || world[x][y].zone != 0) && isClearGroundName(tileKind(x, y))) {
            uint8_t& l = world[x][y].lines;
            if (l != 0) l &= (l & LINE_POWER) ? static_cast<uint8_t>(~LINE_POWER) : static_cast<uint8_t>(~LINE_PIPE);
            else world[x][y].zone = 0;
            money -= cost;
            return;
        }
        demolishAt(x, y);
        if (wasRoad) onRoadsChanged(x, y);
        else refreshBuildingsAround(x, y); // a removed water tile can disconnect a harbour
        money -= cost;
        return;
    }

    if (isRoadTool(tool)) {
        if (RoadTile* r = RoadTile::at(x, y)) {
            r->setTier(roadToolTier(tool)); // upgrade in place
        } else {
            delete entities[x][y];
            entities[x][y] = new RoadTile(x, y, roadToolTier(tool));
        }
        onRoadsChanged(x, y);
        money -= cost;
        return;
    }

    if (tool == TOOL_ROUNDABOUT) {
        RoadTile* r = RoadTile::at(x, y);
        r->roundabout = true;
        r->check();
        money -= cost;
        return;
    }

    if (isZoneTool(tool) || tool == TOOL_DEZONE) {
        world[x][y].zone = zoneToolValue(tool);   // 0 for dezone; buildings stay
        money -= cost;
        return;
    }

    if (isLineTool(tool)) {
        world[x][y].lines |= lineToolBit(tool);
        utilitiesDirty = true;
        money -= cost;
        return;
    }

    if (tool == TOOL_WATER) {
        delete entities[x][y];
        entities[x][y] = NULL;
        world[x][y].lines = 0;   // flooding removes lines and pipes
        utilitiesDirty = true;
        world[x][y].terrain = Terrain::Water;
        ground.markDirty();
        refreshBuildingsAround(x, y); // harbours need water on their border
        money -= cost;
        return;
    }

    // 2x2 Structures: clear the 4 grass tiles (True 2x2 Diamond)
    const sf::Vector2i dOffsets[4] = {{0, 0}, {1, 0}, {-1, 1}, {0, 1}};
    for (int k = 0; k < 4; k++) {
        int cx = x + dOffsets[k].x;
        int cy = y + dOffsets[k].y;
        delete entities[cx][cy];
        entities[cx][cy] = NULL;
    }

    money -= cost;

    std::string zoneBase = "grass";
    std::string typeName = "";
    Zone* masterZone = NULL;

    switch (tool) {
        case TOOL_RES_COTTAGES:
            masterZone = new ResidentalZone(x, y, 1, 1);
            zoneBase = "zone_residental";
            typeName = "ResidentalZone";
            break;
        case TOOL_RES_APARTMENTS:
            masterZone = new ResidentalZone(x, y, 2, 1);
            zoneBase = "zone_residental";
            typeName = "ResidentalZone";
            break;
        case TOOL_RES_TOWER:
            masterZone = new ResidentalZone(x, y, 3, 1);
            zoneBase = "zone_residental";
            typeName = "ResidentalZone";
            break;
        case TOOL_COM_GROCERY:
            masterZone = new CommericalZone(x, y, 1, 1);
            zoneBase = "zone_commercial";
            typeName = "CommericalZone";
            break;
        case TOOL_COM_PLAZA:
            masterZone = new CommericalZone(x, y, 2, 1);
            zoneBase = "zone_commercial";
            typeName = "CommericalZone";
            break;
        case TOOL_COM_TOWER:
            masterZone = new CommericalZone(x, y, 3, 1);
            zoneBase = "zone_commercial";
            typeName = "CommericalZone";
            break;
        case TOOL_IND_WORKSHOP:
            masterZone = new IndustrialZone(x, y, 1, 1);
            zoneBase = "zone_industrial";
            typeName = "IndustrialZone";
            break;
        case TOOL_IND_FACTORY:
            masterZone = new IndustrialZone(x, y, 2, 1);
            zoneBase = "zone_industrial";
            typeName = "IndustrialZone";
            break;
        case TOOL_IND_TECH:
            masterZone = new IndustrialZone(x, y, 3, 1);
            zoneBase = "zone_industrial";
            typeName = "IndustrialZone";
            break;
        case TOOL_FARM_BARN:
            masterZone = new FarmZone(x, y, 1);
            zoneBase = "zone_farm";
            typeName = "FarmZone";
            break;
        case TOOL_FARM_WINDMILL:
            masterZone = new FarmZone(x, y, 2);
            zoneBase = "zone_farm";
            typeName = "FarmZone";
            break;
        case TOOL_SCHOOL:
            masterZone = new SchoolZone(x, y);
            zoneBase = "grass";
            typeName = "SchoolZone";
            break;
        case TOOL_CHURCH:
            masterZone = new ChurchZone(x, y);
            zoneBase = "grass";
            typeName = "ChurchZone";
            break;
        case TOOL_PARK:
            masterZone = new ParkZone(x, y);
            zoneBase = "grass";
            typeName = "ParkZone";
            break;
        case TOOL_HARBOUR:
            masterZone = new HarbourZone(x, y);
            zoneBase = "grass";
            typeName = "HarbourZone";
            break;
        default:
            if (isServiceTool(tool)) {
                masterZone = new ServiceZone(x, y, serviceToolKind(tool));
                zoneBase = "grass";
                typeName = serviceEntityName(serviceToolKind(tool));
                break;
            }
            if (!isUtilityTool(tool)) return;
            masterZone = new UtilityZone(x, y, utilityToolKind(tool));
            zoneBase = "grass";
            typeName = "UtilityZone";
            break;
    }

    // Placed by hand: keeps its tier. Grown on a zone: growZones sets the rules.
    masterZone->fixedTier = !spawningZoned;
    entities[x][y] = masterZone;
    entities[x + 1][y]     = new SubZone(x + 1, y,     x, y, typeName, zoneBase);
    entities[x - 1][y + 1] = new SubZone(x - 1, y + 1, x, y, typeName, zoneBase);
    entities[x][y + 1]     = new SubZone(x,     y + 1, x, y, typeName, zoneBase);
    masterZone->checkValid();
    utilitiesDirty = true;

    // Tell the player at once when a new utility cannot work yet
    if (UtilityZone* u = dynamic_cast<UtilityZone*>(masterZone)) {
        utilities.refreshService();
        utilitiesDirty = false;
        if (!u->linked)
            showToast(std::string(utilityTitle(u->kind)) + " is not connected - join it to a road or " +
                      (isPowerUtility(u->kind) ? "power line" : "water pipe"), false);
        else if (u->noTransformer)
            showToast("Power plants need a Transformer on their grid to deliver power", false);
    }
}

void Play_Scene::executeMultibuild() {
    BuildTool tool = toolbar.getActiveTool();
    if (tool == TOOL_NONE) return;
    // Snapshot for undo; kept only if the action changed something
    std::ostringstream before;
    writeCity(before);
    const int moneyBefore = money;
    const std::string msgBefore = lastPlacementError;
    const float timerBefore = placementErrorTimer;
    runMultibuild(tool);
    std::ostringstream after;
    writeCity(after);
    if (toastIsError && (lastPlacementError != msgBefore || placementErrorTimer > timerBefore)) audio.play("error", 0.7f);
    if (after.str() != before.str()) {
        audio.play(tool == TOOL_BULLDOZE ? "bulldoze"
                   : (isRoadTool(tool) || isLineTool(tool) || isZoneTool(tool) || tool == TOOL_DEZONE) ? "road" : "build");
        undoStack.push_back({before.str(), moneyBefore - money});
        if (undoStack.size() > UNDO_LIMIT) undoStack.erase(undoStack.begin());
    }
}

void Play_Scene::runMultibuild(BuildTool tool) {

    if (isRoadTool(tool)) {
        std::string reason = buildRoadPath(computeDragTiles(), tool);
        if (!reason.empty()) showToast("Can't build here: " + reason, true);
    } else if (isLineTool(tool) || isZoneTool(tool) || tool == TOOL_DEZONE) {
        // Tiles that already carry this line / zone (or are roads, water,
        // forest for zones) are skipped quietly; only real problems toast
        std::string firstFail;
        for (const auto& pt : computeDragTiles()) {
            std::string reason;
            if (canPlaceTile(pt.x, pt.y, tool, &reason)) placeTile(pt.x, pt.y, tool);
            else if (firstFail.empty() && reason.rfind("Already", 0) != 0 && reason.rfind("Roads Already", 0) != 0 &&
                     (!isZoneTool(tool) || reason.rfind("Insufficient", 0) == 0))
                firstFail = reason;
        }
        if (!firstFail.empty()) showToast("Can't build here: " + firstFail, true);
    } else if (tool == TOOL_WATER || tool == TOOL_BULLDOZE) {
        // One toast for the first tile that fails, not one per tile
        std::string firstFail;
        for (const auto& pt : computeDragTiles()) {
            std::string reason;
            if (canPlaceTile(pt.x, pt.y, tool, &reason)) placeTile(pt.x, pt.y, tool);
            else if (firstFail.empty()) firstFail = reason;
        }
        if (!firstFail.empty()) showToast("Can't build here: " + firstFail, true);
    } else {
        int gx = isDragging ? dragCurrent.x : tileX;
        int gy = isDragging ? dragCurrent.y : tileY;
        placeTile(gx, gy, tool);
    }
}

// Daily: for each sector with low-income demand, grow up to 1 + demand / 30
// (max 3) tier-1 buildings on lots whose 4 tiles carry the same zone, are
// empty clear ground without power lines, and touch a road. A position /
// day hash skips two of three lots on the first pass so growth spreads
// out; a second pass fills the rest of the budget. Returns buildings grown.
int Play_Scene::growZones() {
    static const BuildTool byTier[SECTOR_COUNT][3] = {{TOOL_RES_COTTAGES, TOOL_RES_APARTMENTS, TOOL_RES_TOWER},
                                                      {TOOL_COM_GROCERY, TOOL_COM_PLAZA, TOOL_COM_TOWER},
                                                      {TOOL_IND_WORKSHOP, TOOL_IND_FACTORY, TOOL_IND_TECH}};
    const unsigned day = static_cast<unsigned>(gameClock.getTotalDays());
    int grown = 0;
    for (int s = 0; s < SECTOR_COUNT; s++) {
        float d = 0.f;
        for (int w = 0; w < WEALTH_COUNT; w++) d = std::max(d, economy.demand[s][w]);
        if (d <= 0.f) continue;
        int budget = std::min(ZONE_GROWTH_MAX_PER_DAY, 1 + static_cast<int>(d) / ZONE_GROWTH_DEMAND_STEP);
        for (int pass = 0; pass < 2 && budget > 0; pass++) {
            for (int x = 1; x + 1 < tileMapW && budget > 0; x++) {
                for (int y = 0; y + 1 < tileMapH && budget > 0; y++) {
                    const uint8_t zv = world[x][y].zone;
                    if (zoneSector(zv) != s) continue;
                    bool lot = true;
                    for (const auto& o : FOOTPRINT_2X2) {
                        int tx = x + o[0], ty = y + o[1];
                        if (world[tx][ty].zone != zv || entities[tx][ty] != NULL || !isClearGroundName(tileKind(tx, ty)) ||
                            hasLine(tx, ty, LINE_POWER)) { lot = false; break; }
                    }
                    if (!lot || !hasAdjacentRoad(x, y)) continue;
                    unsigned h = static_cast<unsigned>(x) * 73856093u ^ static_cast<unsigned>(y) * 19349663u ^ (day + 1u) * 83492791u;
                    if (pass == 0 && h % 3 != 0) continue;
                    // Grow the richest income level with demand that the density
                    // and the land value allow
                    int tier = 0;
                    for (int w = std::min(2, zoneMaxTier(zv) - 1); w >= 0; w--) {
                        const int lv = world[x][y].landValue;
                        bool landOk = w == 0 || (w == 1 ? lv >= LAND_VALUE_TIER2 : lv >= LAND_VALUE_TIER3);
                        if (economy.demand[s][w] > 0.f && landOk) { tier = w + 1; break; }
                    }
                    if (tier == 0) continue;

                    spawningZoned = true;
                    placeTile(x, y, byTier[s][tier - 1]);
                    spawningZoned = false;
                    Zone* z = dynamic_cast<Zone*>(entities[x][y]);
                    if (z == NULL) continue;
                    z->zoned = true;
                    z->fixedTier = false;
                    z->maxTier = zoneMaxTier(zv);
                    z->variant = 1 + static_cast<int>((h >> 4) % VARIANT_COUNT);
                    if (auto* r = dynamic_cast<ResidentalZone*>(z)) r->updateBuildingTexture();
                    else if (auto* c = dynamic_cast<CommericalZone*>(z)) c->updateBuildingTexture();
                    else if (auto* i = dynamic_cast<IndustrialZone*>(z)) i->updateBuildingTexture();
                    budget--;
                    grown++;
                }
            }
        }
    }
    return grown;
}

// Re-checks road access for every 2x2 building whose border could include
// (x, y). Border tiles sit at most 2 tiles from the building's root.
void Play_Scene::refreshBuildingsAround(int x, int y) {
    for (int i = x - 2; i <= x + 2; i++) {
        for (int j = y - 2; j <= y + 2; j++) {
            if (!inWorldBounds(i, j) || entities[i][j] == NULL) continue;
            Zone* z = dynamic_cast<Zone*>(entities[i][j]);
            if (z != NULL && z->isMultiTile && !z->isSubTile) z->checkValid();
        }
    }
}

// Re-runs road autotiling for (x, y) and its 8 neighbours, which covers
// every direction DirtRoad::check() looks at.
void Play_Scene::refreshRoadsAround(int x, int y) {
    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
            if (!inWorldBounds(i, j) || entities[i][j] == NULL) continue;
            RoadTile* road = dynamic_cast<RoadTile*>(entities[i][j]);
            if (road != NULL) road->check();
        }
    }
}

void Play_Scene::onRoadsChanged(int x, int y) {
    refreshRoadsAround(x, y);     // the tile and every road touching it
    refreshBuildingsAround(x, y); // access for lots on its border
    roadNet.rebuild();            // components and outside connection
    utilitiesDirty = true;        // utilities travel along roads
    minimapDirty = true;
}

// Classifies every tile of a road drag. Land tiles build or upgrade; runs
// of water become a bridge when they are crossed in a straight line, land
// to land, over at most BRIDGE_MAX_SPAN tiles. Costs accumulate so the
// preview turns red where the money runs out.
std::vector<Play_Scene::RoadStep> Play_Scene::planRoadPath(const std::vector<sf::Vector2i>& tiles, BuildTool tool) const {
    const int tier = roadToolTier(tool);
    const int n = static_cast<int>(tiles.size());
    std::vector<RoadStep> steps(static_cast<size_t>(n));
    auto isWater = [&](int k) { return tileKind(tiles[k].x, tiles[k].y) == "Water"; };
    // Edge axis of the step from tile k to k+1: 1 = SE-NW (+-1, 0), 0 = NE-SW (+-1, -+1)
    auto stepAxis = [&](int k) { return tiles[k + 1].y == tiles[k].y ? 1 : 0; };

    for (int k = 0; k < n; k++) {
        RoadStep& st = steps[k];
        st.p = tiles[k];
        if (isWater(k)) continue; // decided per run below
        std::string reason;
        if (RoadTile* r = RoadTile::at(st.p.x, st.p.y)) {
            st.action = (r->tier >= tier) ? ROAD_SKIP : ROAD_UPGRADE;
        } else if (canPlaceTile(st.p.x, st.p.y, tool, &reason)) {
            st.action = ROAD_BUILD;
        } else {
            st.action = ROAD_FAIL;
            st.reason = reason;
        }
        if (st.action == ROAD_BUILD || st.action == ROAD_UPGRADE) st.cost = getPlacementCost(st.p.x, st.p.y, tool);
    }

    for (int s = 0; s < n; s++) {
        if (!isWater(s)) continue;
        int e = s;
        while (e + 1 < n && isWater(e + 1)) e++;
        std::string why;
        int axis = n > 1 ? stepAxis(s > 0 ? s - 1 : s) : 1;
        if (economy.bankrupt) why = "City is bankrupt";
        else if (s == 0 || e == n - 1) why = "Bridges must start and end on land";
        else if (e - s + 1 > BRIDGE_MAX_SPAN) why = "Bridge too long (max " + std::to_string(BRIDGE_MAX_SPAN) + " tiles)";
        else if (steps[s - 1].action == ROAD_FAIL || steps[e + 1].action == ROAD_FAIL) why = "Bridge needs buildable land at both ends";
        for (int k = s - 1; why.empty() && k <= e; k++) {
            if (stepAxis(k) != axis) why = "Bridges must run straight";
        }
        for (int k = s; k <= e; k++) {
            steps[k].action = why.empty() ? ROAD_BRIDGE : ROAD_FAIL;
            steps[k].axis = axis;
            steps[k].reason = why;
            if (why.empty()) steps[k].cost = ROAD_COST[tier] * BRIDGE_COST_MULT;
        }
        s = e;
    }

    int total = 0;
    for (RoadStep& st : steps) {
        if (st.action == ROAD_SKIP || st.action == ROAD_FAIL) continue;
        total += st.cost;
        if (total > money) {
            st.action = ROAD_FAIL;
            st.reason = "Insufficient Funds ($" + std::to_string(total) + " needed)";
        }
    }
    return steps;
}

std::string Play_Scene::buildRoadPath(const std::vector<sf::Vector2i>& tiles, BuildTool tool) {
    std::string firstFail;
    for (const RoadStep& st : planRoadPath(tiles, tool)) {
        const int x = st.p.x, y = st.p.y;
        if (st.action == ROAD_BUILD || st.action == ROAD_UPGRADE) {
            placeTile(x, y, tool);
        } else if (st.action == ROAD_BRIDGE) {
            delete entities[x][y];
            entities[x][y] = new RoadTile(x, y, roadToolTier(tool), st.axis);
            money -= st.cost;
            onRoadsChanged(x, y);
        } else if (st.action == ROAD_FAIL && firstFail.empty()) {
            firstFail = st.reason;
        }
    }
    return firstFail;
}
