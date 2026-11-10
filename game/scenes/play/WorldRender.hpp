#pragma once
// Play_Scene member definitions (included at the end of Play_Scene.hpp).
// World rendering: ground, buildings, cars, overlays, utility lines, drag preview

void Play_Scene::renderWorld(float deltaTime) {
    _window.setView(view);

    // Background
    for (size_t i = 0; i < bg.size(); i++) {
        if (bg[i] != NULL) bg[i]->update(deltaTime);
    }

    // No per-frame entity update: map entities have no per-frame logic.
    // Their simulation runs in runSimulationTick() once per second.

    // Viewport frustum culling bounds
    sf::Vector2f vCenter = view.getCenter();
    sf::Vector2f vSize = view.getSize();
    float viewL = vCenter.x - vSize.x / 2.f - 64.f;
    float viewR = vCenter.x + vSize.x / 2.f + 64.f;
    float viewT = vCenter.y - vSize.y / 2.f - 200.f; // Skyscraper height margin
    float viewB = vCenter.y + vSize.y / 2.f + 64.f;

    const float tallMargin = 140.f; // buildings and trees rise above their tile
    auto isVisible = [&](float px, float py) -> bool {
        return (px + tileSizeX >= viewL && px <= viewR &&
                py + tileSizeY + tallMargin >= viewT && py <= viewB);
    };

    // Only tiles inside the camera's band are visited by the draw passes
    const VisibleTiles vis = visibleTileRange(viewL, viewR, viewT, viewB, tallMargin);

    // PASS 1a: Natural ground for the whole map in a single draw call
    ground.draw(_window);

    // PASS 1b: Ground layer of structures (roads, zone bases)
    for (int i = vis.iMin; i <= vis.iMax; i++) {
        for (int j = vis.jMin(i); j <= vis.jMax(i); j++) {
            if (entities[i][j] != NULL && entities[i][j]->position != NULL) {
                float px = entities[i][j]->position->x;
                float py = entities[i][j]->position->y;
                if (!isVisible(px, py)) continue;

                Zone* z = dynamic_cast<Zone*>(entities[i][j]);
                if (z != NULL) {
                    // Building lots follow the season like natural grass; roads keep their colour
                    if (z->sprite != NULL && !isRoadName(z->getName())) {
                        z->sprite->setColor(ground.getSeasonLook().grass);
                    }
                    z->drawGround(_window);
                } else {
                    entities[i][j]->draw();
                }
            }
        }
    }

    // PASS 1b2: Painted zones that nothing has grown on yet (stronger = denser)
    {
        static const char* zoneTex[3] = {"zone_residental", "zone_commercial", "zone_industrial"};
        static const sf::Uint8 densityAlpha[3] = {110, 160, 210};
        sf::Sprite zs;
        zs.setScale(2.f, 2.f);
        for (int i = vis.iMin; i <= vis.iMax; i++) {
            for (int j = vis.jMin(i); j <= vis.jMax(i); j++) {
                const uint8_t zv = world[i][j].zone;
                if (zv == 0 || entities[i][j] != NULL) continue;
                sf::Vector2f tp = tileScreenPos(i, j);
                if (!isVisible(tp.x, tp.y)) continue;
                zs.setTexture(m_textures[zoneTex[zoneSector(zv)]], true);
                zs.setColor(sf::Color(255, 255, 255, densityAlpha[zoneDensity(zv)]));
                zs.setPosition(tp);
                _window.draw(zs);
            }
        }
    }

    // PASS 1c: Winter snow over land, lots and roads
    ground.drawSnow(_window);

    // PASS 1.5: Resource / soil overlay
    if (overlayMode != 0) {
        renderOverlay(viewL, viewR, viewT, viewB);
    }

    // Utility networks: power lines / pipes (and the roads that also carry them)
    if (overlayMode == 5) renderUtilityLines(viewL, viewR, viewT, viewB, LINE_POWER);
    if (overlayMode == 6) renderUtilityLines(viewL, viewR, viewT, viewB, LINE_PIPE);

    // PASS 2: Render 3D Isometric Buildings in Isometric Depth Order (i + j),
    // walking only the depths and tiles inside the visible band
    const int jLo = vis.jLowest();
    const int jHi = vis.jHighest();

    // Cars, bucketed by the tile they are drawn with (linked lists per tile)
    carHead.assign(static_cast<size_t>(tileMapW * tileMapH), -1);
    carNext.assign(cars.cars.size(), -1);
    for (size_t c = 0; c < cars.cars.size(); c++) {
        sf::Vector2i t = CarRenderer::drawTile(cars.cars[c]);
        if (!inWorldBounds(t.x, t.y)) continue;
        size_t k = static_cast<size_t>(t.x * tileMapH + t.y);
        carNext[c] = carHead[k];
        carHead[k] = static_cast<int>(c);
    }
    sf::Sprite carSprite;

    for (int depth = vis.iMin + jLo; depth <= vis.iMax + jHi; depth++) {
        int iFrom = std::max(vis.iMin, depth - jHi);
        int iTo = std::min(vis.iMax, depth - jLo);
        for (int i = iFrom; i <= iTo; i++) {
            int j = depth - i;
            if (j >= vis.jMin(i) && j <= vis.jMax(i)) {
                for (int c = carHead[static_cast<size_t>(i * tileMapH + j)]; c >= 0; c = carNext[static_cast<size_t>(c)]) {
                    cars.drawCar(_window, cars.cars[static_cast<size_t>(c)], carSprite);
                }
                if (entities[i][j] != NULL && entities[i][j]->position != NULL) {
                    Zone* z = dynamic_cast<Zone*>(entities[i][j]);
                    if (z != NULL && !z->isSubTile) {
                        float px = z->position->x;
                        float py = z->position->y;
                        if (isVisible(px, py)) {
                            z->drawBuilding(_window);
                            renderUtilityIcons(z);
                            if (z->fireDays > 0) drawFire(z);
                        }
                    }
                } else if (entities[i][j] == NULL) {
                    Terrain t = world[i][j].terrain;
                    if (t == Terrain::Forest || t == Terrain::Rock) {
                        sf::Vector2f tp = tileScreenPos(i, j);
                        if (isVisible(tp.x, tp.y)) ground.drawTall(_window, i, j);
                    }
                }
                if (world[i][j].lines & LINE_POWER) drawPylon(i, j);
            }
        }
    }

    // PASS 3: Drag & Build Ghost Preview & World Cursor
    renderDragPreview();

    if (player != NULL && player->isActive && !isDragging) {
        player->update(deltaTime);
    }
}

// Green below 50% of capacity, yellow at capacity, red at 150% and above
inline sf::Color congestionColor(float cong) {
    float t = std::max(0.f, std::min(1.f, (cong - 0.5f) / 1.f));
    sf::Uint8 r = static_cast<sf::Uint8>(t < 0.5f ? 60 + t * 2.f * 180.f : 240);
    sf::Uint8 g = static_cast<sf::Uint8>(t < 0.5f ? 200 : 200 - (t - 0.5f) * 2.f * 150.f);
    return sf::Color(r, g, 50, 190);
}

void Play_Scene::renderOverlay(float viewL, float viewR, float viewT, float viewB) {
    sf::ConvexShape diamond(4);
    diamond.setPoint(0, sf::Vector2f(32.f, 0.f));
    diamond.setPoint(1, sf::Vector2f(64.f, 16.f));
    diamond.setPoint(2, sf::Vector2f(32.f, 32.f));
    diamond.setPoint(3, sf::Vector2f(0.f, 16.f));

    const VisibleTiles vis = visibleTileRange(viewL, viewR, viewT, viewB, 0.f);

    for (int i = vis.iMin; i <= vis.iMax; i++) {
        for (int j = vis.jMin(i); j <= vis.jMax(i); j++) {
            sf::Vector2f tp = tileScreenPos(i, j);
            float px = tp.x;
            float py = tp.y;
            if (px + tileSizeX < viewL || px > viewR || py + tileSizeY < viewT || py > viewB) continue;

            const TileData& td = world[i][j];
            sf::Color col(0, 0, 0, 0);
            if (overlayMode == 8) {
                // Services: police blue, fire red, clinic green (mixed where they overlap);
                // buildings whose garbage nobody collects in brown
                if (td.terrain == Terrain::Water) continue;
                Zone* z = dynamic_cast<Zone*>(entities[i][j]);
                if (z != NULL && z->isSubTile && inWorldBounds(z->rootX, z->rootY)) z = dynamic_cast<Zone*>(entities[z->rootX][z->rootY]);
                if (z != NULL && ServiceSystem::makesGarbage(z) && !services.garbageOk(z->posX, z->posY)) {
                    col = sf::Color(120, 90, 50, 210);
                } else {
                    const int p = services.policeMap[i][j], f = services.fireMap[i][j], c = services.clinicMap[i][j];
                    const int m = std::max(p, std::max(f, c));
                    if (m == 0) col = sf::Color(10, 12, 18, 90);
                    else col = sf::Color(static_cast<sf::Uint8>(40 + f * 1.9f), static_cast<sf::Uint8>(40 + c * 1.6f),
                                         static_cast<sf::Uint8>(40 + p * 2.0f), static_cast<sf::Uint8>(60 + m));
                }
                diamond.setPosition(px, py);
                diamond.setFillColor(col);
                _window.draw(diamond);
                continue;
            }
            if (overlayMode == 7) {
                if (td.terrain == Terrain::Water) continue;
                const int lv = td.landValue;
                col = lv >= LAND_VALUE_TIER3 ? sf::Color(70, 210, 110, 150)
                    : lv >= LAND_VALUE_TIER2 ? sf::Color(230, 200, 70, 140) : sf::Color(200, 70, 60, 130);
                col.a = static_cast<sf::Uint8>(70 + lv);
                diamond.setPosition(px, py);
                diamond.setFillColor(col);
                _window.draw(diamond);
                continue;
            }
            if (overlayMode == 5 || overlayMode == 6) {
                // Utilities: buildings green when served, red when not, amber on
                // untreated water; plants / facilities in the utility colour
                const bool power = overlayMode == 5;
                Zone* z = dynamic_cast<Zone*>(entities[i][j]);
                if (z != NULL && z->isSubTile && inWorldBounds(z->rootX, z->rootY)) z = dynamic_cast<Zone*>(entities[z->rootX][z->rootY]);
                UtilityZone* u = dynamic_cast<UtilityZone*>(z);
                if (u != NULL) {
                    if (isPowerUtility(u->kind) != power) col = sf::Color(10, 12, 18, 90);
                    else col = power ? sf::Color(250, 205, 50, 200) : sf::Color(60, 150, 240, 210);
                } else if (z != NULL && UtilitySystem::consumes(z)) {
                    bool ok = power ? utilities.powered(z->posX, z->posY) : utilities.watered(z->posX, z->posY);
                    bool rawOnly = ok && !power && utilities.untreated(z->posX, z->posY);
                    if (!z->isActive) col = sf::Color(120, 120, 130, 150);
                    else if (rawOnly) col = sf::Color(235, 165, 50, 180);
                    else col = ok ? sf::Color(60, 200, 90, 170) : sf::Color(230, 60, 50, 190);
                } else if (td.terrain == Terrain::Water) {
                    continue;
                } else if (power) {
                    col = sf::Color(10, 12, 18, 90);   // dim land so the network stands out
                } else {
                    // Water view: live groundwater reserve underneath
                    col = sf::Color(40, 130, 230, static_cast<sf::Uint8>(15 + td.groundwater * 1.3f));
                }
                diamond.setPosition(px, py);
                diamond.setFillColor(col);
                _window.draw(diamond);
                continue;
            }
            if (overlayMode == 4) {
                // Traffic: roads from green (free) through yellow to red (jammed); land dimmed
                if (RoadTile::at(i, j) != NULL) {
                    col = congestionColor(traffic.congestion[i][j]);
                } else if (td.terrain != Terrain::Water) {
                    col = sf::Color(10, 12, 18, 80);
                } else {
                    continue;
                }
                diamond.setPosition(px, py);
                diamond.setFillColor(col);
                _window.draw(diamond);
                continue;
            }
            if (td.terrain == Terrain::Water) continue;

            if (overlayMode == 1) {
                // Fertility: red (barren) -> yellow -> green (rich)
                float f = static_cast<float>(td.fertility) / 100.f;
                sf::Uint8 r = static_cast<sf::Uint8>(f < 0.5f ? 210 : 210 - (f - 0.5f) * 2.f * 170.f);
                sf::Uint8 g = static_cast<sf::Uint8>(f < 0.5f ? 60 + f * 2.f * 150.f : 210);
                col = sf::Color(r, g, 50, 120);
            } else if (overlayMode == 2) {
                // Groundwater: deeper blue = more water underground
                sf::Uint8 a = static_cast<sf::Uint8>(30 + td.groundwater * 1.7f);
                col = sf::Color(40, 130, 230, a);
            } else if (overlayMode == 3) {
                if (td.deposit == Deposit::None) {
                    col = sf::Color(10, 12, 18, 90); // dim empty land so deposits stand out
                } else {
                    sf::Uint8 a = static_cast<sf::Uint8>(90 + td.depositAmount * 1.5f);
                    if (td.deposit == Deposit::Coal) col = sf::Color(25, 25, 30, a);
                    else if (td.deposit == Deposit::Iron) col = sf::Color(200, 85, 30, a);
                    else col = sf::Color(215, 215, 220, a);
                }
            }

            diamond.setPosition(px, py);
            diamond.setFillColor(col);
            _window.draw(diamond);
        }
    }
}

void Play_Scene::renderDragPreview() {
    BuildTool tool = toolbar.getActiveTool();
    if (tool == TOOL_NONE) return;

    sf::Sprite ghostSprite;

    // 1x1 Isometric Diamond (Four vertices forming pure 2:1 rhombus)
    sf::ConvexShape diamond1x1(4);
    diamond1x1.setPoint(0, sf::Vector2f(32.f, 0.f));
    diamond1x1.setPoint(1, sf::Vector2f(64.f, 16.f));
    diamond1x1.setPoint(2, sf::Vector2f(32.f, 32.f));
    diamond1x1.setPoint(3, sf::Vector2f(0.f, 16.f));

    auto highlightTile = [&](int tx, int ty, const sf::Color& fill, const sf::Color& outline) {
        if (tx < 0 || tx >= tileMapW || ty < 0 || ty >= tileMapH) return;
        diamond1x1.setPosition(tileScreenPos(tx, ty));
        diamond1x1.setFillColor(fill);
        diamond1x1.setOutlineColor(outline);
        diamond1x1.setOutlineThickness(1.5f);
        _window.draw(diamond1x1);
    };

    toolbar.dragCostPreview = -1;
    toolbar.pillNote.clear();
    if ((tool == TOOL_WATER_WELL || tool == TOOL_WATER_PUMP) && inWorldBounds(tileX, tileY)) {
        int kind = utilityToolKind(tool);
        float gw = UtilityZone::groundwaterAround(tileX, tileY, kind == UTIL_WELL ? WELL_RADIUS : PUMP_RADIUS);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.0f water/day here (%d%% groundwater)", UTIL_OUTPUT[kind] * gw,
                      static_cast<int>(gw * 100.f + 0.5f));
        toolbar.pillNote = buf;
    } else if (tool == TOOL_POWER_COAL && inWorldBounds(tileX, tileY)) {
        bool coal = false;
        for (const auto& o : FOOTPRINT_2X2)
            if (inWorldBounds(tileX + o[0], tileY + o[1]) && world[tileX + o[0]][tileY + o[1]].deposit == Deposit::Coal) coal = true;
        toolbar.pillNote = coal ? "on coal: 500 power" : "400 power (500 on coal)";
    }

    if (isRoadTool(tool)) {
        std::vector<sf::Vector2i> previewTiles;
        if (isDragging) previewTiles = computeDragTiles();
        else if (inWorldBounds(tileX, tileY)) previewTiles.push_back({tileX, tileY});
        std::vector<RoadStep> steps = planRoadPath(previewTiles, tool);

        // Ghost frame from the roads that would exist after the drag
        auto planned = [&](int x, int y) {
            for (const auto& st : steps)
                if (st.p.x == x && st.p.y == y) return st.action != ROAD_FAIL;
            return false;
        };
        const int tier = roadToolTier(tool);
        int total = 0;
        for (const RoadStep& st : steps) {
            const int gx = st.p.x, gy = st.p.y;
            sf::Color base = st.action == ROAD_FAIL    ? sf::Color(231, 76, 60)
                           : st.action == ROAD_BRIDGE  ? sf::Color(64, 160, 230)
                           : st.action == ROAD_UPGRADE ? sf::Color(240, 196, 60)
                           : st.action == ROAD_SKIP    ? sf::Color(150, 150, 150)
                                                       : sf::Color(46, 204, 113);
            highlightTile(gx, gy, uikit::withAlpha(base, 85), uikit::withAlpha(base, 230));
            if (st.action != ROAD_FAIL && st.action != ROAD_SKIP) total += st.cost;
            if (st.action == ROAD_SKIP) continue;

            if (st.action == ROAD_BRIDGE) {
                ghostSprite.setTexture(m_textures["road_bridge"]);
                ghostSprite.setTextureRect(sf::IntRect((tier * 2 + st.axis) * 32, 0, 32, 24));
            } else {
                int mask = 0;
                for (int d = 0; d < 4; d++) {
                    int nx = gx + ROAD_DIR[d][0], ny = gy + ROAD_DIR[d][1];
                    RoadTile* n = RoadTile::at(nx, ny);
                    bool linked = planned(nx, ny) || (n != NULL && n->allowsDir(d));
                    if (linked) mask |= 1 << d;
                }
                ghostSprite.setTexture(m_textures[roadSheetKey(tier)]);
                ghostSprite.setTextureRect(sf::IntRect(mask * 32, 0, 32, 16));
            }
            ghostSprite.setOrigin(0.f, 0.f);
            ghostSprite.setScale(2.f, 2.f);
            ghostSprite.setColor(st.action != ROAD_FAIL ? sf::Color(255, 255, 255, 200) : sf::Color(255, 120, 120, 150));
            ghostSprite.setPosition(tileScreenPos(gx, gy));
            _window.draw(ghostSprite);
        }
        if (isDragging) toolbar.dragCostPreview = total;
        return;
    }

    if (isZoneTool(tool) || tool == TOOL_DEZONE) {
        std::vector<sf::Vector2i> previewTiles;
        if (isDragging) previewTiles = computeDragTiles();
        else if (inWorldBounds(tileX, tileY)) previewTiles.push_back({tileX, tileY});
        const sf::Color sectorCol[3] = {sf::Color(72, 200, 110), sf::Color(70, 160, 235), sf::Color(235, 140, 50)};
        const sf::Color zc = tool == TOOL_DEZONE ? sf::Color(220, 110, 90) : sectorCol[zoneSector(zoneToolValue(tool))];
        int total = 0;
        for (const auto& pt : previewTiles) {
            std::string reason;
            bool ok = canPlaceTile(pt.x, pt.y, tool, &reason);
            if (!ok && reason.rfind("Insufficient", 0) != 0) continue;   // skipped tiles stay unmarked
            sf::Color base = ok ? zc : sf::Color(231, 76, 60);
            highlightTile(pt.x, pt.y, uikit::withAlpha(base, 100), uikit::withAlpha(base, 220));
            if (ok) total += getToolCost(tool);
        }
        if (isDragging) toolbar.dragCostPreview = total;
        if (isZoneTool(tool)) toolbar.pillNote = std::string(zoneDensityName(zoneDensity(zoneToolValue(tool)))) + ", grows to tier " +
                                                 std::to_string(zoneMaxTier(zoneToolValue(tool)));
        return;
    }

    if (isLineTool(tool)) {
        std::vector<sf::Vector2i> previewTiles;
        if (isDragging) previewTiles = computeDragTiles();
        else if (inWorldBounds(tileX, tileY)) previewTiles.push_back({tileX, tileY});
        int total = 0;
        const sf::Color lineCol = tool == TOOL_POWER_LINE ? sf::Color(250, 210, 60) : sf::Color(70, 170, 250);
        for (const auto& pt : previewTiles) {
            std::string reason;
            bool ok = canPlaceTile(pt.x, pt.y, tool, &reason);
            bool skip = !ok && (reason.rfind("Already", 0) == 0 || reason.rfind("Roads Already", 0) == 0);
            sf::Color base = ok ? lineCol : (skip ? sf::Color(150, 150, 160) : sf::Color(231, 76, 60));
            highlightTile(pt.x, pt.y, uikit::withAlpha(base, 90), uikit::withAlpha(base, 230));
            if (ok) total += getToolCost(tool);
        }
        if (isDragging) toolbar.dragCostPreview = total;
        toolbar.pillNote = tool == TOOL_POWER_LINE ? "carries power from plants" : "carries water, runs underground";
        return;
    }

    if (tool == TOOL_ROUNDABOUT) {
        if (!inWorldBounds(tileX, tileY)) return;
        bool ok = canPlaceTile(tileX, tileY, tool);
        sf::Color base = ok ? sf::Color(46, 204, 113) : sf::Color(231, 76, 60);
        highlightTile(tileX, tileY, uikit::withAlpha(base, 85), uikit::withAlpha(base, 230));
        if (ok) {
            RoadTile* r = RoadTile::at(tileX, tileY);
            ghostSprite.setTexture(m_textures["roundabout"]);
            ghostSprite.setTextureRect(sf::IntRect(r->mask * 32, 0, 32, 16));
            ghostSprite.setScale(2.f, 2.f);
            ghostSprite.setColor(sf::Color(255, 255, 255, 210));
            ghostSprite.setPosition(tileScreenPos(tileX, tileY));
            _window.draw(ghostSprite);
        }
        return;
    }

    if (tool == TOOL_WATER) {
        std::vector<sf::Vector2i> previewTiles;
        if (isDragging) {
            previewTiles = computeDragTiles();
        } else if (tileX >= 0 && tileX < tileMapW && tileY >= 0 && tileY < tileMapH) {
            previewTiles.push_back({tileX, tileY});
        }

        for (const auto& pt : previewTiles) {
            int gx = pt.x, gy = pt.y;
            if (gx < 0 || gx >= tileMapW || gy < 0 || gy >= tileMapH) continue;
            sf::Vector2f tp = tileScreenPos(gx, gy);
            float px = tp.x;
            float py = tp.y;

            std::string reason = "";
            bool ok = canPlaceTile(gx, gy, tool, &reason);
            sf::Color fillCol = ok ? sf::Color(46, 204, 113, 85) : sf::Color(231, 76, 60, 110);
            sf::Color outCol = ok ? sf::Color(46, 204, 113, 230) : sf::Color(231, 76, 60, 245);
            highlightTile(gx, gy, fillCol, outCol);

            std::string tkey = "water";
            if (m_textures.count(tkey)) {
                ghostSprite.setTexture(m_textures[tkey]);
                ghostSprite.setOrigin(0.f, 0.f);
                ghostSprite.setScale(2.f, 2.f);
                ghostSprite.setColor(ok ? sf::Color(255, 255, 255, 195) : sf::Color(255, 120, 120, 160));
                ghostSprite.setPosition(px, py);
                _window.draw(ghostSprite);
            }
        }
        return;
    }

    if (tool == TOOL_BULLDOZE) {
        std::vector<sf::Vector2i> previewTiles;
        if (isDragging) {
            previewTiles = computeDragTiles();
        } else if (tileX >= 0 && tileX < tileMapW && tileY >= 0 && tileY < tileMapH) {
            previewTiles.push_back({tileX, tileY});
        }

        for (const auto& pt : previewTiles) {
            int gx = pt.x, gy = pt.y;
            if (gx < 0 || gx >= tileMapW || gy < 0 || gy >= tileMapH) continue;
            {
                Zone* z = entities[gx][gy] ? dynamic_cast<Zone*>(entities[gx][gy]) : NULL;
                if (z != NULL && z->isMultiTile) {
                    int rx = z->isSubTile ? z->rootX : gx;
                    int ry = z->isSubTile ? z->rootY : gy;
                    const sf::Vector2i dOffsets[4] = {{0, 0}, {1, 0}, {-1, 1}, {0, 1}};
                    for (int k = 0; k < 4; k++) {
                        highlightTile(rx + dOffsets[k].x, ry + dOffsets[k].y, sf::Color(231, 76, 60, 130), sf::Color(231, 76, 60, 245));
                    }
                    continue;
                }
                bool canBulldoze = !isClearGroundName(tileKind(gx, gy));
                sf::Color fillCol = canBulldoze ? sf::Color(231, 76, 60, 130) : sf::Color(120, 120, 120, 60);
                sf::Color outCol = canBulldoze ? sf::Color(231, 76, 60, 245) : sf::Color(140, 140, 140, 150);
                highlightTile(gx, gy, fillCol, outCol);
            }
        }
        return;
    }

    // 2x2 Building Tools: Show individual 2:1 isometric diamond highlights and ghost building (True 2x2 Diamond)
    int gx = isDragging ? dragCurrent.x : tileX;
    int gy = isDragging ? dragCurrent.y : tileY;

    if (gx >= 1 && gx + 1 < tileMapW && gy >= 0 && gy + 1 < tileMapH) {
        {
            sf::Vector2f tp = tileScreenPos(gx, gy);
            float px = tp.x;
            float py = tp.y;

            std::string reason = "";
            bool ok = canPlaceTile(gx, gy, tool, &reason);
            sf::Color fillCol = ok ? sf::Color(46, 204, 113, 85) : sf::Color(231, 76, 60, 110);
            sf::Color outCol = ok ? sf::Color(46, 204, 113, 230) : sf::Color(231, 76, 60, 245);

            // Coverage radius of police / fire / clinic / bus depot
            if (isServiceTool(tool) && SERVICE_RADIUS[serviceToolKind(tool)] > 0) {
                const int r = SERVICE_RADIUS[serviceToolKind(tool)];
                for (int dx = -r; dx <= r; dx++)
                    for (int dy = -r; dy <= r; dy++)
                        if (dx * dx + dy * dy <= r * r && inWorldBounds(gx + dx, gy + dy))
                            highlightTile(gx + dx, gy + dy, sf::Color(120, 190, 255, 35), sf::Color(120, 190, 255, 60));
            }

            // Highlight all 4 constituent 2:1 isometric tile diamonds (True 2x2 Diamond)
            const sf::Vector2i dOffsets[4] = {{0, 0}, {1, 0}, {-1, 1}, {0, 1}};
            for (int k = 0; k < 4; k++) {
                highlightTile(gx + dOffsets[k].x, gy + dOffsets[k].y, fillCol, outCol);
            }

            std::string textureKey = "";
            switch (tool) {
                case TOOL_RES_COTTAGES:   textureKey = "res_t1_v1"; break;
                case TOOL_RES_APARTMENTS: textureKey = "res_t2_v1"; break;
                case TOOL_RES_TOWER:      textureKey = "res_t3_v1"; break;
                case TOOL_COM_GROCERY:    textureKey = "com_t1_v1"; break;
                case TOOL_COM_PLAZA:      textureKey = "com_t2_v1"; break;
                case TOOL_COM_TOWER:      textureKey = "com_t3_v1"; break;
                case TOOL_IND_WORKSHOP:   textureKey = "ind_t1_v1"; break;
                case TOOL_IND_FACTORY:    textureKey = "ind_t2_v1"; break;
                case TOOL_IND_TECH:       textureKey = "ind_t3_v1"; break;
                case TOOL_FARM_BARN:      textureKey = "farm_v1";   break;
                case TOOL_FARM_WINDMILL:  textureKey = "farm_v2";   break;
                case TOOL_SCHOOL:         textureKey = "school";    break;
                case TOOL_CHURCH:         textureKey = "church";    break;
                case TOOL_PARK:           textureKey = "park";      break;
                case TOOL_HARBOUR:        textureKey = "harbour";   break;
                default:
                    if (isUtilityTool(tool)) textureKey = utilityTextureKey(utilityToolKind(tool));
                    else if (isServiceTool(tool)) textureKey = serviceTextureKey(serviceToolKind(tool));
                    break;
            }

            if (!textureKey.empty() && m_textures.count(textureKey)) {
                ghostSprite.setTexture(m_textures[textureKey]);
                if (m_textures[textureKey].getSize().y > 40) {
                    ghostSprite.setOrigin(0.f, static_cast<float>(m_textures[textureKey].getSize().y - 40));
                } else if (m_textures[textureKey].getSize().y > 16) {
                    ghostSprite.setOrigin(0.f, static_cast<float>(m_textures[textureKey].getSize().y - 16));
                } else {
                    ghostSprite.setOrigin(0.f, 0.f);
                }
                ghostSprite.setScale(2.f, 2.f);
                ghostSprite.setColor(ok ? sf::Color(255, 255, 255, 195) : sf::Color(255, 120, 120, 150));
                ghostSprite.setPosition(px - 16.f, py);
                _window.draw(ghostSprite);
            }
        }
    }
}

// Flickering flames and smoke over a burning building
void Play_Scene::drawFire(Zone* z) {
    const float t = gameTime.getElapsedTime().asSeconds();
    const float cx = z->position->x + 32.f, base = z->position->y + 24.f;
    sf::ConvexShape flame(3);
    for (int k = 0; k < 5; k++) {
        float fx = cx - 28.f + 14.f * static_cast<float>(k);
        float h = 22.f + 10.f * std::sin(t * 9.f + static_cast<float>(k) * 1.7f) + 6.f * static_cast<float>(z->fireDays);
        flame.setPoint(0, sf::Vector2f(fx - 8.f, base));
        flame.setPoint(1, sf::Vector2f(fx + 8.f, base));
        flame.setPoint(2, sf::Vector2f(fx + 2.f * std::sin(t * 7.f + static_cast<float>(k)), base - h));
        flame.setFillColor(k % 2 ? sf::Color(255, 120, 30, 220) : sf::Color(255, 205, 60, 220));
        _window.draw(flame);
    }
    sf::CircleShape smoke(9.f);
    for (int k = 0; k < 3; k++) {
        float rise = std::fmod(t * 18.f + static_cast<float>(k) * 14.f, 42.f);
        smoke.setPosition(cx - 9.f + 6.f * std::sin(t + static_cast<float>(k)), base - 40.f - rise);
        smoke.setFillColor(sf::Color(70, 70, 75, static_cast<sf::Uint8>(160.f * (1.f - rise / 42.f))));
        _window.draw(smoke);
    }
}

// A pylon with its wires to the neighbouring pylons (drawn in the depth pass)
void Play_Scene::drawPylon(int x, int y) {
    sf::Vector2f tp = tileScreenPos(x, y);
    const float cx = tp.x + 32.f, cy = tp.y + 18.f, top = cy - 38.f;
    sf::RectangleShape pole(sf::Vector2f(4.f, 38.f));
    pole.setPosition(cx - 2.f, top);
    pole.setFillColor(sf::Color(92, 96, 104));
    _window.draw(pole);
    sf::RectangleShape arm(sf::Vector2f(20.f, 3.f));
    arm.setPosition(cx - 10.f, top + 3.f);
    arm.setFillColor(sf::Color(70, 74, 82));
    _window.draw(arm);
    // Wires to the pylons NE and SE (each pair once; SW / NW draw from the other end)
    sf::VertexArray wires(sf::Lines);
    for (int d = 0; d < 2; d++) {
        int nx = x + ROAD_DIR[d][0], ny = y + ROAD_DIR[d][1];
        if (!hasLine(nx, ny, LINE_POWER)) continue;
        sf::Vector2f np = tileScreenPos(nx, ny);
        for (float off : {-9.f, 9.f}) {
            wires.append(sf::Vertex(sf::Vector2f(cx + off, top + 4.f), sf::Color(35, 35, 40)));
            wires.append(sf::Vertex(sf::Vector2f(np.x + 32.f + off, np.y + 18.f - 34.f), sf::Color(35, 35, 40)));
        }
    }
    if (wires.getVertexCount() > 0) _window.draw(wires);
}

// Power / Water view: thick strokes along every conductor, from tile
// centre towards each connected neighbour. Lines are bright, roads (which
// carry both utilities) fainter.
void Play_Scene::renderUtilityLines(float viewL, float viewR, float viewT, float viewB, uint8_t line) {
    const sf::Color bright = line == LINE_POWER ? sf::Color(255, 215, 60, 240) : sf::Color(70, 175, 255, 240);
    const sf::Color faint = line == LINE_POWER ? sf::Color(255, 215, 60, 110) : sf::Color(70, 175, 255, 110);
    sf::VertexArray quads(sf::Quads);
    auto stroke = [&](sf::Vector2f a, sf::Vector2f b, float w, sf::Color c) {
        sf::Vector2f d = b - a;
        float len = std::sqrt(d.x * d.x + d.y * d.y);
        if (len <= 0.f) return;
        sf::Vector2f n(-d.y / len * w / 2.f, d.x / len * w / 2.f);
        quads.append(sf::Vertex(a + n, c)); quads.append(sf::Vertex(b + n, c));
        quads.append(sf::Vertex(b - n, c)); quads.append(sf::Vertex(a - n, c));
    };
    const VisibleTiles vis = visibleTileRange(viewL, viewR, viewT, viewB, 0.f);
    for (int i = vis.iMin; i <= vis.iMax; i++) {
        for (int j = vis.jMin(i); j <= vis.jMax(i); j++) {
            if (!conductsUtility(i, j, line)) continue;
            const bool own = (world[i][j].lines & line) != 0;
            sf::Vector2f tp = tileScreenPos(i, j);
            sf::Vector2f c(tp.x + 32.f, tp.y + 16.f);
            bool any = false;
            for (const auto& d : ROAD_DIR) {
                int nx = i + d[0], ny = j + d[1];
                if (!conductsUtility(nx, ny, line)) continue;
                bool nOwn = (world[nx][ny].lines & line) != 0;
                sf::Vector2f mid(c.x + 16.f * static_cast<float>(d[0]), c.y + 16.f * static_cast<float>(d[1]) + 8.f * static_cast<float>(d[0]));
                stroke(c, mid, own || nOwn ? 6.f : 3.f, own || nOwn ? bright : faint);
                any = true;
            }
            if (own) {
                // node dot, and a stub to any building it feeds
                stroke(sf::Vector2f(c.x - 4.f, c.y), sf::Vector2f(c.x + 4.f, c.y), 8.f, bright);
            } else if (!any) {
                stroke(sf::Vector2f(c.x - 3.f, c.y), sf::Vector2f(c.x + 3.f, c.y), 3.f, faint);
            }
        }
    }
    if (quads.getVertexCount() > 0) _window.draw(quads);
}

// Blinking "no power" / "no water" markers over a building that lacks them,
// and a warning over a utility that is cut off or has no transformer
void Play_Scene::renderUtilityIcons(Zone* z) {
    if (UtilityZone* u = dynamic_cast<UtilityZone*>(z)) {
        if ((u->linked && !u->noTransformer) || std::fmod(gameTime.getElapsedTime().asSeconds(), 1.2f) > 0.8f) return;
        uikit::icon(_window, "warning", z->position->x + 18.f, z->position->y - 34.f, 28.f);
        return;
    }
    if (utilities.grace || !z->isActive || !utilities.isConsumer(z->posX, z->posY)) return;
    const bool power = utilities.powered(z->posX, z->posY), water = utilities.watered(z->posX, z->posY);
    if (power && water) return;
    if (std::fmod(gameTime.getElapsedTime().asSeconds(), 1.2f) > 0.8f) return;
    float x = z->position->x + 16.f, y = z->position->y - 30.f;
    if (!power && !water) x -= 14.f;
    if (!power) { uikit::icon(_window, "no_power", x, y, 28.f); x += 28.f; }
    if (!water) uikit::icon(_window, "no_water", x, y, 28.f);
}
