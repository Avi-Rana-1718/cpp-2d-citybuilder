#pragma once
// Play_Scene member definitions (included at the end of Play_Scene.hpp).
// City statistics panel (G): history graphs and the trade breakdown.

void Play_Scene::recordHistory(const DayReport& rep) {
    DaySample s;
    s.population = townPopulation[0] + townPopulation[1] + townPopulation[2];
    s.money = money;
    s.net = rep.net;
    s.happiness = happinessSystem.cityHappiness;
    for (int k = 0; k < 3; k++) s.demand[k] = townDemand[k];
    s.exports = trade.exportIncome;
    s.imports = trade.importCost;
    history.push_back(s);
    while (history.size() > HISTORY_DAYS) history.pop_front();
}

sf::FloatRect Play_Scene::statsBounds() const {
    return sf::FloatRect((uikit::SW() - 700.f) / 2.f, std::max(60.f, (uikit::SH() - 460.f) / 2.f), 700.f, 460.f);
}

sf::FloatRect Play_Scene::statsTabBounds(int tab) const {
    sf::FloatRect b = statsBounds();
    return sf::FloatRect(b.left + 20.f + static_cast<float>(tab) * 112.f, b.top + 52.f, 104.f, 30.f);
}

bool Play_Scene::handleStatsClick(const sf::Vector2f& p) {
    if (!statsOpen) return false;
    for (int t = 0; t < STATS_TABS; t++)
        if (statsTabBounds(t).contains(p)) { statsTab = t; audio.play("click", 0.6f); return true; }
    sf::FloatRect b = statsBounds();
    if (sf::FloatRect(b.left + b.width - 44.f, b.top + 12.f, 30.f, 30.f).contains(p)) { statsOpen = false; return true; }
    return b.contains(p);
}

// One or more series over the history, sharing the vertical scale
void Play_Scene::drawChart(const sf::FloatRect& r, const std::vector<std::vector<float>>& series,
                           const std::vector<sf::Color>& colors, bool symmetric) {
    using namespace uikit;
    roundedRect(_window, r, 8.f, sf::Color(10, 13, 19, 220), BORDER, 1.f);
    float lo = 0.f, hi = 1.f;
    for (const auto& s : series)
        for (float v : s) { lo = std::min(lo, v); hi = std::max(hi, v); }
    if (symmetric) { float m = std::max(std::fabs(lo), std::fabs(hi)); lo = -m; hi = m; }
    if (hi - lo < 1e-3f) hi = lo + 1.f;
    const float pad = 10.f;
    auto yOf = [&](float v) { return r.top + r.height - pad - (v - lo) / (hi - lo) * (r.height - 2.f * pad); };
    if (lo < 0.f && hi > 0.f) {
        sf::RectangleShape zero(sf::Vector2f(r.width - 2.f * pad, 1.f));
        zero.setPosition(r.left + pad, yOf(0.f));
        zero.setFillColor(sf::Color(255, 255, 255, 50));
        _window.draw(zero);
    }
    for (size_t k = 0; k < series.size(); k++) {
        const auto& s = series[k];
        if (s.size() < 2) continue;
        sf::VertexArray line(sf::LineStrip, s.size());
        for (size_t i = 0; i < s.size(); i++) {
            float x = r.left + pad + (r.width - 2.f * pad) * static_cast<float>(i) / static_cast<float>(HISTORY_DAYS - 1);
            line[i] = sf::Vertex(sf::Vector2f(x, yOf(s[i])), colors[k]);
        }
        _window.draw(line);
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.0f", hi);
    text(_window, buf, r.left + 6.f, r.top + 4.f, 10, TEXT_DIM);
    std::snprintf(buf, sizeof(buf), "%.0f", lo);
    text(_window, buf, r.left + 6.f, r.top + r.height - 16.f, 10, TEXT_DIM);
}

void Play_Scene::renderStats(const sf::Vector2f& mouse) {
    using namespace uikit;
    if (!statsOpen) return;
    sf::FloatRect b = statsBounds();
    panel(_window, b, 14.f);
    text(_window, "City Statistics", b.left + 20.f, b.top + 16.f, 18, TEXT, true);
    std::string sub = "Last " + std::to_string(history.size()) + " days  -  G to close";
    text(_window, sub, b.left + 220.f, b.top + 22.f, 11, TEXT_DIM);
    sf::FloatRect xb(b.left + b.width - 44.f, b.top + 12.f, 30.f, 30.f);
    roundedRect(_window, xb, 8.f, xb.contains(mouse) ? sf::Color(120, 40, 36) : PANEL_RAISED, BORDER, 1.f);
    textMid(_window, "x", xb.left + 10.f, xb.top - 1.f, xb.height, 15, TEXT_DIM, true);

    const char* tabs[STATS_TABS] = {"Population", "Money", "Happiness", "Demand", "Trade"};
    for (int t = 0; t < STATS_TABS; t++) {
        sf::FloatRect tb = statsTabBounds(t);
        bool active = statsTab == t;
        roundedRect(_window, tb, 8.f, active ? withAlpha(ACCENT, 70) : (tb.contains(mouse) ? PANEL_HOVER : PANEL_RAISED),
                    active ? ACCENT : BORDER, active ? 2.f : 1.f);
        textMid(_window, tabs[t], tb.left + (tb.width - textWidth(tabs[t], 12, true)) / 2.f, tb.top, tb.height, 12, TEXT, true);
    }

    sf::FloatRect chart(b.left + 20.f, b.top + 96.f, b.width - 40.f, 250.f);
    std::vector<float> a, c, d;
    for (const DaySample& s : history) {
        switch (statsTab) {
            case 0: a.push_back(static_cast<float>(s.population)); break;
            case 1: a.push_back(static_cast<float>(s.money)); c.push_back(static_cast<float>(s.net)); break;
            case 2: a.push_back(static_cast<float>(s.happiness)); break;
            case 3: a.push_back(static_cast<float>(s.demand[0])); c.push_back(static_cast<float>(s.demand[1]));
                    d.push_back(static_cast<float>(s.demand[2])); break;
            default: a.push_back(s.exports); c.push_back(s.imports); break;
        }
    }
    float y = chart.top + chart.height + 14.f;
    auto legend = [&](const sf::Color& col, const std::string& label, float x) {
        roundedRect(_window, sf::FloatRect(x, y + 3.f, 12.f, 12.f), 3.f, col);
        text(_window, label, x + 18.f, y, 12, TEXT);
    };
    const sf::Color green = GOOD, blue = ACCENT, orange(235, 140, 50), red = BAD;
    switch (statsTab) {
        case 0:
            drawChart(chart, {a}, {green}, false);
            legend(green, "Citizens (residents + workers): " + withCommas(a.empty() ? 0 : static_cast<long long>(a.back())), chart.left);
            break;
        case 1: {
            sf::FloatRect top(chart.left, chart.top, chart.width, 150.f), bottom(chart.left, chart.top + 160.f, chart.width, 90.f);
            drawChart(top, {a}, {blue}, false);
            drawChart(bottom, {c}, {orange}, true);
            legend(blue, "Treasury", chart.left);
            legend(orange, "Net per day", chart.left + 160.f);
            break;
        }
        case 2:
            drawChart(chart, {a}, {sf::Color(250, 205, 70)}, false);
            legend(sf::Color(250, 205, 70), "Average happiness (%)", chart.left);
            break;
        case 3:
            drawChart(chart, {a, c, d}, {green, blue, orange}, true);
            legend(green, "Residential", chart.left);
            legend(blue, "Commercial", chart.left + 160.f);
            legend(orange, "Industrial", chart.left + 320.f);
            break;
        default: {
            sf::FloatRect small(chart.left, chart.top, chart.width, 130.f);
            drawChart(small, {a, c}, {green, red}, false);
            legend(green, "Export income / day", chart.left);
            legend(red, "Import cost / day", chart.left + 200.f);
            // Today's breakdown
            float ty = small.top + small.height + 40.f;
            char buf[160];
            auto line = [&](const std::string& s, const sf::Color& col) { text(_window, s, chart.left, ty, 12, col); ty += 20.f; };
            std::snprintf(buf, sizeof(buf), "Goods   made %.0f  used %.0f  ->  exported %.0f, imported %.0f",
                          trade.goodsMade, trade.goodsUsed, trade.goodsExported, trade.goodsImported);
            line(buf, TEXT);
            std::snprintf(buf, sizeof(buf), "Food    made %.0f  used %.0f  ->  exported %.0f, imported %.0f",
                          trade.foodMade, trade.foodUsed, trade.foodExported, trade.foodImported);
            line(buf, TEXT);
            std::snprintf(buf, sizeof(buf), "Trade capacity %.0f a day (roads leaving the map + harbours)", trade.capacity);
            line(buf, TEXT_DIM);
            if (trade.shortage > 0.f) {
                std::snprintf(buf, sizeof(buf), "Shortage: %.0f%% of needs could not be imported - shops lose customers", trade.shortage * 100.f);
                line(buf, BAD);
            }
            if (trade.glut > 0.f) {
                std::snprintf(buf, sizeof(buf), "Glut: %.0f%% of production could not be shipped - add roads out or a harbour", trade.glut * 100.f);
                line(buf, WARN);
            }
            break;
        }
    }
    if (history.size() < 2) text(_window, "Graphs fill in as the days pass", chart.left + 14.f, chart.top + 14.f, 12, TEXT_DIM);
}
