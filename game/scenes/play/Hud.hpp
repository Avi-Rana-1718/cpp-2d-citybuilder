#pragma once
// Play_Scene member definitions (included at the end of Play_Scene.hpp).
// HUD: top bar, map view panel, inspect card, toast, message log, budget, game over

std::string Play_Scene::getOverlayName() const {
    switch (overlayMode) {
        case 1: return "Fertility";
        case 2: return "Groundwater";
        case 3: return "Deposits";
        case 4: return "Traffic";
        case 5: return "Power";
        case 6: return "Water";
        case 7: return "Land value";
        case 8: return "Services";
        default: return "Normal";
    }
}

// Map View panel: fixed screen-space buttons under the top bar (left side)
sf::FloatRect Play_Scene::viewPanelBounds() const {
    return sf::FloatRect(uikit::SW() - 452.f, 58.f, 440.f, 84.f);
}

sf::FloatRect Play_Scene::viewButtonBounds(int mode) const {
    sf::FloatRect panel = viewPanelBounds();
    return sf::FloatRect(panel.left + 8.f + static_cast<float>(mode) * 48.f, panel.top + 8.f, 40.f, 40.f);
}

bool Play_Scene::handleViewPanelClick(const sf::Vector2f& point) {
    for (int m = 0; m < OVERLAY_COUNT; m++) {
        if (viewButtonBounds(m).contains(point)) {
            setOverlayMode(m);
            return true;
        }
    }
    return false;
}

void Play_Scene::renderViewPanel(const sf::Font& font, const sf::Vector2f& mouse) {
    (void)font;
    using namespace uikit;
    sf::FloatRect pb = viewPanelBounds();
    panel(_window, pb, 12.f);

    const char* icons[OVERLAY_COUNT] = {"view_normal", "view_fertility", "view_water", "view_deposits", "view_traffic",
                                        "view_power", "view_tap", "view_land", "view_services"};
    const char* names[OVERLAY_COUNT] = {"Normal", "Fertility", "Groundwater", "Deposits", "Traffic", "Power", "Water", "Land value", "Services"};
    const sf::Color tints[OVERLAY_COUNT] = {sf::Color(200, 210, 220), sf::Color(120, 215, 90),
                                            sf::Color(80, 170, 240), sf::Color(240, 150, 70), sf::Color(240, 200, 60),
                                            sf::Color(250, 210, 60), sf::Color(90, 180, 245), sf::Color(120, 230, 160),
                                            sf::Color(200, 160, 240)};

    int hovered = -1;
    for (int m = 0; m < OVERLAY_COUNT; m++) {
        sf::FloatRect b = viewButtonBounds(m);
        bool active = (overlayMode == m);
        if (b.contains(mouse)) hovered = m;
        sf::Color fill = active ? withAlpha(tints[m], 60) : (hovered == m ? PANEL_HOVER : PANEL_RAISED);
        roundedRect(_window, b, 10.f, fill, active ? tints[m] : BORDER, active ? 2.f : 1.f);
        icon(_window, icons[m], b.left + 4.f, b.top + 4.f, 32.f, tints[m]);
    }

    // Caption + legend for the active view (hover shows the hovered view's name)
    float ly = pb.top + 56.f;
    if (hovered >= 0 && hovered != overlayMode) {
        textMid(_window, std::string("View: ") + names[hovered] + "  [O]", pb.left + 10.f, ly, 20.f, 11, TEXT_DIM);
        return;
    }
    auto key = [&](float x, const sf::Color& c, const std::string& label) {
        roundedRect(_window, sf::FloatRect(x, ly + 5.f, 10.f, 10.f), 3.f, c, BORDER, 1.f);
        return 14.f + text(_window, label, x + 14.f, ly + 5.f, 10, TEXT_DIM) + 10.f;
    };
    float x = pb.left + 10.f;
    switch (overlayMode) {
        case 1: x += key(x, sf::Color(210, 60, 50), "Barren"); key(x, sf::Color(40, 210, 50), "Rich soil"); break;
        case 2: x += key(x, sf::Color(40, 130, 230, 90), "Dry"); key(x, sf::Color(40, 130, 230, 240), "Wet ground"); break;
        case 3: x += key(x, sf::Color(30, 30, 36), "Coal"); x += key(x, sf::Color(200, 85, 30), "Iron");
                key(x, sf::Color(215, 215, 220), "Stone"); break;
        case 4: x += key(x, congestionColor(0.3f), "Free"); x += key(x, congestionColor(1.0f), "Busy");
                key(x, congestionColor(1.6f), "Jammed"); break;
        case 5: x += key(x, sf::Color(60, 200, 90), "Powered"); x += key(x, sf::Color(230, 60, 50), "Dark");
                key(x, sf::Color(250, 205, 50), "Plant / transformer"); break;
        case 6: x += key(x, sf::Color(60, 200, 90), "Clean"); x += key(x, sf::Color(235, 165, 50), "Untreated");
                x += key(x, sf::Color(230, 60, 50), "Dry"); key(x, sf::Color(40, 130, 230, 200), "Reserve"); break;
        case 7: x += key(x, sf::Color(200, 70, 60), "Low"); x += key(x, sf::Color(230, 200, 70), "Tier 2 (40+)");
                key(x, sf::Color(70, 210, 110), "Tier 3 (60+)"); break;
        case 8: x += key(x, sf::Color(70, 120, 240), "Police"); x += key(x, sf::Color(235, 70, 60), "Fire");
                x += key(x, sf::Color(70, 210, 110), "Clinic"); key(x, sf::Color(120, 90, 50), "No garbage pickup"); break;
        default: textMid(_window, "Map view  [O]", pb.left + 10.f, ly, 20.f, 11, TEXT_DIM); break;
    }
}

// Daily tax with one decimal, e.g. "2.5"
static std::string dailyTaxText(const Zone* z) {
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%.1f", z ? z->dailyTax() : 0.f);
    return buf;
}

void Play_Scene::renderHUD(float deltaTime) {
    (void)deltaTime;
    // HUD view matches the window 1:1, so nothing is stretched
    _window.setView(sf::View(sf::FloatRect(0.f, 0.f, uikit::SW(), uikit::SH())));
    const sf::Font& font = m_fonts["../assets/Hack.ttf"];

    sf::Vector2f mouse = uikit::hudFromPixel(sf::Mouse::getPosition(_window), _window.getSize());

    renderTopBar(mouse);
    renderViewPanel(font, mouse);
    renderInspectCard();
    toolbar.draw(_window, font, mouse);
    if (budgetOpen) renderBudget(mouse);
    renderMinimap(mouse);
    renderLog(mouse);
    {   // Stats chip, left of the messages chip
        using namespace uikit;
        sf::FloatRect c = statsChipBounds();
        roundedRect(_window, c, 8.f, c.contains(mouse) || statsOpen ? PANEL_HOVER : PANEL, statsOpen ? ACCENT : BORDER, 1.f);
        textMid(_window, "Stats [G]", c.left + 10.f, c.top, c.height, 10, TEXT_DIM);
    }
    renderStats(mouse);
    renderTip(mouse);
    renderToast();
    if (economy.bankrupt) renderGameOver(mouse);

    // Small diagnostics, bottom-right
    std::string diag = std::to_string(calculatedFPS) + " FPS  |  Seed " + std::to_string(worldSeed);
    float dw = uikit::textWidth(diag, 10) + 18.f;
    sf::FloatRect dr(uikit::SW() - dw - 10.f, uikit::SH() - 30.f, dw, 20.f);
    uikit::roundedRect(_window, dr, 8.f, uikit::PANEL, uikit::BORDER, 1.f);
    uikit::textMid(_window, diag, dr.left + 9.f, dr.top, dr.height, 10, uikit::TEXT_DIM);
}

// Top status bar: city, money, population + demand, happiness, date/season + speed
void Play_Scene::renderTopBar(const sf::Vector2f& mouse) {
    using namespace uikit;
    const float W = SW();
    const float sh = topBarShift();

    sf::RectangleShape bar(sf::Vector2f(W, 48.f));
    bar.setFillColor(sf::Color(12, 16, 23, 240));
    _window.draw(bar);
    sf::RectangleShape line(sf::Vector2f(W, 1.f));
    line.setPosition(0.f, 48.f);
    line.setFillColor(BORDER);
    _window.draw(line);

    auto chip = [&](float x, float w) {
        sf::FloatRect r(x, 6.f, w, 36.f);
        roundedRect(_window, r, 10.f, PANEL_RAISED, BORDER, 1.f);
        return r;
    };

    // City name (hidden on narrow windows)
    if (sh == 0.f) {
        text(_window, "Chime City", 16.f, 9.f, 15, ACCENT, true);
        text(_window, milestonesEnabled ? rankProgressText() : std::string(CITY_RANKS[cityRank()].name) + " - Year " + std::to_string(gameClock.getYear()),
             16.f, 29.f, 10, TEXT_DIM);
    }

    // Treasury (click for the budget)
    sf::FloatRect m = moneyChipBounds();
    bool moneyHover = m.contains(mouse);
    roundedRect(_window, m, 10.f, moneyHover || budgetOpen ? PANEL_HOVER : PANEL_RAISED,
                budgetOpen ? ACCENT : BORDER, budgetOpen ? 2.f : 1.f);
    icon(_window, "coin", m.left + 8.f, m.top + 6.f, 24.f);
    text(_window, formatMoney(money), m.left + 40.f, m.top + 4.f, 15, money < 0 ? BAD : TEXT, true);
    if (economy.daysInDebt > 0) {
        text(_window, "Debt! " + std::to_string(economy.daysUntilBankrupt()) + " days left", m.left + 40.f, m.top + 22.f, 10, BAD, true);
    } else {
        sf::Color flow = lastDailyIncome > 0 ? GOOD : (lastDailyIncome < 0 ? BAD : TEXT_DIM);
        std::string sign = lastDailyIncome < 0 ? "-" : "+";
        text(_window, sign + formatMoney(std::abs(lastDailyIncome)) + " / day   [B]", m.left + 40.f, m.top + 22.f, 10, flow);
    }

    // Population + demand by income level
    sf::FloatRect p = chip(360.f + sh, 190.f);
    icon(_window, "people", p.left + 8.f, p.top + 6.f, 24.f);
    int pop = townPopulation[0] + townPopulation[1] + townPopulation[2];
    text(_window, withCommas(pop), p.left + 40.f, p.top + 4.f, 15, TEXT, true);
    text(_window, "citizens", p.left + 40.f, p.top + 22.f, 10, TEXT_DIM);
    // Demand: R C I groups, each $ $$ $$$ (up = wanted, down red = surplus)
    const sf::Color rci[3] = {GOOD, ACCENT, sf::Color(235, 140, 50)};
    float bx = p.left + 98.f;
    float mid = p.top + 18.f;
    for (int sct = 0; sct < 3; sct++) {
        for (int w = 0; w < 3; w++) {
            float d = economy.demand[sct][w] / 100.f;
            float x = bx + static_cast<float>(sct) * 30.f + static_cast<float>(w) * 8.f;
            sf::RectangleShape track(sf::Vector2f(6.f, 26.f));
            track.setPosition(x, mid - 13.f);
            track.setFillColor(sf::Color(10, 13, 19, 220));
            _window.draw(track);
            float h = std::max(1.f, 13.f * std::fabs(d));
            sf::RectangleShape barShape(sf::Vector2f(6.f, h));
            barShape.setPosition(x, d >= 0.f ? mid - h : mid);
            barShape.setFillColor(d >= 0.f ? rci[sct] : BAD);
            _window.draw(barShape);
        }
        text(_window, sectorLetter(sct), bx + static_cast<float>(sct) * 30.f + 7.f, p.top + 30.f, 8, TEXT_DIM, true);
    }

    // Happiness
    int happy = happinessSystem.cityHappiness;
    sf::Color hc = happy >= 75 ? GOOD : (happy >= 50 ? WARN : BAD);
    sf::FloatRect h = chip(560.f + sh, 130.f);
    icon(_window, "smile", h.left + 8.f, h.top + 6.f, 24.f, hc);
    text(_window, std::to_string(happy) + "%", h.left + 40.f, h.top + 4.f, 15, hc, true);
    text(_window, "happiness", h.left + 40.f, h.top + 22.f, 10, TEXT_DIM);

    // Date, season and speed controls
    Season season = gameClock.getSeason();
    const char* seasonIcons[4] = {"season_spring", "season_summer", "season_autumn", "season_winter"};
    const sf::Color seasonColors[4] = {sf::Color(120, 220, 110), sf::Color(250, 205, 70),
                                       sf::Color(235, 140, 60), sf::Color(170, 205, 245)};
    sf::FloatRect d = chip(700.f + sh, 420.f);
    icon(_window, seasonIcons[static_cast<int>(season)], d.left + 8.f, d.top + 6.f, 24.f);
    text(_window, gameClock.dateString(), d.left + 40.f, d.top + 4.f, 14, TEXT, true);
    std::string sub = GameClock::seasonName(season);
    if (gameClock.isPaused()) sub += "  -  PAUSED";
    text(_window, sub, d.left + 40.f, d.top + 22.f, 10, gameClock.isPaused() ? WARN : seasonColors[static_cast<int>(season)], true);

    // Power and water: delivered / needed (red when anything goes without)
    if (1130.f + sh + 2.f * 136.f <= W) {
        auto utilChip = [&](float x, const char* ic, float have, float need, bool shortage, const char* unit) {
            sf::FloatRect r = chip(x, 130.f);
            sf::Color c = utilities.grace ? TEXT_DIM : (shortage ? BAD : GOOD);
            icon(_window, ic, r.left + 8.f, r.top + 6.f, 24.f, c);
            char buf[48];
            std::snprintf(buf, sizeof(buf), "%.0f/%.0f", have, need);
            text(_window, buf, r.left + 38.f, r.top + 4.f, 13, c, true);
            text(_window, utilities.grace ? std::string("not needed yet") : std::string(unit), r.left + 38.f, r.top + 22.f, 10, TEXT_DIM);
        };
        utilChip(1130.f + sh, "view_power", utilities.powerDelivered, utilities.powerDemand, utilities.unpowered > 0, "power units");
        utilChip(1266.f + sh, "view_tap", utilities.waterDelivered, utilities.waterDemand,
                 utilities.unwatered > 0 || utilities.untreatedShare > 0.3f, "water / day");
    }

    const char* speedIcons[GameClock::SPEED_COUNT] = {"speed_pause", "speed_1", "speed_2", "speed_4"};
    for (int i = 0; i < GameClock::SPEED_COUNT; i++) {
        sf::FloatRect b = speedButtonBounds(i);
        bool active = gameClock.getSpeedIndex() == i;
        bool hovered = b.contains(mouse);
        sf::Color fill = active ? sf::Color(30, 80, 120) : (hovered ? PANEL_HOVER : sf::Color(18, 23, 32));
        roundedRect(_window, b, 7.f, fill, active ? ACCENT : BORDER, active ? 2.f : 1.f);
        icon(_window, speedIcons[i], b.left + 5.f, b.top + 2.f, 24.f, active ? sf::Color::White : TEXT_DIM);
    }
}

// Messages chip (bottom-right) and, when open, the message log above it
void Play_Scene::renderLog(const sf::Vector2f& mouse) {
    using namespace uikit;
    sf::FloatRect chipR = logChipBounds();
    int errors = 0;
    for (const Notice& n : notices) if (n.error && gameClock.getTotalDays() - n.day <= 30) errors++;
    bool hover = chipR.contains(mouse);
    roundedRect(_window, chipR, 8.f, hover || logOpen ? PANEL_HOVER : PANEL, logOpen ? ACCENT : BORDER, 1.f);
    std::string label = "Messages [L]" + (errors > 0 ? "  " + std::to_string(errors) : std::string());
    textMid(_window, label, chipR.left + 10.f, chipR.top, chipR.height, 10, errors > 0 ? WARN : TEXT_DIM, errors > 0);
    if (!logOpen) return;

    sf::FloatRect p(SW() - 432.f, SH() - 420.f, 420.f, 356.f);
    panel(_window, p, 12.f);
    text(_window, "Messages", p.left + 14.f, p.top + 10.f, 14, TEXT, true);
    text(_window, "F5 save  F9 load  Z undo", p.left + p.width - 14.f - textWidth("F5 save  F9 load  Z undo", 10), p.top + 14.f, 10, TEXT_DIM);
    float y = p.top + 38.f;
    if (notices.empty()) text(_window, "Nothing yet", p.left + 14.f, y, 11, TEXT_DIM);
    for (const Notice& n : notices) {
        if (y > p.top + p.height - 22.f) break;
        GameClock c;
        c.restore(n.day, 0.f, 1);
        text(_window, c.dateString(), p.left + 14.f, y, 10, TEXT_DIM);
        text(_window, fitText(n.text, 10, p.width - 128.f), p.left + 112.f, y, 10, n.error ? sf::Color(255, 170, 150) : TEXT);
        y += 20.f;
    }
}

// Placement error toast, bottom-centre, fading out
void Play_Scene::renderToast() {
    using namespace uikit;
    if (placementErrorTimer <= 0.f || lastPlacementError.empty()) return;
    float alpha = std::min(1.f, placementErrorTimer / 0.5f); // fade over the last half second
    const std::string& msg = lastPlacementError;
    float w = textWidth(msg, 13, true) + 64.f;
    sf::FloatRect r((SW() - w) / 2.f, SH() - 84.f, w, 40.f);
    sf::Uint8 a = static_cast<sf::Uint8>(235.f * alpha);
    roundedRect(_window, sf::FloatRect(r.left + 2.f, r.top + 3.f, r.width, r.height), 12.f, withAlpha(SHADOW, static_cast<sf::Uint8>(90.f * alpha)));
    sf::Color edge = toastIsError ? BAD : ACCENT;
    roundedRect(_window, r, 12.f, toastIsError ? sf::Color(48, 20, 22, a) : sf::Color(18, 34, 52, a), withAlpha(edge, a), 1.5f);
    icon(_window, "warning", r.left + 12.f, r.top + 8.f, 24.f, sf::Color(255, 255, 255, a));
    textMid(_window, msg, r.left + 44.f, r.top, r.height, 13, withAlpha(sf::Color(255, 215, 210), a), true);
}

// Inspect card (Inspect mode): building or natural tile under the cursor
void Play_Scene::renderInspectCard() {
    using namespace uikit;
    inspectCardRect = sf::FloatRect();
    if (toolbar.getActiveTool() != TOOL_NONE) return;
    if (tileX < 0 || tileX >= tileMapW || tileY < 0 || tileY >= tileMapH) return;

    bool natural = (entities[tileX][tileY] == NULL);
    Zone* z = natural ? NULL : dynamic_cast<Zone*>(entities[tileX][tileY]);
    Zone* mz = z;
    if (z != NULL && z->isSubTile && z->rootX >= 0 && z->rootY >= 0) {
        mz = dynamic_cast<Zone*>(entities[z->rootX][z->rootY]);
    }
    if (!natural && mz == NULL) return;

    const std::string kind = natural ? tileKind(tileX, tileY) : mz->getName();
    const bool isRoad = isRoadName(kind);
    RoadTile* road = isRoad ? RoadTile::at(tileX, tileY) : NULL;
    const bool isBuilding = !natural && !isRoad;
    const bool isZone = kind == "ResidentalZone" || kind == "CommericalZone" || kind == "IndustrialZone";

    // Title, subtitle, accent and thumbnail
    std::string title, subtitle;
    sf::Color accent = ACCENT;
    if (natural) {
        title = terrainName(world[tileX][tileY].terrain);
        subtitle = isClearGroundName(kind) ? "Open land - buildable" : "Bulldoze to clear";
        if (world[tileX][tileY].zone != 0) {
            const uint8_t zv = world[tileX][tileY].zone;
            title = sectorName(zoneSector(zv)) + " zone";
            subtitle = std::string(zoneDensityName(zoneDensity(zv))) + " - grows to tier " + std::to_string(zoneMaxTier(zv));
        }
        accent = sf::Color(120, 200, 110);
    } else if (isRoad) {
        const char* tierTitle[3] = {"Dirt Road", "Asphalt Road", "Avenue"};
        title = tierTitle[road->tier];
        if (road->isBridge()) title += " Bridge";
        if (road->roundabout) title += " Roundabout";
        subtitle = "Capacity " + std::to_string(static_cast<int>(TrafficSystem::effectiveCapacity(road))) + " trips / day";
        accent = road->tier == 2 ? sf::Color(226, 196, 70) : sf::Color(190, 196, 204);
    } else if (kind == "ResidentalZone") {
        title = "Residential " + wealthSigns(mz->wealth()); subtitle = dynamic_cast<ResidentalZone*>(mz)->getVariantName(); accent = GOOD;
    } else if (kind == "CommericalZone") {
        title = "Commercial " + wealthSigns(mz->wealth()); subtitle = dynamic_cast<CommericalZone*>(mz)->getVariantName(); accent = ACCENT;
    } else if (kind == "IndustrialZone") {
        title = "Industrial " + wealthSigns(mz->wealth()); subtitle = dynamic_cast<IndustrialZone*>(mz)->getVariantName(); accent = sf::Color(235, 140, 50);
    } else if (kind == "FarmZone") {
        title = "Farm"; subtitle = dynamic_cast<FarmZone*>(mz)->getVariantName(); accent = sf::Color(150, 205, 90);
    } else if (kind == "SchoolZone") {
        title = "Elementary School"; subtitle = "Education - radius 5"; accent = BAD;
    } else if (kind == "ChurchZone") {
        title = "Community Church"; subtitle = "Faith - radius 4"; accent = sf::Color(170, 120, 220);
    } else if (kind == "ParkZone") {
        title = "Nature Park"; subtitle = "Health & fun - radius 4"; accent = GOOD;
    } else if (kind == "HarbourZone") {
        title = "Trade Harbour"; subtitle = "Sea trade port"; accent = ACCENT;
    } else if (serviceKindOfName(kind) >= 0) {
        const ServiceZone* sv = dynamic_cast<const ServiceZone*>(mz);
        title = serviceTitle(sv->kind);
        static const char* subs[SERVICE_KIND_COUNT] = {"Safety for nearby homes", "Puts out fires nearby", "Health for nearby homes",
                                                       "Stores garbage until full", "Jobs on a deposit", "Buses for nearby homes",
                                                       "Burns garbage into power - pollutes", "Recycles garbage cleanly"};
        subtitle = subs[sv->kind];
        accent = sf::Color(150, 170, 220);
    } else if (kind == "UtilityZone") {
        const UtilityZone* u = dynamic_cast<const UtilityZone*>(mz);
        title = utilityTitle(u->kind);
        static const char* subs[UTIL_KIND_COUNT] = {"Power plant - pollutes", "Clean power", "Clean power - weak in winter",
                                                    "Steps power down for its grid", "Clean water from groundwater",
                                                    "Raw water - drains groundwater", "Cleans raw water", "Stores spare water",
                                                    "Sewage into the water - pollutes", "Cleans sewage"};
        subtitle = subs[u->kind];
        accent = isPowerUtility(u->kind) ? sf::Color(245, 200, 60) : sf::Color(64, 160, 230);
    } else {
        title = kind;
    }

    // Height: header + building rows + land section
    const float W = 320.f;
    float hgt = 82.f;                       // header
    if (isBuilding || isRoad) hgt += 34.f;  // status + tax / upkeep row
    if (isRoad) hgt += 30.f;                // traffic bar
    if (isZone) hgt += 2 * 30.f;            // occupancy + happiness bars
    if (kind == "FarmZone") hgt += 22.f;    // soil / season line
    const bool isUtility = kind == "UtilityZone";
    const bool isConsumer = isBuilding && UtilitySystem::consumes(mz);
    if (isUtility) hgt += 2 * 30.f;         // output + grid bars
    if (isConsumer) hgt += 30.f;            // power / water chips
    hgt += 142.f;                           // land section
    const bool isService = !natural && serviceKindOfName(kind) >= 0;
    if (isService) hgt += 30.f;             // service info row
    sf::FloatRect card(SW() - W - 12.f, 152.f, W, hgt);
    inspectCardRect = card;
    panel(_window, card, 12.f);
    roundedRect(_window, sf::FloatRect(card.left, card.top, card.width, 4.f), 2.f, accent);

    // Header: thumbnail well + title
    sf::FloatRect well(card.left + 12.f, card.top + 14.f, 64.f, 56.f);
    roundedRect(_window, well, 9.f, sf::Color(10, 13, 19, 230));
    sf::FloatRect inner(well.left + 4.f, well.top + 4.f, well.width - 8.f, well.height - 8.f);
    if (isBuilding && mz->buildingSprite != NULL) {
        thumbnailTex(_window, mz->buildingSprite->getTexture(), inner);
    } else if (isRoad) {
        thumbnail(_window, road->roundabout ? "roundabout_icon"
                                            : std::string(roadSheetKey(road->tier)) + "_icon", inner);
    } else if (natural) {
        Terrain t = world[tileX][tileY].terrain;
        const char* key = t == Terrain::Water ? "water" : t == Terrain::Sand ? "sand" : t == Terrain::Forest ? "forest"
                        : t == Terrain::Rock ? "rock" : t == Terrain::ForestPath ? "forest_path" : "grass";
        thumbnail(_window, key, inner);
    }

    float tx = well.left + well.width + 12.f;
    const float maxTitleW = card.left + card.width - 14.f - tx;
    text(_window, fitText(title, 15, maxTitleW, true), tx, card.top + 18.f, 15, TEXT, true);
    text(_window, fitText(subtitle, 11, maxTitleW), tx, card.top + 38.f, 11, TEXT_DIM);
    if (isZone) {
        for (int s = 0; s < 3; s++) {
            icon(_window, "star", tx + static_cast<float>(s) * 18.f, card.top + 54.f, 16.f,
                 s < mz->tier ? sf::Color::White : sf::Color(255, 255, 255, 45));
        }
        const std::string rule = mz->fixedTier ? "Placed - fixed tier"
                               : (mz->zoned ? "Zoned - up to tier " + std::to_string(mz->maxTier) : "Grows freely");
        text(_window, fitText(rule, 10, maxTitleW - 62.f), tx + 62.f, card.top + 56.f, 10,
             mz->fixedTier ? TEXT_DIM : sf::Color(120, 200, 140));
    } else if (isBuilding || isRoad) {
        text(_window, isRoad ? "1x1 tile" : "2x2 building", tx, card.top + 56.f, 10, TEXT_DIM);
    }

    float y = card.top + 82.f;
    const float lx = card.left + 14.f, rw = card.width - 28.f;

    auto barRow = [&](const std::string& label, const std::string& value, float frac, const sf::Color& c) {
        text(_window, label, lx, y, 11, TEXT_DIM);
        textRight(_window, value, lx + rw, y, 11, TEXT, true);
        bar(_window, sf::FloatRect(lx, y + 16.f, rw, 7.f), frac, c);
        y += 30.f;
    };

    if (isBuilding) {
        bool connected = mz->isActive;
        bool outside = roadNet.buildingOutside(mz->posX, mz->posY);
        sf::Color sc = !connected ? BAD : (outside ? GOOD : WARN);
        std::string status = !connected ? "No road access" : (outside ? "Road connected" : "No outside link - slow growth");
        if (kind == "ParkZone") status = "Always open";
        if (mz->fireDays > 0) { status = "ON FIRE"; sc = BAD; }
        if (const UtilityZone* uz = dynamic_cast<const UtilityZone*>(mz)) {
            const bool power = isPowerUtility(uz->kind);
            if (!uz->linked) { status = power ? "Not connected - add road / power line" : "Not connected - add road / pipe"; sc = BAD; }
            else if (uz->noTransformer) { status = "No transformer on this grid"; sc = WARN; }
            else { status = power ? "On the power grid" : "On the water grid"; sc = GOOD; }
        }
        float sw = textWidth(status, 11, true) + 20.f;
        roundedRect(_window, sf::FloatRect(lx, y, sw, 22.f), 11.f, withAlpha(sc, 45), sc, 1.f);
        textMid(_window, status, lx + 10.f, y, 22.f, 11, sc, true);
        if (mz->dailyUpkeep() > 0) {
            std::string cost = "-$" + std::to_string(mz->dailyUpkeep()) + " / day";
            textMid(_window, cost, lx + rw - textWidth(cost, 12, true), y, 22.f, 12, BAD, true);
        } else {
            std::string tax = "$" + dailyTaxText(mz) + " / day";
            textMid(_window, tax, lx + rw - textWidth(tax, 12, true), y, 22.f, 12, mz->dailyTax() > 0.f ? GOOD : TEXT_DIM, true);
        }
        y += 34.f;
    }
    if (isRoad) {
        bool outside = roadNet.tileOutside(tileX, tileY);
        sf::Color sc = outside ? GOOD : WARN;
        std::string status = outside ? "Reaches the map edge" : "No outside connection";
        float sw = textWidth(status, 11, true) + 20.f;
        roundedRect(_window, sf::FloatRect(lx, y, sw, 22.f), 11.f, withAlpha(sc, 45), sc, 1.f);
        textMid(_window, status, lx + 10.f, y, 22.f, 11, sc, true);
        std::string cost = "-$" + std::to_string(road->dailyUpkeep()) + " / day";
        textMid(_window, cost, lx + rw - textWidth(cost, 12, true), y, 22.f, 12, BAD, true);
        y += 34.f;
        float cong = traffic.congestion[tileX][tileY];
        barRow("Traffic", std::to_string(static_cast<int>(traffic.load[tileX][tileY])) + " trips  (" +
                          std::to_string(static_cast<int>(cong * 100.f)) + "%)",
               std::min(1.f, cong), cong >= 1.f ? BAD : (cong >= 0.6f ? WARN : GOOD));
    }
    if (isZone) {
        float occ = mz->capacity > 0 ? static_cast<float>(mz->population) / static_cast<float>(mz->capacity) : 0.f;
        barRow("Occupancy", std::to_string(mz->population) + " / " + std::to_string(mz->capacity), occ, ACCENT);
        int hap = mz->overallHappiness;
        barRow("Happiness", std::to_string(hap) + "%", static_cast<float>(hap) / 100.f,
               hap >= 75 ? GOOD : (hap >= 50 ? WARN : BAD));
    }
    if (isConsumer) {
        // Power / water status chips
        float cx = lx;
        auto statusChip = [&](const std::string& label, const sf::Color& c) {
            float w = textWidth(label, 11, true) + 20.f;
            roundedRect(_window, sf::FloatRect(cx, y, w, 22.f), 11.f, withAlpha(c, 45), c, 1.f);
            textMid(_window, label, cx + 10.f, y, 22.f, 11, c, true);
            cx += w + 8.f;
        };
        const int rx = mz->posX, ry = mz->posY;
        if (utilities.grace) {
            statusChip("Utilities not needed yet (town < " + std::to_string(UTILITY_GRACE_POP) + ")", TEXT_DIM);
        } else {
            statusChip(utilities.powered(rx, ry) ? "Power" : "No power", utilities.powered(rx, ry) ? GOOD : BAD);
            if (!utilities.watered(rx, ry)) statusChip("No water", BAD);
            else if (utilities.untreated(rx, ry)) statusChip("Untreated", WARN);
            else statusChip("Water", GOOD);
            statusChip(services.garbageOk(rx, ry) ? "Garbage" : "No pickup", services.garbageOk(rx, ry) ? GOOD : BAD);
            if (!utilities.sewered(rx, ry)) statusChip("No sewer", BAD);
        }
        y += 30.f;
    }
    if (isUtility) {
        const UtilityZone* u = dynamic_cast<const UtilityZone*>(mz);
        const UtilityGrid* g = utilities.gridFor(u);
        char buf[64];
        const float base = UTIL_OUTPUT[u->kind];
        const sf::Color uc = isPowerUtility(u->kind) ? sf::Color(245, 200, 60) : sf::Color(64, 160, 230);
        switch (u->kind) {
            case UTIL_COAL: case UTIL_WIND: case UTIL_SOLAR:
                std::snprintf(buf, sizeof(buf), "%.0f power", u->output);
                barRow("Output", buf, std::min(1.f, u->output / (base * 1.25f)), uc);
                if (g) std::snprintf(buf, sizeof(buf), "%.0f / %.0f", g->powerUsed, std::min(g->powerSupply, g->transformerCap));
                barRow(g && g->transformerCap <= 0.f ? "Grid: no transformer!" : "Grid load", g ? buf : "No road",
                       g && g->powerSupply > 0.f ? std::min(1.f, g->powerUsed / std::max(1.f, std::min(g->powerSupply, g->transformerCap))) : 0.f,
                       g && g->transformerCap <= 0.f ? BAD : uc);
                break;
            case UTIL_TRANSFORMER:
                if (g) std::snprintf(buf, sizeof(buf), "%.0f from plants", g->powerSupply);
                barRow("Grid supply", g ? buf : "No road", g ? std::min(1.f, g->powerSupply / std::max(1.f, g->powerDemand)) : 0.f,
                       g && g->powerSupply >= g->powerDemand ? GOOD : BAD);
                if (g) std::snprintf(buf, sizeof(buf), "%.0f / %.0f", g->powerUsed, g->transformerCap);
                barRow("Transformers on grid", g ? buf : "-", g ? std::min(1.f, g->powerUsed / std::max(1.f, g->transformerCap)) : 0.f, uc);
                break;
            case UTIL_WELL: case UTIL_PUMP: {
                float gw = UtilityZone::groundwaterAround(u->posX, u->posY, u->drawRadius());
                std::snprintf(buf, sizeof(buf), "%.0f / %.0f %s", u->output, base, u->kind == UTIL_WELL ? "clean" : "raw");
                barRow("Output", buf, u->output / base, uc);
                std::snprintf(buf, sizeof(buf), "%d%% (radius %d)", static_cast<int>(gw * 100.f + 0.5f), u->drawRadius());
                barRow("Groundwater reserve", buf, gw, gw < 0.25f ? BAD : (gw < 0.45f ? WARN : GOOD));
                break;
            }
            case UTIL_TREATMENT:
                if (g) std::snprintf(buf, sizeof(buf), "%.0f / %.0f", g->treated(), g->treatCap);
                barRow("Treating on grid", g ? buf : "No road", g ? std::min(1.f, g->treated() / std::max(1.f, g->treatCap)) : 0.f, uc);
                if (g) std::snprintf(buf, sizeof(buf), "%.0f untreated", g->untreatedPool());
                barRow("Raw water left over", g ? buf : "-", g && g->raw > 0.f ? g->untreatedPool() / g->raw : 0.f,
                       g && g->untreatedPool() > 0.f ? WARN : GOOD);
                break;
            case UTIL_SEWAGE_OUTFLOW:
            case UTIL_SEWAGE_PLANT:
                if (g) std::snprintf(buf, sizeof(buf), "%.0f / %.0f", g->sewageUsed, g->sewageCap);
                barRow("Sewage on grid", g ? buf : "Not connected", g && g->sewageCap > 0.f ? std::min(1.f, g->sewageUsed / g->sewageCap) : 0.f,
                       g && g->sewageDemand > g->sewageCap ? BAD : uc);
                if (g) std::snprintf(buf, sizeof(buf), "%.0f needed", g->sewageDemand);
                barRow("Waste water", g ? buf : "-", g && g->sewageDemand > 0.f ? std::min(1.f, g->sewageCap / g->sewageDemand) : 0.f, uc);
                break;
            case UTIL_TOWER:
                std::snprintf(buf, sizeof(buf), "%.0f / %.0f", u->stored, base);
                barRow("Stored", buf, u->stored / base, uc);
                if (g) std::snprintf(buf, sizeof(buf), "%.0f / %.0f", g->cleanUsed + g->towerUsed + g->untreatedUsed, g->waterDemand);
                barRow("Grid water", g ? buf : "No road", g ? std::min(1.f, (g->cleanPool() + g->raw - g->treated()) / std::max(1.f, g->waterDemand)) : 0.f,
                       GOOD);
                break;
        }
    }
    if (isService) {
        const ServiceZone* sv = dynamic_cast<const ServiceZone*>(mz);
        char buf[80];
        switch (sv->kind) {
            case SERVICE_LANDFILL:
                std::snprintf(buf, sizeof(buf), "%.0f / %.0f  (+%.0f today)", sv->stored, LANDFILL_SPACE, sv->handled);
                barRow(sv->stored >= LANDFILL_SPACE ? "FULL - build another" : "Filled", buf, sv->stored / LANDFILL_SPACE,
                       sv->stored >= LANDFILL_SPACE ? BAD : (sv->stored > 0.8f * LANDFILL_SPACE ? WARN : sf::Color(170, 150, 110)));
                break;
            case SERVICE_INCINERATOR:
                std::snprintf(buf, sizeof(buf), "%.0f / %.0f  -> %.0f power", sv->handled, INCINERATOR_RATE, sv->handled * INCINERATOR_POWER);
                barRow("Burning today", buf, sv->handled / INCINERATOR_RATE, sf::Color(230, 120, 60));
                break;
            case SERVICE_RECYCLING:
                std::snprintf(buf, sizeof(buf), "%.0f / %.0f", sv->handled, RECYCLING_RATE);
                barRow("Recycling today", buf, sv->handled / RECYCLING_RATE, GOOD);
                break;
            case SERVICE_MINE:
                std::snprintf(buf, sizeof(buf), "%d / %d miners - deposit %d%%", sv->population, sv->capacity,
                              static_cast<int>(sv->depositRichness() * 100.f + 0.5f));
                barRow("Mine", buf, sv->depositRichness(), sf::Color(170, 150, 130));
                break;
            default:
                std::snprintf(buf, sizeof(buf), "radius %d", SERVICE_RADIUS[sv->kind]);
                barRow(sv->kind == SERVICE_BUS_DEPOT ? "30% of commuters ride the bus" : "Covers homes", buf, 1.f, ACCENT);
                break;
        }
    }
    if (kind == "FarmZone") {
        FarmZone* fz = dynamic_cast<FarmZone*>(mz);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Soil x%.2f  -  %s x%.1f", fz->fertilityMul,
                      GameClock::seasonName(gameClock.getSeason()).c_str(), seasonalFarmMultiplier(gameClock.getSeason()));
        text(_window, buf, lx, y, 11, sf::Color(170, 220, 120), true);
        y += 22.f;
    }

    // Land section
    sf::RectangleShape div(sf::Vector2f(rw, 1.f));
    div.setPosition(lx, y);
    div.setFillColor(BORDER);
    _window.draw(div);
    y += 8.f;
    const TileData& land = world[tileX][tileY];
    text(_window, "LAND", lx, y, 10, TEXT_DIM, true);
    textRight(_window, terrainName(land.terrain), lx + rw, y, 10, TEXT_DIM);
    y += 18.f;
    barRow("Soil fertility", std::to_string(land.fertility) + "%", static_cast<float>(land.fertility) / 100.f,
           sf::Color(120, 200, 80));
    barRow("Groundwater", std::to_string(land.groundwater) + "%", static_cast<float>(land.groundwater) / 100.f,
           sf::Color(70, 150, 230));
    barRow("Land value", std::to_string(land.landValue) + (land.landValue >= LAND_VALUE_TIER3 ? "  (tier 3 ok)" : land.landValue >= LAND_VALUE_TIER2 ? "  (tier 2 ok)" : ""),
           static_cast<float>(land.landValue) / 100.f, sf::Color(120, 220, 150));
    if (land.deposit == Deposit::None) {
        text(_window, "Deposit", lx, y, 11, TEXT_DIM);
        textRight(_window, "None", lx + rw, y, 11, TEXT_DIM, true);
    } else {
        sf::Color dc = land.deposit == Deposit::Coal ? sf::Color(150, 150, 160)
                     : land.deposit == Deposit::Iron ? sf::Color(220, 110, 50) : sf::Color(220, 220, 225);
        barRow(depositName(land.deposit), std::to_string(land.depositAmount) + "%",
               static_cast<float>(land.depositAmount) / 100.f, dc);
    }
}

// -------------------------------------------------------------
// Budget panel: tax sliders + demand per income level, expenses, loans
// -------------------------------------------------------------
sf::FloatRect Play_Scene::budgetBounds() const {
    // Centred; never under the top bar
    return sf::FloatRect((uikit::SW() - 780.f) / 2.f, std::max(56.f, (uikit::SH() - 600.f) / 2.f), 780.f, 600.f);
}

sf::FloatRect Play_Scene::budgetCloseBounds() const {
    sf::FloatRect b = budgetBounds();
    return sf::FloatRect(b.left + b.width - 44.f, b.top + 12.f, 30.f, 30.f);
}

// Row layout: 3 sector groups (header 24 px) of 3 income rows (34 px)
static float budgetRowTop(const sf::FloatRect& b, int sector, int wealth) {
    return b.top + 92.f + static_cast<float>(sector) * (24.f + 3.f * 34.f) + 24.f + static_cast<float>(wealth) * 34.f;
}

sf::FloatRect Play_Scene::taxSliderBounds(int sector, int wealth) const {
    sf::FloatRect b = budgetBounds();
    return sf::FloatRect(b.left + 150.f, budgetRowTop(b, sector, wealth) + 13.f, 170.f, 8.f);
}

sf::FloatRect Play_Scene::loanButtonBounds() const {
    sf::FloatRect b = budgetBounds();
    return sf::FloatRect(b.left + 510.f, b.top + 470.f, 250.f, 34.f);
}

sf::FloatRect Play_Scene::newCityButtonBounds() const {
    return sf::FloatRect(uikit::SW() / 2.f - 110.f, uikit::SH() / 2.f + 60.f, 220.f, 40.f);
}

int Play_Scene::taxFromSliderX(int sector, int wealth, float x) const {
    sf::FloatRect t = taxSliderBounds(sector, wealth);
    float f = std::max(0.f, std::min(1.f, (x - t.left) / t.width));
    return TAX_MIN + static_cast<int>(std::round(f * static_cast<float>(TAX_MAX - TAX_MIN)));
}

bool Play_Scene::handleBudgetPointer(const sf::Vector2f& p, bool down, bool justPressed) {
    if (!budgetOpen) return false;
    sf::FloatRect b = budgetBounds();

    // Which row is hovered (for the demand hint)
    hoveredBucket = -1;
    for (int sct = 0; sct < SECTOR_COUNT; sct++)
        for (int w = 0; w < WEALTH_COUNT; w++) {
            float top = budgetRowTop(b, sct, w);
            if (p.x >= b.left + 16.f && p.x <= b.left + 490.f && p.y >= top && p.y < top + 34.f) hoveredBucket = sct * 3 + w;
        }

    if (justPressed) {
        if (budgetCloseBounds().contains(p)) { setBudgetOpen(false); return true; }
        if (loanButtonBounds().contains(p)) {
            if (economy.takeLoan()) showToast("Loan received: +$10,000 (repaid daily over a year)", false);
            else showToast("Loan limit reached (" + std::to_string(LOAN_MAX_ACTIVE) + " active loans)", true);
            return true;
        }
        for (int sct = 0; sct < SECTOR_COUNT; sct++) {
            for (int w = 0; w < WEALTH_COUNT; w++) {
                sf::FloatRect t = taxSliderBounds(sct, w);
                sf::FloatRect hit(t.left - 8.f, t.top - 10.f, t.width + 16.f, t.height + 20.f);
                if (hit.contains(p)) {
                    std::memcpy(taxBeforeDrag, taxRates, sizeof(taxBeforeDrag));   // for undo
                    activeSlider = sct * 3 + w;
                    taxRates[sct][w] = taxFromSliderX(sct, w, p.x);
                    return true;
                }
            }
        }
        return b.contains(p); // clicks on the panel never reach the map
    }
    if (down && activeSlider >= 0) {
        int sct = activeSlider / 3, w = activeSlider % 3;
        taxRates[sct][w] = taxFromSliderX(sct, w, p.x);
        return true;
    }
    if (!down && activeSlider >= 0) {
        // Slider released: remember the old rates so Z can put them back
        if (std::memcmp(taxBeforeDrag, taxRates, sizeof(taxBeforeDrag)) != 0) {
            UndoStep step{"", 0, true};
            std::memcpy(step.tax, taxBeforeDrag, sizeof(step.tax));
            undoStack.push_back(step);
            if (undoStack.size() > UNDO_LIMIT) undoStack.erase(undoStack.begin());
        }
        activeSlider = -1;
    }
    return false;
}

void Play_Scene::renderBudget(const sf::Vector2f& mouse) {
    using namespace uikit;
    // keep the hover hint current even without clicks
    handleBudgetPointer(mouse, activeSlider >= 0, false);

    sf::FloatRect b = budgetBounds();
    panel(_window, b, 14.f);
    text(_window, "City Budget", b.left + 20.f, b.top + 16.f, 18, TEXT, true);
    text(_window, "Daily figures  -  drag a slider to change that tax rate", b.left + 20.f, b.top + 42.f, 11, TEXT_DIM);

    sf::FloatRect xb = budgetCloseBounds();
    bool xHover = xb.contains(mouse);
    roundedRect(_window, xb, 8.f, xHover ? sf::Color(120, 40, 36) : PANEL_RAISED, BORDER, 1.f);
    textMid(_window, "x", xb.left + 10.f, xb.top - 1.f, xb.height, 15, xHover ? sf::Color::White : TEXT_DIM, true);

    // ---- left: taxes and demand ----
    const sf::Color rci[3] = {GOOD, ACCENT, sf::Color(235, 140, 50)};
    float lx = b.left + 20.f;
    text(_window, "TAX & DEMAND", lx, b.top + 72.f, 11, TEXT_DIM, true);
    text(_window, "rate", lx + 336.f, b.top + 72.f, 9, TEXT_DIM);
    text(_window, "income", lx + 402.f, b.top + 72.f, 9, TEXT_DIM);
    for (int sct = 0; sct < SECTOR_COUNT; sct++) {
        float gy = budgetRowTop(b, sct, 0) - 24.f;
        roundedRect(_window, sf::FloatRect(lx, gy + 6.f, 4.f, 14.f), 2.f, rci[sct]);
        text(_window, sectorName(sct), lx + 10.f, gy + 6.f, 12, TEXT, true);

        for (int w = 0; w < WEALTH_COUNT; w++) {
            float ry = budgetRowTop(b, sct, w);
            if (hoveredBucket == sct * 3 + w) {
                roundedRect(_window, sf::FloatRect(lx - 6.f, ry, 478.f, 32.f), 8.f, sf::Color(255, 255, 255, 12));
            }
            text(_window, wealthSigns(w), lx + 10.f, ry + 10.f, 12, rci[sct], true);

            // Demand bar centred on zero
            float d = economy.demand[sct][w] / 100.f;
            sf::FloatRect db(lx + 36.f, ry + 12.f, 44.f, 10.f);
            roundedRect(_window, db, 3.f, sf::Color(10, 13, 19, 220));
            float half = db.width / 2.f;
            float wbar = std::max(1.f, half * std::fabs(d));
            sf::RectangleShape fill(sf::Vector2f(wbar, db.height - 2.f));
            fill.setPosition(d >= 0.f ? db.left + half : db.left + half - wbar, db.top + 1.f);
            fill.setFillColor(d >= 0.f ? rci[sct] : BAD);
            _window.draw(fill);
            sf::RectangleShape zero(sf::Vector2f(1.f, db.height + 4.f));
            zero.setPosition(db.left + half, db.top - 2.f);
            zero.setFillColor(TEXT_DIM);
            _window.draw(zero);
            char dbuf[8];
            int dval = static_cast<int>(std::lround(economy.demand[sct][w]));
            if (dval == 0) std::snprintf(dbuf, sizeof(dbuf), "0");
            else std::snprintf(dbuf, sizeof(dbuf), "%+d", dval);
            textRight(_window, dbuf, lx + 120.f, ry + 10.f, 10, d >= 0.f ? rci[sct] : BAD, true);

            // Tax slider
            sf::FloatRect t = taxSliderBounds(sct, w);
            int rate = taxRates[sct][w];
            float f = static_cast<float>(rate - TAX_MIN) / static_cast<float>(TAX_MAX - TAX_MIN);
            sf::Color rateCol = rate >= 13 ? BAD : (rate <= 5 ? GOOD : TEXT);
            bar(_window, t, f, withAlpha(rci[sct], 170));
            float kx = t.left + t.width * f;
            bool active = activeSlider == sct * 3 + w;
            sf::CircleShape knob(active ? 9.f : 7.f);
            knob.setOrigin(knob.getRadius(), knob.getRadius());
            knob.setPosition(kx, t.top + t.height / 2.f);
            knob.setFillColor(active ? sf::Color::White : sf::Color(220, 228, 238));
            knob.setOutlineColor(rci[sct]);
            knob.setOutlineThickness(2.f);
            _window.draw(knob);
            text(_window, std::to_string(rate) + "%", lx + 336.f, ry + 9.f, 12, rateCol, true);

            float inc = economy.lastDay.tax[sct][w];
            char buf[24];
            std::snprintf(buf, sizeof(buf), "$%.0f", inc);
            textRight(_window, buf, lx + 470.f, ry + 9.f, 12, inc > 0.5f ? GOOD : TEXT_DIM, true);
        }
    }

    // Demand hint for the hovered row
    std::string hint = "Hover a row to see what limits its demand";
    if (hoveredBucket >= 0) {
        int sct = hoveredBucket / 3, w = hoveredBucket % 3;
        char buf[96];
        std::snprintf(buf, sizeof(buf), "%s %s: demand %+.0f  -  %s", sectorLetter(sct).c_str(), wealthSigns(w).c_str(),
                      economy.demand[sct][w], economy.limitingFactor(sct, w).c_str());
        hint = buf;
    }
    text(_window, hint, lx, b.top + b.height - 30.f, 11, hoveredBucket >= 0 ? WARN : TEXT_DIM, hoveredBucket >= 0);

    // ---- right: expenses, summary, loans ----
    float rx = b.left + 510.f, rw = 250.f;
    sf::RectangleShape divider(sf::Vector2f(1.f, b.height - 110.f));
    divider.setPosition(rx - 16.f, b.top + 72.f);
    divider.setFillColor(BORDER);
    _window.draw(divider);

    const DayReport& rep = economy.lastDay;
    float y = b.top + 72.f;
    text(_window, "DAILY EXPENSES", rx, y, 11, TEXT_DIM, true);
    y += 22.f;
    auto row = [&](const std::string& label, const std::string& value, const sf::Color& c) {
        text(_window, label, rx, y, 12, TEXT_DIM);
        textRight(_window, value, rx + rw, y, 12, c, true);
        y += 22.f;
    };
    auto dollars = [](float v) { return "$" + withCommas(static_cast<long long>(std::round(v))); };
    row("Roads (" + std::to_string(rep.roadTiles) + " tiles)", dollars(static_cast<float>(rep.upkeepRoads)), TEXT);
    row("Schools", dollars(static_cast<float>(rep.upkeepSchools)), TEXT);
    row("Churches", dollars(static_cast<float>(rep.upkeepChurches)), TEXT);
    row("Parks", dollars(static_cast<float>(rep.upkeepParks)), TEXT);
    row("Harbours", dollars(static_cast<float>(rep.upkeepHarbours)), TEXT);
    row("Power & water", dollars(static_cast<float>(rep.upkeepUtilities)), TEXT);
    row("Services", dollars(static_cast<float>(rep.upkeepServices)), TEXT);
    row("Loan repayments", dollars(rep.loanPayments), TEXT);

    y += 8.f;
    text(_window, "SUMMARY", rx, y, 11, TEXT_DIM, true);
    y += 22.f;
    row("Tax income", "+" + dollars(rep.taxTotal), GOOD);
    row("Exports - imports", (rep.tradeIncome >= rep.tradeCost ? "+" : "-") + dollars(std::fabs(rep.tradeIncome - rep.tradeCost)),
        rep.tradeIncome >= rep.tradeCost ? GOOD : BAD);
    row("Expenses", "-" + dollars(rep.expenseTotal), BAD);
    float net = rep.taxTotal + rep.tradeIncome - rep.expenseTotal - rep.tradeCost;
    text(_window, "Net per day", rx, y, 13, TEXT, true);
    textRight(_window, (net >= 0.f ? "+" : "-") + dollars(std::fabs(net)), rx + rw, y, 13, net >= 0.f ? GOOD : BAD, true);
    y += 30.f;

    text(_window, "LOANS", rx, y, 11, TEXT_DIM, true);
    y += 22.f;
    row("Active", std::to_string(economy.loans.size()) + " / " + std::to_string(LOAN_MAX_ACTIVE), TEXT);
    row("Still owed", dollars(economy.loanBalance()), economy.loans.empty() ? TEXT_DIM : WARN);

    sf::FloatRect lb = loanButtonBounds();
    bool canLoan = economy.canTakeLoan();
    bool lHover = lb.contains(mouse) && canLoan;
    roundedRect(_window, lb, 10.f, canLoan ? (lHover ? sf::Color(40, 90, 130) : sf::Color(28, 64, 96)) : PANEL_RAISED,
                canLoan ? ACCENT : BORDER, 1.f);
    std::string loanLabel = canLoan ? "Take $10,000 loan (10%)" : "Loan limit reached";
    textMid(_window, loanLabel, lb.left + (lb.width - textWidth(loanLabel, 12, true)) / 2.f, lb.top, lb.height, 12,
            canLoan ? TEXT : TEXT_DIM, true);
    text(_window, "Repaid daily over 1 year", rx, lb.top + lb.height + 6.f, 10, TEXT_DIM);

    // Debt warning strip
    if (economy.daysInDebt > 0) {
        sf::FloatRect w(b.left + 16.f, b.top + b.height - 70.f, b.width - 32.f, 30.f);
        roundedRect(_window, w, 8.f, sf::Color(70, 22, 24, 235), BAD, 1.f);
        std::string msg = "In debt for " + std::to_string(economy.daysInDebt) + " days  -  bankrupt in " +
                          std::to_string(economy.daysUntilBankrupt()) + " days unless the treasury recovers";
        icon(_window, "warning", w.left + 8.f, w.top + 5.f, 20.f);
        textMid(_window, msg, w.left + 36.f, w.top, w.height, 11, sf::Color(255, 210, 205), true);
    }
}

void Play_Scene::renderGameOver(const sf::Vector2f& mouse) {
    using namespace uikit;
    sf::RectangleShape dim(sf::Vector2f(SW(), SH()));
    dim.setFillColor(sf::Color(5, 7, 10, 190));
    _window.draw(dim);

    sf::FloatRect b(SW() / 2.f - 260.f, SH() / 2.f - 140.f, 520.f, 270.f);
    panel(_window, b, 16.f, PANEL, BAD);
    icon(_window, "warning", b.left + b.width / 2.f - 24.f, b.top + 20.f, 48.f);
    std::string title = "City Bankrupt";
    text(_window, title, b.left + (b.width - textWidth(title, 22, true)) / 2.f, b.top + 80.f, 22, BAD, true);
    std::string l1 = "The treasury stayed in debt for 6 months.";
    std::string l2 = "Reached " + gameClock.dateString() + " with " +
                     withCommas(townPopulation[0] + townPopulation[1] + townPopulation[2]) + " citizens.";
    text(_window, l1, b.left + (b.width - textWidth(l1, 12)) / 2.f, b.top + 118.f, 12, TEXT);
    text(_window, l2, b.left + (b.width - textWidth(l2, 12)) / 2.f, b.top + 140.f, 12, TEXT_DIM);

    sf::FloatRect nb = newCityButtonBounds();
    bool hover = nb.contains(mouse);
    roundedRect(_window, nb, 10.f, hover ? sf::Color(40, 90, 130) : sf::Color(28, 64, 96), ACCENT, 1.f);
    std::string label = "Found a new city  [N]";
    textMid(_window, label, nb.left + (nb.width - textWidth(label, 13, true)) / 2.f, nb.top, nb.height, 13, TEXT, true);
}

void Play_Scene::refreshUI() {
    yr = gameClock.getYear();
    mon = gameClock.getMonth();
    day = gameClock.getDay();
}
