#pragma once
// Play_Scene member definitions (included at the end of Play_Scene.hpp).
// Day / night lighting and the minimap.

// ---- Day and night ----
// One light cycle lasts DAYNIGHT_DAYS game days. Darkness is 0 by day and
// rises to 1 in the middle of the night (about a third of the cycle).
float Play_Scene::nightAmount() const {
    if (!settings.dayNight) return 0.f;
    const float t = (static_cast<float>(gameClock.getTotalDays()) + gameClock.getDayProgress()) / DAYNIGHT_DAYS;
    const float light = 0.5f + 0.5f * std::cos(2.f * 3.14159265f * (t - std::floor(t) - 0.5f));   // 1 noon, 0 midnight
    return std::max(0.f, std::min(1.f, (0.45f - light) / 0.35f));
}

// After the world pass (world view active): a blue wash over the map, then
// lit windows and street lights on top of it
void Play_Scene::renderNight(float viewL, float viewR, float viewT, float viewB) {
    const float night = nightAmount();
    if (night <= 0.01f || overlayMode != 0) return;
    sf::RectangleShape wash(sf::Vector2f(viewR - viewL, viewB - viewT));
    wash.setPosition(viewL, viewT);
    wash.setFillColor(sf::Color(12, 18, 48, static_cast<sf::Uint8>(135.f * night)));
    _window.draw(wash);

    sf::VertexArray lights(sf::Quads);
    auto glow = [&](float x, float y, float r, sf::Color c) {
        sf::Color edge(c.r, c.g, c.b, 0);
        // a small diamond, bright in the middle
        lights.append(sf::Vertex(sf::Vector2f(x, y - r), edge));
        lights.append(sf::Vertex(sf::Vector2f(x + r * 1.6f, y), c));
        lights.append(sf::Vertex(sf::Vector2f(x, y + r), edge));
        lights.append(sf::Vertex(sf::Vector2f(x - r * 1.6f, y), c));
    };
    const sf::Uint8 a = static_cast<sf::Uint8>(230.f * night);
    const VisibleTiles vis = visibleTileRange(viewL, viewR, viewT, viewB, 96.f);
    for (int i = vis.iMin; i <= vis.iMax; i++) {
        for (int j = vis.jMin(i); j <= vis.jMax(i); j++) {
            if (entities[i][j] == NULL) continue;
            sf::Vector2f tp = tileScreenPos(i, j);
            if (RoadTile::at(i, j) != NULL) {
                if ((i + j) % 3 == 0) glow(tp.x + 32.f, tp.y + 16.f, 9.f, sf::Color(255, 210, 120, static_cast<sf::Uint8>(a * 0.45f)));
                continue;
            }
            Zone* z = dynamic_cast<Zone*>(entities[i][j]);
            if (z == NULL || z->isSubTile || z->population <= 0 || !z->isActive) continue;
            if (!utilities.powered(i, j)) continue;   // dark without power
            // Windows: a few per building, more for bigger ones, placed by a position hash
            unsigned h = static_cast<unsigned>(i) * 73856093u ^ static_cast<unsigned>(j) * 19349663u;
            const int count = 2 + z->tier * 2;
            const float height = 14.f + 22.f * static_cast<float>(z->tier - 1);
            for (int k = 0; k < count; k++) {
                h = h * 1664525u + 1013904223u;
                float wx = tp.x + 32.f + static_cast<float>(static_cast<int>(h % 41u) - 20);
                float wy = tp.y + 18.f - static_cast<float>((h >> 8) % static_cast<unsigned>(height));
                sf::Color c = (h >> 16) % 3 == 0 ? sf::Color(255, 240, 190, a) : sf::Color(255, 205, 110, a);
                lights.append(sf::Vertex(sf::Vector2f(wx - 1.5f, wy - 1.5f), c));
                lights.append(sf::Vertex(sf::Vector2f(wx + 1.5f, wy - 1.5f), c));
                lights.append(sf::Vertex(sf::Vector2f(wx + 1.5f, wy + 1.5f), c));
                lights.append(sf::Vertex(sf::Vector2f(wx - 1.5f, wy + 1.5f), c));
            }
        }
    }
    if (lights.getVertexCount() > 0) _window.draw(lights, sf::BlendAdd);
}

// ---- Minimap ----
sf::FloatRect Play_Scene::minimapBounds() const {
    const float w = 132.f, h = 196.f;
    return sf::FloatRect(uikit::SW() - w - 12.f, uikit::SH() - 72.f - h, w, h);
}

// World (screen-space) extent of the whole map: X = 32x, Y = 32y + 16x
static sf::FloatRect worldExtent() {
    return sf::FloatRect(0.f, 0.f, 32.f * tileMapW + 32.f, 32.f * tileMapH + 16.f * tileMapW + 16.f);
}

sf::Color Play_Scene::minimapColor(int x, int y) const {
    if (RoadTile* r = RoadTile::at(x, y)) return r->isBridge() ? sf::Color(150, 140, 120) : sf::Color(70, 72, 80);
    if (entities[x][y] != NULL) {
        Zone* z = dynamic_cast<Zone*>(entities[x][y]);
        if (z != NULL && z->isSubTile && inWorldBounds(z->rootX, z->rootY)) z = dynamic_cast<Zone*>(entities[z->rootX][z->rootY]);
        if (z != NULL) {
            const std::string n = z->getName();
            if (n == "ResidentalZone") return sf::Color(80, 200, 110);
            if (n == "CommericalZone") return sf::Color(80, 160, 240);
            if (n == "IndustrialZone" || n == "MineZone") return sf::Color(235, 145, 60);
            if (n == "FarmZone") return sf::Color(190, 200, 90);
            if (n == "UtilityZone") return sf::Color(245, 205, 60);
            if (n == "ParkZone") return sf::Color(40, 150, 70);
            return sf::Color(200, 170, 230);   // civic and services
        }
    }
    if (world[x][y].zone != 0) {
        static const sf::Color zc[3] = {sf::Color(60, 120, 70), sf::Color(55, 95, 130), sf::Color(130, 95, 55)};
        return zc[zoneSector(world[x][y].zone)];
    }
    switch (world[x][y].terrain) {
        case Terrain::Water:  return sf::Color(40, 100, 175);
        case Terrain::Sand:   return sf::Color(205, 190, 130);
        case Terrain::Forest: return sf::Color(30, 85, 40);
        case Terrain::Rock:   return sf::Color(120, 120, 125);
        default:              return sf::Color(70, 130, 60);
    }
}

void Play_Scene::rebuildMinimap() {
    const sf::FloatRect ext = worldExtent();
    const sf::FloatRect mb = minimapBounds();
    const float s = std::min(mb.width / ext.width, mb.height / ext.height);
    minimapScale = s;
    minimapQuads.setPrimitiveType(sf::Quads);
    minimapQuads.resize(static_cast<size_t>(tileMapW * tileMapH * 4));
    size_t v = 0;
    for (int i = 0; i < tileMapW; i++)
        for (int j = 0; j < tileMapH; j++) {
            sf::Vector2f p = tileScreenPos(i, j);
            sf::Color c = minimapColor(i, j);
            minimapQuads[v++] = sf::Vertex(sf::Vector2f((p.x + 32.f) * s, p.y * s), c);
            minimapQuads[v++] = sf::Vertex(sf::Vector2f((p.x + 64.f) * s, (p.y + 16.f) * s), c);
            minimapQuads[v++] = sf::Vertex(sf::Vector2f((p.x + 32.f) * s, (p.y + 32.f) * s), c);
            minimapQuads[v++] = sf::Vertex(sf::Vector2f(p.x * s, (p.y + 16.f) * s), c);
        }
}

bool Play_Scene::handleMinimapClick(const sf::Vector2f& p) {
    if (!settings.minimap) return false;
    sf::FloatRect mb = minimapBounds();
    if (!mb.contains(p)) return false;
    // Centre the camera on the clicked spot
    view.setCenter((p.x - mb.left) / minimapScale, (p.y - mb.top) / minimapScale);
    return true;
}

void Play_Scene::renderMinimap(const sf::Vector2f& mouse) {
    using namespace uikit;
    if (!settings.minimap) return;
    sf::FloatRect mb = minimapBounds();
    // Rebuilt only when the map changed (builds, growth, fires, loads) or the window resized
    if (minimapDirty || minimapQuads.getVertexCount() == 0 || mb != minimapBuiltFor) {
        rebuildMinimap();
        minimapDirty = false;
        minimapBuiltFor = mb;
    }
    roundedRect(_window, sf::FloatRect(mb.left - 6.f, mb.top - 6.f, mb.width + 12.f, mb.height + 12.f), 10.f, PANEL,
                mb.contains(mouse) ? ACCENT : BORDER, 1.f);
    sf::RenderStates st;
    st.transform.translate(mb.left, mb.top);
    _window.draw(minimapQuads, st);
    // The camera's view
    sf::Vector2f c = view.getCenter(), sz = view.getSize();
    sf::RectangleShape cam(sf::Vector2f(sz.x * minimapScale, sz.y * minimapScale));
    cam.setPosition(mb.left + (c.x - sz.x / 2.f) * minimapScale, mb.top + (c.y - sz.y / 2.f) * minimapScale);
    cam.setFillColor(sf::Color(255, 255, 255, 25));
    cam.setOutlineColor(sf::Color(255, 255, 255, 200));
    cam.setOutlineThickness(1.f);
    _window.draw(cam);
}
