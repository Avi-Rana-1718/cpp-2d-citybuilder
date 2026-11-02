#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
#include "../../src/Scene.hpp"
#include "../../src/_global.hpp"

#include "../entities/Player.hpp"
#include "../entities/_Zone.hpp"
#include "../entities/ResidentalZone.hpp"
#include "../entities/CommericalZone.hpp"
#include "../entities/IndustrialZone.hpp"
#include "../entities/FarmZone.hpp"
#include "../entities/HarbourZone.hpp"
#include "../entities/WaterTile.hpp"
#include "../entities/SchoolZone.hpp"
#include "../entities/ChurchZone.hpp"
#include "../entities/ParkZone.hpp"
#include "../entities/SubZone.hpp"
#include "../entities/roads/DirtRoad.hpp"
#include "../entities/GlassTile.hpp"
#include "../simulation/HappinessSystem.hpp"
#include "../ui/Window.hpp"
#include "../ui/Toolbar.hpp"

enum population {
    RESIDENTAL,
    COMMERICAL,
    INDUSTRIAL
};

enum populationStats {
    TAXES,
    POPULATION,
    EDUCATION,
    SAFETY,
    HEALTH,
    RECREATION,
    TRAFFIC,
    EASEOFACCESS,
    JOBS
};

class Play_Scene : public Scene {
    Entity* player = NULL;
    Entity* uiStats = NULL;
    Entity* uiMoney = NULL;
    Entity* uiTime = NULL;
    Entity* uiDebug = NULL;

    Toolbar toolbar;
    HappinessSystem happinessSystem;

    float lastActionTime = 0.f;
    float cameraSpeed = 600.f;
    float simAccumulator = 0.f;

    // Drag-to-multibuild state
    bool isDragging = false;
    bool wasLeftPressed = false;
    sf::Vector2i dragStart{-1, -1};
    sf::Vector2i dragCurrent{-1, -1};

public:
    ~Play_Scene();
    void init() override;
    void update(float deltaTime) override;

private:
    void handleCameraMovement(float deltaTime);
    void handleToolHotkeys();
    bool handleTileHover(const sf::Vector2i& mousePixel, bool mouseOverUI);
    void handleGestures(bool isHoveringTile, bool mouseOverUI);

    void renderWorld(float deltaTime);
    void renderDragPreview();
    void renderHUD(float deltaTime);

    void refreshUI();
    void populateCity();

    std::vector<sf::Vector2i> computeDragTiles() const;
    void placeTile(int x, int y, BuildTool tool);
    void executeMultibuild();
};

Play_Scene::~Play_Scene() {
    delete player;
    player = NULL;
}

void Play_Scene::init() {
    std::cout << "Scene created with name: Play_Scene\n";
 
    townDemand[0] = 1000;
    townDemand[1] = 500;
    townDemand[2] = 400;
    money = 5000;
    lastActionTime = 0.f;

    // Initialize HUD Panels
    uiStats = new Window;
    uiStats->text->setCharacterSize(14);
    uiStats->position->x = 20;
    uiStats->position->y = 20;
    ui.push_back(uiStats);

    uiMoney = new Window;
    uiMoney->text->setCharacterSize(18);
    uiMoney->position->x = 20;
    uiMoney->position->y = WINDOW_H - uiMoney->shape->getGlobalBounds().getSize().y - 40;
    ui.push_back(uiMoney);

    uiTime = new Window;
    uiTime->text->setCharacterSize(18);
    uiTime->position->x = 20 + uiMoney->shape->getGlobalBounds().getPosition().x + uiMoney->shape->getGlobalBounds().getSize().x;
    uiTime->position->y = WINDOW_H - uiMoney->text->getGlobalBounds().getSize().y - 40;
    ui.push_back(uiTime);

    uiDebug = new Window;
    uiDebug->text->setCharacterSize(15);
    uiDebug->position->x = WINDOW_W - uiDebug->shape->getGlobalBounds().getSize().x - 20;
    uiDebug->position->y = 20;
    ui.push_back(uiDebug);

    // Initialize Tile Cursor
    player = new Player;

    // Fill map with default grass tiles
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            entities[i][j] = new GrassTile(i, j);
        }
    }

    delete entities[10][4];
    entities[10][4] = new ResidentalZone(10, 4);

    // Initialize TheoTown bottom dock
    toolbar.init(static_cast<float>(WINDOW_W), static_cast<float>(WINDOW_H));

    // Center camera on city grid
    view.setSize(sf::Vector2f(static_cast<float>(WINDOW_W), static_cast<float>(WINDOW_H)));
    view.setCenter(sf::Vector2f(350.f, 470.f));
    _window.setView(view);
}

void Play_Scene::update(float deltaTime) {
    this->_deltaTime = deltaTime;

    // 1. Process Input
    handleCameraMovement(deltaTime);
    handleToolHotkeys();

    sf::Vector2i mousePixel = sf::Mouse::getPosition(_window);
    sf::Vector2f mousePixelF(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));

    // Check if mouse is interacting with UI elements
    bool mouseOverUI = toolbar.contains(mousePixelF);
    for (size_t i = 0; i < ui.size(); i++) {
        if (ui[i] != NULL && ui[i]->shape != NULL) {
            if (ui[i]->shape->getGlobalBounds().contains(mousePixelF)) {
                mouseOverUI = true;
                break;
            }
        }
    }

    bool isLeftPressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    if (isLeftPressed && !wasLeftPressed && mouseOverUI) {
        toolbar.handleClick(mousePixelF);
    }

    bool isHoveringTile = handleTileHover(mousePixel, mouseOverUI);
    handleGestures(isHoveringTile, mouseOverUI);

    refreshUI();

    // 2. Render Passes
    _window.clear(sf::Color(15, 20, 25));
    renderWorld(deltaTime);
    renderHUD(deltaTime);
    _window.display();

    // 3. Economy & Simulation Tick
    simAccumulator += deltaTime;
    if (simAccumulator >= 1.0f) {
        happinessSystem.updateSimulation();
        simAccumulator = 0.f;
    }

    if (std::rand() % 1000 == 7 && static_cast<int>(gameTime.getElapsedTime().asSeconds()) % 7 == 0) {
        populateCity();
    }
}

void Play_Scene::handleCameraMovement(float deltaTime) {
    sf::Vector2f camMove(0.f, 0.f);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
        camMove.y -= cameraSpeed * deltaTime;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        camMove.y += cameraSpeed * deltaTime;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        camMove.x -= cameraSpeed * deltaTime;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        camMove.x += cameraSpeed * deltaTime;
    }

    if (camMove.x != 0.f || camMove.y != 0.f) {
        view.move(camMove);
    }
}

void Play_Scene::handleToolHotkeys() {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
        toolbar.setActiveTool(TOOL_NONE);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
        toolbar.setActiveTool(TOOL_RESIDENTIAL);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::C)) {
        toolbar.setActiveTool(TOOL_COMMERCIAL);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::I)) {
        toolbar.setActiveTool(TOOL_INDUSTRIAL);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
        toolbar.setActiveTool(TOOL_FARM);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::H)) {
        toolbar.setActiveTool(TOOL_HARBOUR);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
        toolbar.setActiveTool(TOOL_WATER);
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

    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            if (entities[i][j] == NULL || entities[i][j]->position == NULL) continue;
            float px = entities[i][j]->position->x;
            float py = entities[i][j]->position->y;

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
            player->position->x = entities[tileX][tileY]->position->x;
            player->position->y = entities[tileX][tileY]->position->y;
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

    // Single hotkey tap fallback
    if (!isDragging && isHoveringTile && player != NULL && player->isActive) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::R) && now > lastActionTime + 0.15f) {
            placeTile(tileX, tileY, TOOL_RESIDENTIAL);
            lastActionTime = now;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && now > lastActionTime + 0.15f) {
            placeTile(tileX, tileY, TOOL_ROAD);
            lastActionTime = now;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::C) && now > lastActionTime + 0.15f) {
            placeTile(tileX, tileY, TOOL_COMMERCIAL);
            lastActionTime = now;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::I) && now > lastActionTime + 0.15f) {
            placeTile(tileX, tileY, TOOL_INDUSTRIAL);
            lastActionTime = now;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::F) && now > lastActionTime + 0.15f) {
            placeTile(tileX, tileY, TOOL_FARM);
            lastActionTime = now;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1) && now > lastActionTime + 0.15f) {
            placeTile(tileX, tileY, TOOL_SCHOOL);
            lastActionTime = now;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2) && now > lastActionTime + 0.15f) {
            placeTile(tileX, tileY, TOOL_CHURCH);
            lastActionTime = now;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3) && now > lastActionTime + 0.15f) {
            placeTile(tileX, tileY, TOOL_PARK);
            lastActionTime = now;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::H) && now > lastActionTime + 0.15f) {
            placeTile(tileX, tileY, TOOL_HARBOUR);
            lastActionTime = now;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && now > lastActionTime + 0.15f) {
            placeTile(tileX, tileY, TOOL_WATER);
            lastActionTime = now;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::X) && now > lastActionTime + 0.15f) {
            placeTile(tileX, tileY, TOOL_BULLDOZE);
            lastActionTime = now;
        }
    }
}

std::vector<sf::Vector2i> Play_Scene::computeDragTiles() const {
    std::vector<sf::Vector2i> tiles;
    if (dragStart.x < 0 || dragStart.y < 0 || dragCurrent.x < 0 || dragCurrent.y < 0) {
        return tiles;
    }

    if (toolbar.getActiveTool() == TOOL_ROAD) {
        int stepX = (dragCurrent.x >= dragStart.x) ? 1 : -1;
        int stepY = (dragCurrent.y >= dragStart.y) ? 1 : -1;

        for (int x = dragStart.x; ; x += stepX) {
            tiles.push_back({x, dragStart.y});
            if (x == dragCurrent.x) break;
        }
        for (int y = dragStart.y + stepY; ; y += stepY) {
            if ((stepY > 0 && y > dragCurrent.y) || (stepY < 0 && y < dragCurrent.y)) break;
            tiles.push_back({dragCurrent.x, y});
            if (y == dragCurrent.y) break;
        }
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

void Play_Scene::placeTile(int x, int y, BuildTool tool) {
    if (x < 0 || x >= tileMapW || y < 0 || y >= tileMapH) return;
    if (tool == TOOL_NONE) return;

    bool is2x2 = (tool == TOOL_SCHOOL || tool == TOOL_CHURCH);
    if (is2x2) {
        if (x + 1 >= tileMapW || y + 1 >= tileMapH) return;
    }

    int cost = 0;
    switch (tool) {
        case TOOL_RESIDENTIAL: cost = 100; break;
        case TOOL_COMMERCIAL:  cost = 100; break;
        case TOOL_INDUSTRIAL:  cost = 120; break;
        case TOOL_FARM:        cost = 60;  break;
        case TOOL_ROAD:        cost = 10;  break;
        case TOOL_HARBOUR:     cost = 250; break;
        case TOOL_WATER:       cost = 20;  break;
        case TOOL_SCHOOL:      cost = 200; break;
        case TOOL_CHURCH:      cost = 180; break;
        case TOOL_PARK:        cost = 80;  break;
        case TOOL_BULLDOZE:    cost = 5;   break;
        default: return;
    }

    if (tool == TOOL_BULLDOZE) {
        if (entities[x][y] == NULL || entities[x][y]->getName() == "GrassTile") {
            return;
        }
        if (money < cost) return;

        Zone* z = dynamic_cast<Zone*>(entities[x][y]);
        if (z != NULL && z->isMultiTile) {
            int rx = z->isSubTile ? z->rootX : x;
            int ry = z->isSubTile ? z->rootY : y;
            if (rx >= 0 && rx + 1 < tileMapW && ry >= 0 && ry + 1 < tileMapH) {
                for (int dx = 0; dx <= 1; dx++) {
                    for (int dy = 0; dy <= 1; dy++) {
                        delete entities[rx + dx][ry + dy];
                        entities[rx + dx][ry + dy] = new GrassTile(rx + dx, ry + dy);
                    }
                }
            }
        } else {
            delete entities[x][y];
            entities[x][y] = new GrassTile(x, y);
        }
        money -= cost;
        return;
    }

    if (money < cost) return;

    if (is2x2) {
        // Clear all 4 tiles in the 2x2 area, clearing any overlapping multi-tile structures properly
        for (int dx = 0; dx <= 1; dx++) {
            for (int dy = 0; dy <= 1; dy++) {
                int cx = x + dx;
                int cy = y + dy;
                Zone* ez = dynamic_cast<Zone*>(entities[cx][cy]);
                if (ez != NULL && ez->isMultiTile) {
                    int rx = ez->isSubTile ? ez->rootX : cx;
                    int ry = ez->isSubTile ? ez->rootY : cy;
                    if (rx >= 0 && rx + 1 < tileMapW && ry >= 0 && ry + 1 < tileMapH && (rx != x || ry != y)) {
                        for (int mdx = 0; mdx <= 1; mdx++) {
                            for (int mdy = 0; mdy <= 1; mdy++) {
                                int sx = rx + mdx;
                                int sy = ry + mdy;
                                if ((sx < x || sx > x + 1) || (sy < y || sy > y + 1)) {
                                    delete entities[sx][sy];
                                    entities[sx][sy] = new GrassTile(sx, sy);
                                }
                            }
                        }
                    }
                }
                delete entities[cx][cy];
                entities[cx][cy] = NULL;
            }
        }

        money -= cost;

        std::string typeName = (tool == TOOL_SCHOOL) ? "SchoolZone" : "ChurchZone";
        if (tool == TOOL_SCHOOL) {
            entities[x][y] = new SchoolZone(x, y);
        } else {
            entities[x][y] = new ChurchZone(x, y);
        }

        entities[x + 1][y]     = new SubZone(x + 1, y, x, y, typeName);
        entities[x][y + 1]     = new SubZone(x, y + 1, x, y, typeName);
        entities[x + 1][y + 1] = new SubZone(x + 1, y + 1, x, y, typeName);
        return;
    }

    // 1x1 placement: If replacing an existing multi-tile tile, reset its siblings to GrassTile
    Zone* ez = dynamic_cast<Zone*>(entities[x][y]);
    if (ez != NULL && ez->isMultiTile) {
        int rx = ez->isSubTile ? ez->rootX : x;
        int ry = ez->isSubTile ? ez->rootY : y;
        if (rx >= 0 && rx + 1 < tileMapW && ry >= 0 && ry + 1 < tileMapH) {
            for (int dx = 0; dx <= 1; dx++) {
                for (int dy = 0; dy <= 1; dy++) {
                    if (rx + dx == x && ry + dy == y) continue;
                    delete entities[rx + dx][ry + dy];
                    entities[rx + dx][ry + dy] = new GrassTile(rx + dx, ry + dy);
                }
            }
        }
    }

    money -= cost;

    delete entities[x][y];
    entities[x][y] = NULL;

    switch (tool) {
        case TOOL_RESIDENTIAL:
            entities[x][y] = new ResidentalZone(x, y);
            break;
        case TOOL_COMMERCIAL:
            entities[x][y] = new CommericalZone(x, y);
            break;
        case TOOL_INDUSTRIAL:
            entities[x][y] = new IndustrialZone(x, y);
            break;
        case TOOL_FARM:
            entities[x][y] = new FarmZone(x, y);
            break;
        case TOOL_ROAD:
            entities[x][y] = new DirtRoad(x, y);
            break;
        case TOOL_HARBOUR:
            entities[x][y] = new HarbourZone(x, y);
            break;
        case TOOL_WATER:
            entities[x][y] = new WaterTile(x, y);
            break;
        case TOOL_PARK:
            entities[x][y] = new ParkZone(x, y);
            break;
        default:
            break;
    }
}

void Play_Scene::executeMultibuild() {
    BuildTool tool = toolbar.getActiveTool();
    if (tool == TOOL_NONE) return;

    auto tiles = computeDragTiles();
    for (const auto& pt : tiles) {
        placeTile(pt.x, pt.y, tool);
    }

    // Refresh road orientations across map
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            if (entities[i][j] != NULL && entities[i][j]->getName() == "DirtRoad") {
                DirtRoad* road = dynamic_cast<DirtRoad*>(entities[i][j]);
                if (road != NULL) road->check();
            }
        }
    }
}

void Play_Scene::renderWorld(float deltaTime) {
    _window.setView(view);

    for (size_t i = 0; i < bg.size(); i++) {
        if (bg[i] != NULL) bg[i]->update(deltaTime);
    }
    for (int i = 0; i < tileMapW; i++) {
        for (int j = 0; j < tileMapH; j++) {
            if (entities[i][j] != NULL)
                entities[i][j]->update(deltaTime);
        }
    }

    renderDragPreview();

    if (player != NULL && player->isActive && !isDragging) {
        player->update(deltaTime);
    }
}

void Play_Scene::renderDragPreview() {
    BuildTool tool = toolbar.getActiveTool();
    if (tool == TOOL_NONE) return;

    sf::Sprite ghostSprite;
    sf::RectangleShape ghostRect(sf::Vector2f(static_cast<float>(tileSizeX), static_cast<float>(tileSizeY)));

    // Multi-tile preview for 2x2 School & Church
    if (tool == TOOL_SCHOOL || tool == TOOL_CHURCH) {
        int gx = isDragging ? dragCurrent.x : tileX;
        int gy = isDragging ? dragCurrent.y : tileY;

        if (gx >= 0 && gx + 1 < tileMapW && gy >= 0 && gy + 1 < tileMapH) {
            if (entities[gx][gy] != NULL && entities[gx][gy]->position != NULL) {
                float px = entities[gx][gy]->position->x;
                float py = entities[gx][gy]->position->y;

                std::string tkey = (tool == TOOL_SCHOOL) ? "school" : "church";
                float originY = (tool == TOOL_SCHOOL) ? 24.f : 28.f;
                sf::Color tint = (tool == TOOL_SCHOOL) ? sf::Color(255, 180, 180, 210) : sf::Color(220, 160, 255, 210);

                if (m_textures.count(tkey)) {
                    ghostSprite.setTexture(m_textures[tkey]);
                    ghostSprite.setOrigin(0.f, originY);
                    ghostSprite.setScale(2.f, 2.f);
                    ghostSprite.setColor(tint);
                    ghostSprite.setPosition(px, py);
                    _window.draw(ghostSprite);
                }
            }
        }
        return;
    }

    if (!isDragging) return;
    auto dragTiles = computeDragTiles();

    for (const auto& pt : dragTiles) {
        int gx = pt.x;
        int gy = pt.y;
        if (gx < 0 || gx >= tileMapW || gy < 0 || gy >= tileMapH) continue;
        if (entities[gx][gy] == NULL || entities[gx][gy]->position == NULL) continue;

        float px = entities[gx][gy]->position->x;
        float py = entities[gx][gy]->position->y;

        std::string textureKey = "";
        sf::Color tint(255, 255, 255, 190);

        switch (tool) {
            case TOOL_RESIDENTIAL:
                textureKey = "residental";
                tint = sf::Color(100, 255, 100, 190);
                break;
            case TOOL_COMMERCIAL:
                textureKey = "commercial";
                tint = sf::Color(255, 240, 100, 190);
                break;
            case TOOL_INDUSTRIAL:
                textureKey = "industrial";
                tint = sf::Color(255, 180, 100, 190);
                break;
            case TOOL_FARM:
                textureKey = "farm";
                tint = sf::Color(160, 255, 140, 190);
                break;
            case TOOL_HARBOUR:
                textureKey = "harbour";
                tint = sf::Color(100, 220, 255, 190);
                break;
            case TOOL_WATER:
                textureKey = "water";
                tint = sf::Color(100, 180, 255, 190);
                break;
            case TOOL_PARK:
                textureKey = "park";
                tint = sf::Color(120, 255, 160, 190);
                break;
            case TOOL_ROAD:
                textureKey = "road";
                tint = sf::Color(220, 220, 220, 190);
                break;
            case TOOL_BULLDOZE:
                ghostRect.setPosition(px, py);
                ghostRect.setFillColor(sf::Color(255, 60, 60, 160));
                ghostRect.setOutlineColor(sf::Color::Red);
                ghostRect.setOutlineThickness(1.5f);
                _window.draw(ghostRect);
                continue;
            default:
                continue;
        }

        if (!textureKey.empty() && m_textures.count(textureKey)) {
            ghostSprite.setTexture(m_textures[textureKey]);
            ghostSprite.setScale(2.f, 2.f);
            ghostSprite.setColor(tint);
            ghostSprite.setPosition(px, py);
            _window.draw(ghostSprite);
        }
    }
}

void Play_Scene::renderHUD(float deltaTime) {
    _window.setView(_window.getDefaultView());

    for (size_t i = 0; i < ui.size(); i++) {
        if (ui[i] != NULL) ui[i]->update(deltaTime);
    }

    sf::Vector2i mousePixel = sf::Mouse::getPosition(_window);
    sf::Vector2f mousePixelF(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));
    toolbar.draw(_window, m_fonts["../assets/Hack.ttf"], mousePixelF);
}

void Play_Scene::refreshUI() {
    int totalSec = static_cast<int>(gameTime.getElapsedTime().asSeconds());
    yr = totalSec / 1800;
    mon = (totalSec - (yr * 1800)) / 155;
    day = (totalSec - (yr * 1800) - (mon * 155)) / 5;

    if (uiDebug != NULL && uiDebug->text != NULL && uiDebug->shape != NULL) {
        std::string posStr = (player != NULL && player->isActive)
            ? (std::to_string(tileX) + " " + std::to_string(tileY))
            : "None";

        std::string toolName = toolbar.getActiveToolName();

        std::string debugStr = "Tool: " + toolName + "\n";
        if (isDragging) {
            auto tiles = computeDragTiles();
            debugStr += "Drag: " + std::to_string(tiles.size()) + " tiles\n";
        } else if (toolbar.getActiveTool() == TOOL_NONE) {
            debugStr += "Click category/tool below\n";
        } else {
            debugStr += "Drag map to multibuild\n";
        }
        debugStr += "Tile: " + posStr + " | FPS: " + std::to_string(calculatedFPS);

        uiDebug->text->setString(debugStr);
        uiDebug->position->x = WINDOW_W - uiDebug->shape->getGlobalBounds().getSize().x - 20;
    }
    if (uiTime != NULL && uiTime->text != NULL && uiMoney != NULL && uiMoney->shape != NULL) {
        uiTime->text->setString(std::to_string(day) + "/" + std::to_string(mon) + "/" + std::to_string(yr));
        uiTime->position->x = 40 + uiMoney->shape->getGlobalBounds().getPosition().x + uiMoney->shape->getGlobalBounds().getSize().x;
    }
    if (uiMoney != NULL && uiMoney->text != NULL) {
        uiMoney->text->setString("$" + std::to_string(money));
    }
    if (uiStats != NULL && uiStats->text != NULL) {
        int happy = happinessSystem.cityHappiness;
        std::string happyIcon = (happy >= 75) ? "[Great :D]" : ((happy >= 50) ? "[Fair :)]" : "[Low :(]");
        uiStats->text->setString(
            " CITY STATS  " + happyIcon + "\n"
            " Happiness:    " + std::to_string(happy) + "%\n"
            "  - Education: " + std::to_string(townStats[2]) + "%\n"
            "  - Safety:    " + std::to_string(townStats[3]) + "%\n"
            "  - Health:    " + std::to_string(townStats[4]) + "%\n"
            "  - Jobs:      " + std::to_string(townStats[8]) + "%\n\n"
            " POPULATION / CAPACITY\n"
            " Res: " + std::to_string(townPopulation[0]) + " / " + std::to_string(townCapacity[0]) + "\n"
            " Com: " + std::to_string(townPopulation[1]) + " / " + std::to_string(townCapacity[1]) + "\n"
            " Ind: " + std::to_string(townPopulation[2]) + " / " + std::to_string(townCapacity[2]) + "\n\n"
            " TAX RATES\n"
            " Res: " + std::to_string(townTaxes[0]) + "% | Com: " + std::to_string(townTaxes[1]) + "% | Ind: " + std::to_string(townTaxes[2]) + "%"
        );
    }
}

void Play_Scene::populateCity() {
    for (int i = 0; i < 3; i++) {
        int k = townCapacity[i] - townPopulation[i];
        if (townDemand[i] <= 0 || k <= 0) {
            continue;
        }

        int populationInc = (k > 10) ? (std::rand() % (k / 10) + 1) : (std::rand() % k + 1);
        townPopulation[i] += populationInc;
        townDemand[i] -= populationInc;
    }
}
