#pragma once
// Play_Scene member definitions (included at the end of Play_Scene.hpp).
// Tiered progression: a rank needs population, happiness AND services.

static int countBuildings(const char* name) {
    int n = 0;
    for (int i = 0; i < tileMapW; i++)
        for (int j = 0; j < tileMapH; j++) {
            if (entities[i][j] == NULL) continue;
            Zone* z = dynamic_cast<Zone*>(entities[i][j]);
            if (z != NULL && !z->isSubTile && z->isActive && z->getName() == name) n++;
        }
    return n;
}

// Everything rank r asks for, each with whether the city meets it right now.
// Requirements build on each other: Town also asks for the Village basics.
std::vector<Play_Scene::RankCheck> Play_Scene::rankRequirements(int r) const {
    std::vector<RankCheck> out;
    if (r <= 0 || r >= CITY_RANK_COUNT) return out;
    const CityRank& rank = CITY_RANKS[r];
    char buf[96];
    auto add = [&](const std::string& label, bool met, const std::string& value) { out.push_back({label, met, value}); };

    const int pop = townPopulation[0] + townPopulation[1] + townPopulation[2];
    add(std::to_string(rank.population) + " citizens", pop >= rank.population, uikit::withCommas(pop));
    const int happy = happinessSystem.cityHappiness;
    add(std::to_string(rank.happiness) + "% happiness", happy >= rank.happiness, std::to_string(happy) + "%");

    // Village and up: utilities and garbage for (almost) everyone
    const float consumers = static_cast<float>(std::max(1, utilities.consumers));
    const float powerShare = 1.f - static_cast<float>(utilities.unpowered) / consumers;
    const float waterShare = 1.f - static_cast<float>(utilities.unwatered) / consumers;
    std::snprintf(buf, sizeof(buf), "%d%% / %d%%", static_cast<int>(powerShare * 100.f), static_cast<int>(waterShare * 100.f));
    add("Power and water for 95% of buildings", utilities.consumers > 0 && powerShare >= 0.95f && waterShare >= 0.95f, buf);
    const float garbageShare = services.garbageBuildings > 0
        ? 1.f - static_cast<float>(services.unservedGarbage) / static_cast<float>(services.garbageBuildings) : 0.f;
    std::snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(garbageShare * 100.f));
    add("Garbage collected for 95% of buildings", services.garbageBuildings > 0 && garbageShare >= 0.95f, buf);
    if (r == 1) return out;

    // Town and up: the basic public services
    for (const auto& s : {std::make_pair("SchoolZone", "A school"), std::make_pair("PoliceZone", "A police station"),
                          std::make_pair("FireZone", "A fire station"), std::make_pair("ClinicZone", "A clinic")}) {
        int n = countBuildings(s.first);
        add(s.second, n > 0, std::to_string(n));
    }
    if (r == 2) return out;

    // City and up: sewers, buses, education, land value
    std::snprintf(buf, sizeof(buf), "%d%%", static_cast<int>((1.f - utilities.unseweredShare) * 100.f));
    add("Sewers for 90% of homes", utilities.unseweredShare <= 0.1f && utilities.sewageCapacity > 0.f, buf);
    int buses = countBuildings("BusDepotZone");
    add("A bus depot", buses > 0, std::to_string(buses));
    const int eduNeed = r == 3 ? 40 : 60, landNeed = r == 3 ? 40 : 50;
    add("Education " + std::to_string(eduNeed) + "+", economy.avgEducation >= static_cast<float>(eduNeed),
        std::to_string(static_cast<int>(economy.avgEducation)));
    add("Land value " + std::to_string(landNeed) + "+ under homes", economy.avgLandValue >= static_cast<float>(landNeed),
        std::to_string(static_cast<int>(economy.avgLandValue)));
    if (r == 3) return out;

    // Metropolis: clean waste and flowing traffic
    int cleanWaste = countBuildings("RecyclingZone") + countBuildings("IncineratorZone");
    add("Recycling or an incinerator", cleanWaste > 0, std::to_string(cleanWaste));
    std::snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(traffic.cityCongestion * 100.f));
    add("Traffic under 100% of capacity", traffic.cityCongestion < 1.f, buf);
    return out;
}

bool Play_Scene::meetsRank(int r) const {
    for (const RankCheck& c : rankRequirements(r))
        if (!c.met) return false;
    return r > 0 && r < CITY_RANK_COUNT;
}

// Daily: climb every rank whose requirements are all met (never down)
void Play_Scene::checkMilestones() {
    const int pop = townPopulation[0] + townPopulation[1] + townPopulation[2];
    bestPopulation = std::max(bestPopulation, pop);
    while (rankReached + 1 < CITY_RANK_COUNT && meetsRank(rankReached + 1)) {
        const int r = ++rankReached;
        if (!milestonesEnabled) continue;
        money += CITY_RANKS[r].reward;
        std::string unlocked;
        for (int t = TOOL_NONE + 1; t <= TOOL_SEWAGE_PLANT; t++) {
            if (toolRank(static_cast<BuildTool>(t)) != r) continue;
            std::string name = toolbar.titleOf(static_cast<BuildTool>(t));
            if (name.empty()) continue;
            if (!unlocked.empty()) unlocked += ", ";
            unlocked += name;
        }
        notify(std::string("Chime is now a ") + CITY_RANKS[r].name + "! +" + uikit::formatMoney(CITY_RANKS[r].reward) +
               (unlocked.empty() ? std::string() : " - new: " + unlocked), false);
        audio.play("milestone");
    }
}

// "next: Town 3/8" for the top bar
std::string Play_Scene::rankProgressText() const {
    if (rankReached + 1 >= CITY_RANK_COUNT) return CITY_RANKS[rankReached].name;
    const auto checks = rankRequirements(rankReached + 1);
    int met = 0;
    for (const RankCheck& c : checks) met += c.met ? 1 : 0;
    return std::string(CITY_RANKS[rankReached].name) + " - next " + CITY_RANKS[rankReached + 1].name + " " +
           std::to_string(met) + "/" + std::to_string(checks.size());
}

// Stats panel tab: the next rank's checklist
void Play_Scene::renderRankTab(const sf::FloatRect& area) {
    using namespace uikit;
    const int next = rankReached + 1;
    std::string head = std::string("Chime is a ") + CITY_RANKS[rankReached].name;
    text(_window, head, area.left, area.top, 16, ACCENT, true);
    if (next >= CITY_RANK_COUNT) {
        text(_window, "The highest rank - well done, mayor.", area.left, area.top + 30.f, 12, TEXT_DIM);
        return;
    }
    const CityRank& nr = CITY_RANKS[next];
    std::string sub = std::string("To become a ") + nr.name + " (+" + formatMoney(nr.reward) + ", unlocks new buildings):";
    text(_window, sub, area.left, area.top + 28.f, 12, TEXT_DIM);
    float y = area.top + 56.f;
    for (const RankCheck& c : rankRequirements(next)) {
        sf::Color col = c.met ? GOOD : BAD;
        roundedRect(_window, sf::FloatRect(area.left, y + 2.f, 14.f, 14.f), 4.f, withAlpha(col, 60), col, 1.f);
        if (c.met) textMid(_window, "v", area.left + 3.f, y + 1.f, 14.f, 10, col, true);
        text(_window, c.label, area.left + 24.f, y, 12, c.met ? TEXT : sf::Color(235, 200, 190));
        textRight(_window, c.value, area.left + area.width, y, 12, col, true);
        y += 24.f;
    }
}
