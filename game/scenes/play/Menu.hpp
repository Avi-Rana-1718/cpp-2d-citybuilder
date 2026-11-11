#pragma once
// Play_Scene member definitions (included at the end of Play_Scene.hpp).
// Menus: title screen, pause menu (Esc), new city, settings; city ranks
// (milestones) and the tip box.

enum MenuButtonId {
    MB_CONTINUE, MB_NEW, MB_LOAD, MB_SAVE, MB_SETTINGS, MB_QUIT, MB_RESUME, MB_MAINMENU,
    MB_DIFF0, MB_DIFF1, MB_DIFF2, MB_REROLL, MB_SANDBOX, MB_CREATE, MB_BACK,
    MB_SFX_DOWN, MB_SFX_UP, MB_AMB_DOWN, MB_AMB_UP, MB_CAM_DOWN, MB_CAM_UP, MB_AUTOSAVE, MB_TIPS, MB_DAYNIGHT, MB_MINIMAP
};

static bool fileExists(const std::string& path) {
    std::ifstream f(path);
    return static_cast<bool>(f);
}

std::vector<Play_Scene::MenuButton> Play_Scene::menuButtons() const {
    std::vector<MenuButton> b;
    const float W = 340.f, H = 46.f, gap = 12.f;
    const float x = (uikit::SW() - W) / 2.f;
    float y = uikit::SH() / 2.f - 120.f;
    auto add = [&](int id, const std::string& label, bool enabled = true, bool active = false) {
        b.push_back({sf::FloatRect(x, y, W, H), id, label, enabled, active});
        y += H + gap;
    };
    auto row = [&](std::vector<std::pair<int, std::string>> items, int activeId) {
        const float w = (W - gap * static_cast<float>(items.size() - 1)) / static_cast<float>(items.size());
        for (size_t k = 0; k < items.size(); k++)
            b.push_back({sf::FloatRect(x + static_cast<float>(k) * (w + gap), y, w, H), items[k].first, items[k].second, true,
                         items[k].first == activeId});
        y += H + gap;
    };
    auto stepper = [&](int down, int up, const std::string& label) {
        b.push_back({sf::FloatRect(x, y, H, H), down, "-", true, false});
        b.push_back({sf::FloatRect(x + H + gap, y, W - 2.f * (H + gap), H), -1, label, false, false});
        b.push_back({sf::FloatRect(x + W - H, y, H, H), up, "+", true, false});
        y += H + gap;
    };

    switch (menuPage) {
        case MENU_TITLE:
            add(MB_CONTINUE, "Continue", fileExists("autosave.sav") || fileExists("city.sav"));
            add(MB_NEW, "New City");
            add(MB_LOAD, "Load City", fileExists("city.sav"));
            add(MB_SETTINGS, "Settings");
            add(MB_QUIT, "Quit");
            break;
        case MENU_PAUSE:
            add(MB_RESUME, "Resume");
            add(MB_SAVE, "Save City  (F5)");
            add(MB_LOAD, "Load City  (F9)", fileExists("city.sav"));
            add(MB_SETTINGS, "Settings");
            add(MB_MAINMENU, "Main Menu");
            add(MB_QUIT, "Save & Quit");
            break;
        case MENU_NEW:
            y -= 40.f;
            row({{MB_DIFF0, "Easy $50k"}, {MB_DIFF1, "Normal $20k"}, {MB_DIFF2, "Hard $10k"}}, MB_DIFF0 + settings.difficulty);
            b.push_back({sf::FloatRect(x, y, W - 120.f, H), -1, "Map seed " + std::to_string(newCitySeed), false, false});
            b.push_back({sf::FloatRect(x + W - 108.f, y, 108.f, H), MB_REROLL, "Reroll", true, false});
            y += H + gap;
            add(MB_SANDBOX, newCitySandbox ? "[x] Sandbox: ready-made town" : "[ ] Sandbox: ready-made town", true, newCitySandbox);
            add(MB_CREATE, "Found City");
            add(MB_BACK, "Back");
            break;
        case MENU_SETTINGS: {
            y -= 110.f;
            stepper(MB_SFX_DOWN, MB_SFX_UP, "Sound effects " + std::to_string(settings.sfxVolume) + "%");
            stepper(MB_AMB_DOWN, MB_AMB_UP, "City ambience " + std::to_string(settings.ambientVolume) + "%");
            char cam[32];
            std::snprintf(cam, sizeof(cam), "Camera speed x%.2g", settings.cameraSpeed);
            stepper(MB_CAM_DOWN, MB_CAM_UP, cam);
            add(MB_AUTOSAVE, settings.autosave ? "[x] Autosave monthly" : "[ ] Autosave monthly", true, settings.autosave);
            add(MB_TIPS, settings.tips ? "[x] Show tips" : "[ ] Show tips", true, settings.tips);
            add(MB_DAYNIGHT, settings.dayNight ? "[x] Day and night" : "[ ] Day and night", true, settings.dayNight);
            add(MB_MINIMAP, settings.minimap ? "[x] Minimap  (M)" : "[ ] Minimap  (M)", true, settings.minimap);
            add(MB_BACK, "Back");
            break;
        }
        default:
            break;
    }
    return b;
}

void Play_Scene::openMenu(MenuPage page) {
    if (page == MENU_SETTINGS) menuBack = menuPage == MENU_NONE ? MENU_PAUSE : menuPage;
    if (page == MENU_NEW) newCitySeed = static_cast<uint32_t>(std::random_device{}() % 1000000u) + 1u;
    menuPage = page;
    isDragging = false;
}

void Play_Scene::handleMenuClick(int id) {
    audio.play("click");
    switch (id) {
        case MB_CONTINUE: {
            const std::string path = fileExists("autosave.sav") ? "autosave.sav" : "city.sav";
            if (loadCity(path)) { menuPage = MENU_NONE; notify("Welcome back to " + std::string(CITY_RANKS[cityRank()].name) + " Chime", false); }
            else showToast("Could not load " + path + " (" + loadError + ")", true);
            break;
        }
        case MB_NEW: openMenu(MENU_NEW); break;
        case MB_LOAD:
            if (loadCity("city.sav")) { menuPage = MENU_NONE; notify("Loaded city.sav", false); }
            else showToast("Could not load city.sav (" + loadError + ")", true);
            break;
        case MB_SAVE:
            if (saveCity("city.sav")) notify("City saved (city.sav)", false);
            else showToast("Could not save city.sav", true);
            menuPage = MENU_NONE;
            break;
        case MB_SETTINGS: openMenu(MENU_SETTINGS); break;
        case MB_QUIT: quitRequested = true; break;
        case MB_RESUME: menuPage = MENU_NONE; break;
        case MB_MAINMENU:
            onClose();   // autosave on the way out
            menuPage = MENU_TITLE;
            break;
        case MB_DIFF0: case MB_DIFF1: case MB_DIFF2:
            settings.difficulty = id - MB_DIFF0;
            settings.save();
            break;
        case MB_REROLL: newCitySeed = static_cast<uint32_t>(std::random_device{}() % 1000000u) + 1u; break;
        case MB_SANDBOX: newCitySandbox = !newCitySandbox; break;
        case MB_CREATE: beginNewCity(newCitySeed, settings.difficulty, newCitySandbox); break;
        case MB_BACK: menuPage = menuPage == MENU_SETTINGS ? menuBack : MENU_TITLE; settings.save(); break;
        case MB_SFX_DOWN: settings.sfxVolume -= 10; settings.clamp(); break;
        case MB_SFX_UP:   settings.sfxVolume += 10; settings.clamp(); break;
        case MB_AMB_DOWN: settings.ambientVolume -= 10; settings.clamp(); break;
        case MB_AMB_UP:   settings.ambientVolume += 10; settings.clamp(); break;
        case MB_CAM_DOWN: settings.cameraSpeed -= 0.25f; settings.clamp(); break;
        case MB_CAM_UP:   settings.cameraSpeed += 0.25f; settings.clamp(); break;
        case MB_AUTOSAVE: settings.autosave = !settings.autosave; break;
        case MB_TIPS: settings.tips = !settings.tips; break;
        case MB_DAYNIGHT: settings.dayNight = !settings.dayNight; break;
        case MB_MINIMAP: settings.minimap = !settings.minimap; break;
        default: break;
    }
}

// A fresh city from the New City page (or a sandbox with the starter town)
void Play_Scene::beginNewCity(uint32_t seed, int difficulty, bool sandbox) {
    gameClock = GameClock();
    startNewCity(seed);
    money = GameSettings::startingMoneyFor(difficulty);
    bestPopulation = 0;
    rankReached = 0;
    milestonesEnabled = !sandbox;
    if (sandbox) {
        starterCity = true;
        buildBenchTown();
        buildStarterShowcase();
        starterCity = false;
    }
    utilities.refreshService();
    services.update();
    traffic.update();
    happinessSystem.updateSimulation();
    utilitiesDirty = false;
    placementErrorTimer = 0.f;
    toolbar.setActiveTool(TOOL_NONE);
    menuPage = MENU_NONE;
    notify(sandbox ? "Sandbox city founded - everything unlocked" : "City founded! Start with a road from the map edge", false);
}

// Menus run instead of the game: the city is frozen behind them
void Play_Scene::updateMenu(float deltaTime) {
    const bool focused = _window.hasFocus();
    sf::Vector2f mouse = uikit::hudFromPixel(sf::Mouse::getPosition(_window), _window.getSize());
    bool down = focused && sf::Mouse::isButtonPressed(sf::Mouse::Left);
    if (down && !menuMouseWasDown) {
        for (const MenuButton& b : menuButtons())
            if (b.enabled && b.id >= 0 && b.r.contains(mouse)) { handleMenuClick(b.id); break; }
    }
    menuMouseWasDown = down;
    bool esc = focused && sf::Keyboard::isKeyPressed(sf::Keyboard::Escape);
    if (esc && !wasEscKey) {
        if (menuPage == MENU_PAUSE) menuPage = MENU_NONE;
        else if (menuPage == MENU_SETTINGS) menuPage = menuBack;
        else if (menuPage == MENU_NEW) menuPage = MENU_TITLE;
    }
    wasEscKey = esc;
    wasLeftPressed = down;
    budgetMouseWasDown = down;

    // The title screen slowly drifts across the map
    if (menuPage == MENU_TITLE || menuPage == MENU_NEW || (menuPage == MENU_SETTINGS && menuBack == MENU_TITLE)) {
        // A slow circle around the map centre, so it never drifts off the world
        const float t = gameTime.getElapsedTime().asSeconds() * 0.04f;
        sf::Vector2f c = tileScreenPos(tileMapW / 2, tileMapH / 2);
        view.setCenter(c.x + 32.f + 520.f * std::cos(t), c.y + 16.f + 260.f * std::sin(t));
    }
    (void)deltaTime;

    _window.clear(sf::Color(15, 20, 25));
    renderWorld(deltaTime);
    _window.setView(sf::View(sf::FloatRect(0.f, 0.f, uikit::SW(), uikit::SH())));
    renderMenu(mouse);
    renderToast();
    if (placementErrorTimer > 0.f) placementErrorTimer -= deltaTime;
    _window.display();
    audio.update(true, false);
}

void Play_Scene::renderMenu(const sf::Vector2f& mouse) {
    using namespace uikit;
    // Dim the world
    sf::RectangleShape shade(sf::Vector2f(SW(), SH()));
    shade.setFillColor(sf::Color(8, 10, 14, menuPage == MENU_PAUSE ? 150 : 110));
    _window.draw(shade);

    const auto buttons = menuButtons();
    float top = buttons.empty() ? SH() / 2.f : buttons.front().r.top;
    float bottom = buttons.empty() ? SH() / 2.f : buttons.back().r.top + buttons.back().r.height;
    sf::FloatRect panelR((SW() - 400.f) / 2.f, top - 120.f, 400.f, bottom - top + 150.f);
    panel(_window, panelR, 16.f);

    const char* titles[] = {"", "Chime City", "Paused", "New City", "Settings"};
    const std::string title = titles[static_cast<int>(menuPage)];
    const unsigned size = menuPage == MENU_TITLE ? 30 : 22;
    text(_window, title, (SW() - textWidth(title, size, true)) / 2.f, panelR.top + 26.f, size, ACCENT, true);
    std::string sub = menuPage == MENU_TITLE ? "An isometric city builder"
                    : menuPage == MENU_PAUSE ? std::string(CITY_RANKS[cityRank()].name) + " - " + gameClock.dateString()
                    : menuPage == MENU_NEW ? "Pick a difficulty and a map"
                    : "Saved to settings.cfg";
    text(_window, sub, (SW() - textWidth(sub, 11)) / 2.f, panelR.top + 26.f + static_cast<float>(size) + 10.f, 11, TEXT_DIM);

    for (const MenuButton& b : buttons) {
        const bool hover = b.enabled && b.id >= 0 && b.r.contains(mouse);
        sf::Color fill = b.id < 0 ? sf::Color(10, 13, 19, 200) : (hover ? PANEL_HOVER : PANEL_RAISED);
        if (b.active) fill = withAlpha(ACCENT, 70);
        roundedRect(_window, b.r, 10.f, fill, b.active ? ACCENT : BORDER, b.active ? 2.f : 1.f);
        const sf::Color tc = !b.enabled && b.id >= 0 ? sf::Color(110, 118, 130) : TEXT;
        const unsigned ts = b.r.width < 80.f ? 18 : 13;
        textMid(_window, b.label, b.r.left + (b.r.width - textWidth(b.label, ts, true)) / 2.f, b.r.top, b.r.height, ts, tc, true);
    }
    if (menuPage == MENU_TITLE) {
        const std::string hint = "F5 save  -  F9 load  -  Esc pause  -  Z undo  -  L messages  -  G stats";
        text(_window, hint, (SW() - textWidth(hint, 10)) / 2.f, panelR.top + panelR.height + 14.f, 10, TEXT_DIM);
    }
}

// ---- tips ----
// The first thing a new mayor is missing, in the order it usually matters
std::string Play_Scene::computeTip() const {
    int roads = 0, zoned = 0, homes = 0, jobs = 0, plants = 0, waterSrc = 0, garbage = 0, fireStations = 0, schools = 0;
    for (int i = 0; i < tileMapW; i++)
        for (int j = 0; j < tileMapH; j++) {
            if (world[i][j].zone != 0) zoned++;
            if (entities[i][j] == NULL) continue;
            if (RoadTile::at(i, j) != NULL) { roads++; continue; }
            Zone* z = dynamic_cast<Zone*>(entities[i][j]);
            if (z == NULL || z->isSubTile) continue;
            const std::string n = z->getName();
            if (n == "ResidentalZone") homes++;
            else if (n == "CommericalZone" || n == "IndustrialZone" || n == "FarmZone") jobs++;
            else if (n == "SchoolZone") schools++;
            else if (n == "FireZone") fireStations++;
            else if (n == "LandfillZone" || n == "IncineratorZone" || n == "RecyclingZone") garbage++;
            else if (const UtilityZone* u = dynamic_cast<const UtilityZone*>(z)) {
                if (u->kind <= UTIL_SOLAR) plants++;
                if (u->kind == UTIL_WELL || u->kind == UTIL_PUMP) waterSrc++;
            }
        }
    const int pop = townPopulation[0] + townPopulation[1] + townPopulation[2];
    if (roads == 0) return "Drag a road (Space) from the map edge - the edge link brings settlers in";
    if (zoned == 0 && homes == 0) return "Paint a residential zone (R) along the road - houses grow by themselves";
    if (jobs == 0) return "Residents need work: zone commercial (C) or industrial (I) land too";
    if (pop >= 100 && plants == 0) return "From 150 people buildings need power: wind turbines + a transformer (U menu)";
    if (pop >= 100 && waterSrc == 0) return "...and water: wells on high groundwater, joined by road or pipe";
    if (pop >= 120 && garbage == 0) return "Garbage piles up from 150 people - build a landfill (Waste menu)";
    if (pop >= 250 && fireStations == 0) return "Fires break out from 300 residents - a fire station covers 7 tiles";
    if (pop >= 200 && schools == 0) return "A school (1) lets residents earn more and buildings upgrade";
    if (money < 2000) return "Low on money? Raise taxes in the budget (B) or take a loan";
    return "";
}

sf::FloatRect Play_Scene::tipBounds() const {
    if (tipText.empty()) return sf::FloatRect();
    float w = uikit::textWidth(tipText, 12) + 96.f;
    return sf::FloatRect((uikit::SW() - w) / 2.f, uikit::SH() - 136.f, w, 36.f);
}

void Play_Scene::renderTip(const sf::Vector2f& mouse) {
    using namespace uikit;
    if (!settings.tips || !milestonesEnabled || tipText.empty()) return;
    sf::FloatRect r = tipBounds();
    roundedRect(_window, r, 12.f, sf::Color(20, 34, 30, 235), GOOD, 1.f);
    textMid(_window, "TIP", r.left + 12.f, r.top, r.height, 11, GOOD, true);
    textMid(_window, tipText, r.left + 46.f, r.top, r.height, 12, TEXT);
    sf::FloatRect x(r.left + r.width - 30.f, r.top + 7.f, 22.f, 22.f);
    roundedRect(_window, x, 6.f, x.contains(mouse) ? sf::Color(120, 40, 36) : PANEL_RAISED, BORDER, 1.f);
    textMid(_window, "x", x.left + 7.f, x.top - 1.f, x.height, 12, TEXT_DIM, true);
}
