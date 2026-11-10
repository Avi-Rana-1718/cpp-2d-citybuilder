#pragma once
// Play_Scene member definitions (included at the end of Play_Scene.hpp).
// Input: camera, hotkeys, tile hover, drag gestures, speed buttons

// Pause / 1x / 2x / 4x buttons, right side of the top bar
sf::FloatRect Play_Scene::speedButtonBounds(int speedIndex) const {
    return sf::FloatRect(966.f + uikit::topBarShift() + static_cast<float>(speedIndex) * 38.f, 10.f, 34.f, 28.f);
}

bool Play_Scene::handleSpeedClick(const sf::Vector2f& point) {
    for (int i = 0; i < GameClock::SPEED_COUNT; i++) {
        if (speedButtonBounds(i).contains(point)) {
            gameClock.setSpeedIndex(i);
            return true;
        }
    }
    return false;
}

void Play_Scene::handleCameraMovement(float deltaTime) {
    float curSpeed = cameraSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
        curSpeed *= 2.2f;
    }

    sf::Vector2f camMove(0.f, 0.f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
        camMove.y -= curSpeed * deltaTime;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        camMove.y += curSpeed * deltaTime;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        camMove.x -= curSpeed * deltaTime;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        camMove.x += curSpeed * deltaTime;
    }

    if (camMove.x != 0.f || camMove.y != 0.f) {
        view.move(camMove);
    }

    // Zoom controls: Q / PageUp / Equal to zoom in, E / PageDown / Dash to zoom out
    bool zoomChanged = false;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q) || sf::Keyboard::isKeyPressed(sf::Keyboard::PageUp) || sf::Keyboard::isKeyPressed(sf::Keyboard::Equal)) {
        currentZoom = std::max(0.4f, currentZoom - 0.9f * deltaTime);
        zoomChanged = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::E) || sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown) || sf::Keyboard::isKeyPressed(sf::Keyboard::Dash)) {
        currentZoom = std::min(3.2f, currentZoom + 0.9f * deltaTime);
        zoomChanged = true;
    }

    if (zoomChanged) {
        view.setSize(uikit::SW() * currentZoom, uikit::SH() * currentZoom);
    }
}

void Play_Scene::handleToolHotkeys() {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
        toolbar.setActiveTool(TOOL_NONE);
        setBudgetOpen(false);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
        toolbar.setActiveTool(TOOL_ZONE_R_LOW);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::C)) {
        toolbar.setActiveTool(TOOL_ZONE_C_LOW);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::I)) {
        toolbar.setActiveTool(TOOL_ZONE_I_LOW);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
        toolbar.setActiveTool(TOOL_FARM);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::H)) {
        toolbar.setActiveTool(TOOL_HARBOUR);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && !sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) && !sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
        // Only trigger water tool if shift is not pressed (to allow WASD fast pan)
        if (toolbar.getActiveTool() == TOOL_NONE) {
            toolbar.setActiveTool(TOOL_WATER);
        }
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) {
        toolbar.setActiveTool(TOOL_SCHOOL);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) {
        toolbar.setActiveTool(TOOL_CHURCH);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) {
        toolbar.setActiveTool(TOOL_PARK);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
        toolbar.setActiveTool(TOOL_ROAD);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::X)) {
        toolbar.setActiveTool(TOOL_BULLDOZE);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::U)) {
        toolbar.setActiveTool(TOOL_POWER_COAL);
    }
    // Time: P pause/resume, [ slower, ] faster (edge-triggered)
    bool pauseKey = sf::Keyboard::isKeyPressed(sf::Keyboard::P);
    bool slowerKey = sf::Keyboard::isKeyPressed(sf::Keyboard::LBracket);
    bool fasterKey = sf::Keyboard::isKeyPressed(sf::Keyboard::RBracket);
    if (pauseKey && !wasPauseKey) gameClock.togglePause();
    if (slowerKey && !wasSlowerKey) gameClock.slower();
    if (fasterKey && !wasFasterKey) gameClock.faster();
    wasPauseKey = pauseKey;
    wasSlowerKey = slowerKey;
    wasFasterKey = fasterKey;

    // F5 save, F9 load (city.sav, else the autosave), Ctrl+Z / Z undo, L messages
    bool saveKey = sf::Keyboard::isKeyPressed(sf::Keyboard::F5);
    bool loadKey = sf::Keyboard::isKeyPressed(sf::Keyboard::F9);
    bool undoKey = sf::Keyboard::isKeyPressed(sf::Keyboard::Z);
    bool logKey = sf::Keyboard::isKeyPressed(sf::Keyboard::L);
    if (saveKey && !wasSaveKey) {
        if (saveCity("city.sav")) notify("City saved (city.sav) - F9 loads it", false);
        else notify("Could not save city.sav", true);
    }
    if (loadKey && !wasLoadKey) {
        std::ifstream probe("city.sav");
        const std::string path = probe ? "city.sav" : "autosave.sav";
        probe.close();
        if (loadCity(path)) { toolbar.setActiveTool(TOOL_NONE); notify("Loaded " + path, false); }
        else showToast("No save to load (F5 saves)", true);
    }
    if (undoKey && !wasUndoKey) undoLast();
    if (logKey && !wasLogKey) logOpen = !logOpen;
    wasSaveKey = saveKey;
    wasLoadKey = loadKey;
    wasUndoKey = undoKey;
    wasLogKey = logKey;

    // O cycles the map view (edge-triggered)
    bool overlayKey = sf::Keyboard::isKeyPressed(sf::Keyboard::O);
    if (overlayKey && !wasOverlayKeyPressed) {
        overlayMode = (overlayMode + 1) % OVERLAY_COUNT;
    }
    wasOverlayKeyPressed = overlayKey;
}

bool Play_Scene::handleTileHover(const sf::Vector2i& mousePixel, bool mouseOverUI) {
    if (mouseOverUI) {
        if (player != NULL && !isDragging) {
            player->isActive = false;
        }
        return false;
    }

    sf::Vector2f mouseWorld = _window.mapPixelToCoords(mousePixel, view);
    int hoveredX = -1;
    int hoveredY = -1;

    // Fast O(1) inverse isometric coordinate estimate
    int estI = static_cast<int>(std::round((mouseWorld.x - 32.f) / 32.f));
    int estJ = static_cast<int>(std::round((mouseWorld.y - 16.f * estI - 16.f) / 32.f));

    int minI = std::max(0, estI - 2);
    int maxI = std::min(tileMapW - 1, estI + 2);
    int minJ = std::max(0, estJ - 2);
    int maxJ = std::min(tileMapH - 1, estJ + 2);

    for (int i = minI; i <= maxI; i++) {
        for (int j = minJ; j <= maxJ; j++) {
            sf::Vector2f tp = tileScreenPos(i, j);
            float px = tp.x;
            float py = tp.y;

            if (mouseWorld.x >= px && mouseWorld.x < px + tileSizeX &&
                mouseWorld.y >= py && mouseWorld.y < py + tileSizeY) {
                float dx = std::abs(mouseWorld.x - (px + tileSizeX / 2.0f));
                float dy = std::abs(mouseWorld.y - (py + tileSizeY / 2.0f));
                if ((dx / (tileSizeX / 2.0f)) + (dy / (tileSizeY / 2.0f)) <= 1.0f) {
                    hoveredX = i;
                    hoveredY = j;
                    break;
                }
            }
        }
        if (hoveredX != -1) break;
    }

    if (hoveredX != -1 && hoveredY != -1) {
        tileX = hoveredX;
        tileY = hoveredY;
        if (player != NULL) {
            player->isActive = true;
            sf::Vector2f tp = tileScreenPos(tileX, tileY);
            player->position->x = tp.x;
            player->position->y = tp.y;
        }
        return true;
    }

    if (player != NULL && !isDragging) {
        player->isActive = false;
    }
    return false;
}

void Play_Scene::handleGestures(bool isHoveringTile, bool mouseOverUI) {
    float now = gameTime.getElapsedTime().asSeconds();
    bool isLeftPressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    bool isRightPressed = sf::Mouse::isButtonPressed(sf::Mouse::Right);

    // Right-click: abort drag or deselect tool
    if (isRightPressed) {
        if (isDragging) {
            isDragging = false;
            dragStart = {-1, -1};
        } else if (toolbar.getActiveTool() != TOOL_NONE) {
            toolbar.setActiveTool(TOOL_NONE);
        }
    }

    // Left-click Drag Gestures (World Only)
    if (!mouseOverUI && toolbar.getActiveTool() != TOOL_NONE) {
        if (isLeftPressed && !wasLeftPressed) {
            if (isHoveringTile) {
                isDragging = true;
                dragStart = {tileX, tileY};
                dragCurrent = {tileX, tileY};
            }
        } else if (isLeftPressed && wasLeftPressed) {
            if (isDragging && isHoveringTile) {
                dragCurrent = {tileX, tileY};
            }
        } else if (!isLeftPressed && wasLeftPressed) {
            if (isDragging) {
                if (isHoveringTile) {
                    dragCurrent = {tileX, tileY};
                }
                executeMultibuild();
                isDragging = false;
                dragStart = {-1, -1};
                lastActionTime = now;
            }
        }
    } else if (isLeftPressed && !wasLeftPressed && mouseOverUI) {
        isDragging = false;
    }

    wasLeftPressed = isLeftPressed;
}

std::vector<sf::Vector2i> Play_Scene::computeDragTiles() const {
    std::vector<sf::Vector2i> tiles;
    if (dragStart.x < 0 || dragStart.y < 0 || dragCurrent.x < 0 || dragCurrent.y < 0) {
        return tiles;
    }

    if (isRoadTool(toolbar.getActiveTool()) || isLineTool(toolbar.getActiveTool())) {
        // Edge-connected L (see roadDragPath)
        for (const auto& p : roadDragPath(dragStart, dragCurrent))
            if (inWorldBounds(p.x, p.y)) tiles.push_back(p);
    } else {
        int minX = std::max(0, std::min(dragStart.x, dragCurrent.x));
        int maxX = std::min(tileMapW - 1, std::max(dragStart.x, dragCurrent.x));
        int minY = std::max(0, std::min(dragStart.y, dragCurrent.y));
        int maxY = std::min(tileMapH - 1, std::max(dragStart.y, dragCurrent.y));

        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                tiles.push_back({x, y});
            }
        }
    }

    return tiles;
}
