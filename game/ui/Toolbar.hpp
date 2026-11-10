#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include "./UiKit.hpp"
#include "../simulation/EconomyData.hpp"

enum BuildTool {
    TOOL_NONE,
    // Individual Residential Buildings
    TOOL_RES_COTTAGES,    // T1 Suburban Cottages
    TOOL_RES_APARTMENTS,  // T2 Brick Apartments
    TOOL_RES_TOWER,       // T3 Luxury Highrise

    // Individual Commercial Buildings
    TOOL_COM_GROCERY,     // T1 Corner Grocery
    TOOL_COM_PLAZA,       // T2 Shopping Plaza
    TOOL_COM_TOWER,       // T3 Banking Tower

    // Individual Industrial Buildings
    TOOL_IND_WORKSHOP,    // T1 Scrap Workshop
    TOOL_IND_FACTORY,     // T2 Manufacturing Plant
    TOOL_IND_TECH,        // T3 Microchip Clean-Fab

    // Agricultural Farms
    TOOL_FARM_BARN,       // Red Barn & Silo
    TOOL_FARM_WINDMILL,   // Windmill Homestead

    // Civics
    TOOL_SCHOOL,          // Elementary School
    TOOL_CHURCH,          // Community Church
    TOOL_PARK,            // Nature Park
    TOOL_HARBOUR,         // Trade Harbour

    // Infrastructure
    TOOL_ROAD,            // Dirt Road
    TOOL_ROAD_ASPHALT,    // Asphalt Road (also upgrades dirt)
    TOOL_ROAD_AVENUE,     // Avenue (also upgrades dirt / asphalt)
    TOOL_ROUNDABOUT,      // Converts a 3- or 4-way junction
    TOOL_WATER,           // Water Body
    TOOL_BULLDOZE,        // Demolish ($5, $15 forest, $60 rock)

    // Power & water (2x2 utility buildings, same order as UtilityKind)
    TOOL_POWER_COAL,
    TOOL_POWER_WIND,
    TOOL_POWER_SOLAR,
    TOOL_TRANSFORMER,
    TOOL_WATER_WELL,
    TOOL_WATER_PUMP,
    TOOL_WATER_TREATMENT,
    TOOL_WATER_TOWER,
    TOOL_POWER_LINE,      // 1x1 drag: overground power line
    TOOL_WATER_PIPE,      // 1x1 drag: underground water pipe

    // Zones (rectangle drag): R / C / I x Low / Medium / High density, in makeZone order
    TOOL_ZONE_R_LOW, TOOL_ZONE_R_MED, TOOL_ZONE_R_HIGH,
    TOOL_ZONE_C_LOW, TOOL_ZONE_C_MED, TOOL_ZONE_C_HIGH,
    TOOL_ZONE_I_LOW, TOOL_ZONE_I_MED, TOOL_ZONE_I_HIGH,
    TOOL_DEZONE,          // clears zones (buildings stay)

    // City services (2x2, same order as ServiceKind)
    TOOL_POLICE, TOOL_FIRE_STATION, TOOL_CLINIC, TOOL_LANDFILL, TOOL_MINE, TOOL_BUS_DEPOT,
    TOOL_INCINERATOR, TOOL_RECYCLING,
    // Sewage (utility buildings)
    TOOL_SEWAGE_OUTFLOW, TOOL_SEWAGE_PLANT,

    // Compatibility Aliases
    TOOL_RESIDENTIAL = TOOL_RES_COTTAGES,
    TOOL_COMMERCIAL  = TOOL_COM_GROCERY,
    TOOL_INDUSTRIAL  = TOOL_IND_WORKSHOP,
    TOOL_FARM        = TOOL_FARM_BARN
};

// Price of each tool: the single source for placement checks and the build cards
inline int toolCost(BuildTool tool) {
    switch (tool) {
        case TOOL_RES_COTTAGES:   return 100;
        case TOOL_RES_APARTMENTS: return 250;
        case TOOL_RES_TOWER:      return 600;
        case TOOL_COM_GROCERY:    return 120;
        case TOOL_COM_PLAZA:      return 280;
        case TOOL_COM_TOWER:      return 750;
        case TOOL_IND_WORKSHOP:   return 150;
        case TOOL_IND_FACTORY:    return 350;
        case TOOL_IND_TECH:       return 800;
        case TOOL_FARM_BARN:      return 70;
        case TOOL_FARM_WINDMILL:  return 90;
        case TOOL_ROAD:           return ROAD_COST[0];
        case TOOL_ROAD_ASPHALT:   return ROAD_COST[1];
        case TOOL_ROAD_AVENUE:    return ROAD_COST[2];
        case TOOL_ROUNDABOUT:     return ROUNDABOUT_COST;
        case TOOL_HARBOUR:        return 250;
        case TOOL_WATER:          return 20;
        case TOOL_SCHOOL:         return 200;
        case TOOL_CHURCH:         return 180;
        case TOOL_PARK:           return 80;
        case TOOL_BULLDOZE:       return 5;
        case TOOL_POWER_COAL: case TOOL_POWER_WIND: case TOOL_POWER_SOLAR: case TOOL_TRANSFORMER:
        case TOOL_WATER_WELL: case TOOL_WATER_PUMP: case TOOL_WATER_TREATMENT: case TOOL_WATER_TOWER:
            return UTIL_COST[tool - TOOL_POWER_COAL];
        case TOOL_POWER_LINE:     return LINE_COST_POWER;
        case TOOL_WATER_PIPE:     return LINE_COST_PIPE;
        case TOOL_ZONE_R_LOW: case TOOL_ZONE_R_MED: case TOOL_ZONE_R_HIGH:
        case TOOL_ZONE_C_LOW: case TOOL_ZONE_C_MED: case TOOL_ZONE_C_HIGH:
        case TOOL_ZONE_I_LOW: case TOOL_ZONE_I_MED: case TOOL_ZONE_I_HIGH:
            return ZONE_COST_PER_TILE;
        case TOOL_DEZONE:         return 0;
        case TOOL_POLICE: case TOOL_FIRE_STATION: case TOOL_CLINIC:
        case TOOL_LANDFILL: case TOOL_MINE: case TOOL_BUS_DEPOT:
        case TOOL_INCINERATOR: case TOOL_RECYCLING:
            return SERVICE_COST[tool - TOOL_POLICE];
        case TOOL_SEWAGE_OUTFLOW: return UTIL_COST[UTIL_SEWAGE_OUTFLOW];
        case TOOL_SEWAGE_PLANT:   return UTIL_COST[UTIL_SEWAGE_PLANT];
        default: return 0;
    }
}

// Tools that lay road tiles; the value is the road tier they build
inline bool isRoadTool(BuildTool tool) {
    return tool == TOOL_ROAD || tool == TOOL_ROAD_ASPHALT || tool == TOOL_ROAD_AVENUE;
}
inline int roadToolTier(BuildTool tool) {
    return tool == TOOL_ROAD_AVENUE ? 2 : (tool == TOOL_ROAD_ASPHALT ? 1 : 0);
}

// Power & water tools; the value is the UtilityKind they build
inline bool isUtilityTool(BuildTool tool) {
    return (tool >= TOOL_POWER_COAL && tool <= TOOL_WATER_TOWER) || tool == TOOL_SEWAGE_OUTFLOW || tool == TOOL_SEWAGE_PLANT;
}
inline int utilityToolKind(BuildTool tool) {
    if (tool == TOOL_SEWAGE_OUTFLOW) return UTIL_SEWAGE_OUTFLOW;
    if (tool == TOOL_SEWAGE_PLANT) return UTIL_SEWAGE_PLANT;
    return (tool >= TOOL_POWER_COAL && tool <= TOOL_WATER_TOWER) ? static_cast<int>(tool - TOOL_POWER_COAL) : -1;
}
// Power line / water pipe tools; the value is the LINE_* bit they lay
// Service tools; the value is the ServiceKind they build
inline bool isServiceTool(BuildTool tool) { return tool >= TOOL_POLICE && tool <= TOOL_RECYCLING; }
inline int serviceToolKind(BuildTool tool) { return isServiceTool(tool) ? static_cast<int>(tool - TOOL_POLICE) : -1; }
// Zone tools; the value is the TileData::zone they paint (1 + sector * 3 + density)
inline bool isZoneTool(BuildTool tool) { return tool >= TOOL_ZONE_R_LOW && tool <= TOOL_ZONE_I_HIGH; }
inline uint8_t zoneToolValue(BuildTool tool) { return isZoneTool(tool) ? static_cast<uint8_t>(1 + (tool - TOOL_ZONE_R_LOW)) : 0; }
inline bool isLineTool(BuildTool tool) { return tool == TOOL_POWER_LINE || tool == TOOL_WATER_PIPE; }
inline uint8_t lineToolBit(BuildTool tool) { return tool == TOOL_POWER_LINE ? 1 : (tool == TOOL_WATER_PIPE ? 2 : 0); }

// -------------------------------------------------------------
// City ranks: reaching a population unlocks buildings and pays a bonus
// -------------------------------------------------------------
struct CityRank { const char* name; int population; int reward; };
const CityRank CITY_RANKS[] = {
    {"Settlement", 0, 0}, {"Village", 300, 5000}, {"Town", 1000, 15000}, {"City", 2500, 30000}, {"Metropolis", 5000, 60000},
};
const int CITY_RANK_COUNT = 5;
inline int rankForPopulation(int pop) {
    int r = 0;
    for (int k = 0; k < CITY_RANK_COUNT; k++) if (pop >= CITY_RANKS[k].population) r = k;
    return r;
}
// Rank a tool needs (0 = available from the start)
inline int toolRank(BuildTool tool) {
    switch (tool) {
        case TOOL_ROAD_ASPHALT: case TOOL_ZONE_R_MED: case TOOL_ZONE_C_MED: case TOOL_ZONE_I_MED:
        case TOOL_RES_APARTMENTS: case TOOL_COM_PLAZA: case TOOL_IND_FACTORY:
        case TOOL_POLICE: case TOOL_CLINIC: case TOOL_POWER_SOLAR: case TOOL_WATER_PUMP: case TOOL_WATER_TREATMENT:
        case TOOL_WATER_TOWER: case TOOL_BUS_DEPOT: case TOOL_SEWAGE_OUTFLOW: case TOOL_CHURCH:
            return 1;
        case TOOL_ROAD_AVENUE: case TOOL_ROUNDABOUT: case TOOL_ZONE_R_HIGH: case TOOL_ZONE_C_HIGH: case TOOL_ZONE_I_HIGH:
        case TOOL_RES_TOWER: case TOOL_COM_TOWER: case TOOL_IND_TECH: case TOOL_POWER_COAL: case TOOL_INCINERATOR:
        case TOOL_SEWAGE_PLANT: case TOOL_MINE: case TOOL_HARBOUR:
            return 2;
        case TOOL_RECYCLING:
            return 3;
        default:
            return 0;
    }
}

// Texture shown on the tool's card and used as its placement ghost
inline std::string toolThumbnailKey(BuildTool tool) {
    switch (tool) {
        case TOOL_RES_COTTAGES:   return "res_t1_v1";
        case TOOL_RES_APARTMENTS: return "res_t2_v1";
        case TOOL_RES_TOWER:      return "res_t3_v1";
        case TOOL_COM_GROCERY:    return "com_t1_v1";
        case TOOL_COM_PLAZA:      return "com_t2_v1";
        case TOOL_COM_TOWER:      return "com_t3_v1";
        case TOOL_IND_WORKSHOP:   return "ind_t1_v1";
        case TOOL_IND_FACTORY:    return "ind_t2_v1";
        case TOOL_IND_TECH:       return "ind_t3_v1";
        case TOOL_FARM_BARN:      return "farm_v1";
        case TOOL_FARM_WINDMILL:  return "farm_v2";
        case TOOL_SCHOOL:         return "school";
        case TOOL_CHURCH:         return "church";
        case TOOL_PARK:           return "park";
        case TOOL_HARBOUR:        return "harbour";
        case TOOL_ROAD:           return "road_dirt_icon";
        case TOOL_ROAD_ASPHALT:   return "road_asphalt_icon";
        case TOOL_ROAD_AVENUE:    return "road_avenue_icon";
        case TOOL_ROUNDABOUT:     return "roundabout_icon";
        case TOOL_WATER:          return "water";
        case TOOL_BULLDOZE:       return "ui_bulldoze";
        case TOOL_POWER_COAL:     return "util_coal";
        case TOOL_POWER_WIND:     return "util_wind";
        case TOOL_POWER_SOLAR:    return "util_solar";
        case TOOL_TRANSFORMER:    return "util_transformer";
        case TOOL_WATER_WELL:     return "util_well";
        case TOOL_WATER_PUMP:     return "util_pump";
        case TOOL_WATER_TREATMENT: return "util_treatment";
        case TOOL_WATER_TOWER:    return "util_tower";
        case TOOL_POWER_LINE:     return "ui_power_line";
        case TOOL_WATER_PIPE:     return "ui_water_pipe";
        case TOOL_ZONE_R_LOW: case TOOL_ZONE_R_MED: case TOOL_ZONE_R_HIGH: return "zone_residental";
        case TOOL_ZONE_C_LOW: case TOOL_ZONE_C_MED: case TOOL_ZONE_C_HIGH: return "zone_commercial";
        case TOOL_ZONE_I_LOW: case TOOL_ZONE_I_MED: case TOOL_ZONE_I_HIGH: return "zone_industrial";
        case TOOL_DEZONE:         return "ui_bulldoze";
        case TOOL_POLICE:         return "svc_police";
        case TOOL_FIRE_STATION:   return "svc_fire";
        case TOOL_CLINIC:         return "svc_clinic";
        case TOOL_LANDFILL:       return "svc_landfill";
        case TOOL_MINE:           return "svc_mine";
        case TOOL_BUS_DEPOT:      return "svc_bus";
        case TOOL_INCINERATOR:    return "svc_incinerator";
        case TOOL_RECYCLING:      return "svc_recycling";
        case TOOL_SEWAGE_OUTFLOW: return "util_outflow";
        case TOOL_SEWAGE_PLANT:   return "util_sewage";
        default:                  return "";
    }
}

enum class ToolbarCategory {
    INSPECT,
    RESIDENTIAL,
    COMMERCIAL,
    INDUSTRIAL,
    CIVICS,
    INFRASTRUCTURE,
    UTILITIES,
    WASTE
};

struct ToolItem {
    BuildTool tool;
    std::string title;      // e.g. "SUBURBAN COTTAGES" (shown in title case)
    std::string subtitle;   // one-line description on the card
    std::string hotkey;     // "" when the tool has none
    sf::Color accentColor;
    ToolbarCategory category;
};

struct CategoryTab {
    ToolbarCategory category;
    std::string title;
    std::string icon;       // ui icon name
    std::string hotkeyHint; // shown in the tooltip
    sf::Color accentColor;
    sf::FloatRect bounds;
};

// -------------------------------------------------------------
// Build menu: left sidebar of category icons + flyout of build cards
// -------------------------------------------------------------
// Clicking a category opens its flyout; clicking a card selects that tool
// and closes the flyout so the map is clear. While a tool is active, a
// pill at the bottom-left shows what is being placed (click X or Esc to stop).

class Toolbar {
    BuildTool currentTool = TOOL_NONE;
public:
    int dragCostPreview = -1; // total cost of the drag in progress (-1 = show the tool price)
    std::string pillNote;     // extra line on the pill (e.g. a well's expected output here)
    int unlockedRank = 99;    // tools needing a higher rank are shown locked
private:
    ToolbarCategory activeCategory = ToolbarCategory::INSPECT;
    bool flyoutOpen = false;

    float windowH = 900.f;
    std::vector<CategoryTab> tabs;
    std::vector<ToolItem> allTools;

    // Layout
    static constexpr float SIDE_X = 10.f, SIDE_Y = 58.f, SIDE_W = 60.f;
    static constexpr float BTN = 48.f, BTN_GAP = 8.f;
    static constexpr float FLY_X = 78.f, FLY_W = 280.f, FLY_HEADER = 40.f;
    static constexpr float CARD_H = 64.f, CARD_GAP = 6.f;
    static constexpr float PILL_W = 340.f, PILL_H = 48.f;

    ToolbarCategory categoryOf(BuildTool tool) const {
        for (const auto& item : allTools)
            if (item.tool == tool) return item.category;
        return ToolbarCategory::INSPECT;
    }

    std::vector<const ToolItem*> toolsIn(ToolbarCategory c) const {
        std::vector<const ToolItem*> out;
        for (const auto& item : allTools)
            if (item.category == c) out.push_back(&item);
        return out;
    }

    const CategoryTab* tabFor(ToolbarCategory c) const {
        for (const auto& t : tabs)
            if (t.category == c) return &t;
        return nullptr;
    }

public:
    Toolbar() = default;

    void init(float windowW, float windowH_) {
        (void)windowW;
        windowH = windowH_;

        tabs.clear();
        auto addTab = [&](ToolbarCategory c, const std::string& title, const std::string& icon,
                          const std::string& hint, sf::Color accent) {
            float y = SIDE_Y + 8.f + static_cast<float>(tabs.size()) * (BTN + BTN_GAP);
            tabs.push_back({c, title, icon, hint, accent, sf::FloatRect(SIDE_X + (SIDE_W - BTN) / 2.f, y, BTN, BTN)});
        };
        addTab(ToolbarCategory::INSPECT, "Inspect", "inspect", "Esc", sf::Color(64, 160, 230));
        addTab(ToolbarCategory::RESIDENTIAL, "Residential", "residential", "R", sf::Color(72, 200, 110));
        addTab(ToolbarCategory::COMMERCIAL, "Commercial", "commercial", "C", sf::Color(70, 160, 235));
        addTab(ToolbarCategory::INDUSTRIAL, "Industry & Farms", "industrial", "I / F", sf::Color(235, 140, 50));
        addTab(ToolbarCategory::CIVICS, "Civic & Ports", "civic", "1 2 3 H", sf::Color(170, 120, 220));
        addTab(ToolbarCategory::INFRASTRUCTURE, "Roads", "road", "Space W X", sf::Color(160, 172, 186));
        addTab(ToolbarCategory::UTILITIES, "Power & Water", "utilities", "U", sf::Color(245, 200, 60));
        addTab(ToolbarCategory::WASTE, "Waste & Sewage", "waste", "", sf::Color(150, 130, 90));

        allTools.clear();
        auto add = [&](BuildTool t, const char* title, const char* sub, const char* key, sf::Color c, ToolbarCategory cat) {
            allTools.push_back({t, title, sub, key, c, cat});
        };
        using C = ToolbarCategory;
        // Zones first (buildings grow by themselves), then buildings placed by hand (fixed tier)
        const sf::Color rc(72, 200, 110), cc(70, 160, 235), ic(235, 140, 50);
        add(TOOL_ZONE_R_LOW, "LOW DENSITY", "Zone: houses, tier 1 - $5/tile", "R", rc, C::RESIDENTIAL);
        add(TOOL_ZONE_R_MED, "MEDIUM DENSITY", "Zone: grows to tier 2 - $5/tile", "", rc, C::RESIDENTIAL);
        add(TOOL_ZONE_R_HIGH, "HIGH DENSITY", "Zone: grows to tier 3 - $5/tile", "", rc, C::RESIDENTIAL);
        add(TOOL_RES_COTTAGES, "SUBURBAN COTTAGES", "Placed, fixed tier 1", "", rc, C::RESIDENTIAL);
        add(TOOL_RES_APARTMENTS, "BRICK APARTMENTS", "Placed, fixed tier 2", "", rc, C::RESIDENTIAL);
        add(TOOL_RES_TOWER, "LUXURY HIGHRISE", "Placed, fixed tier 3", "", rc, C::RESIDENTIAL);

        add(TOOL_ZONE_C_LOW, "LOW DENSITY", "Zone: shops, tier 1 - $5/tile", "C", cc, C::COMMERCIAL);
        add(TOOL_ZONE_C_MED, "MEDIUM DENSITY", "Zone: grows to tier 2 - $5/tile", "", cc, C::COMMERCIAL);
        add(TOOL_ZONE_C_HIGH, "HIGH DENSITY", "Zone: grows to tier 3 - $5/tile", "", cc, C::COMMERCIAL);
        add(TOOL_COM_GROCERY, "CORNER GROCERY", "Placed, fixed tier 1", "", cc, C::COMMERCIAL);
        add(TOOL_COM_PLAZA, "SHOPPING PLAZA", "Placed, fixed tier 2", "", cc, C::COMMERCIAL);
        add(TOOL_COM_TOWER, "CORPORATE TOWER", "Placed, fixed tier 3", "", cc, C::COMMERCIAL);

        add(TOOL_ZONE_I_LOW, "LOW DENSITY", "Zone: industry, tier 1 - $5/tile", "I", ic, C::INDUSTRIAL);
        add(TOOL_ZONE_I_MED, "MEDIUM DENSITY", "Zone: grows to tier 2 - $5/tile", "", ic, C::INDUSTRIAL);
        add(TOOL_ZONE_I_HIGH, "HIGH DENSITY", "Zone: grows to tier 3 - $5/tile", "", ic, C::INDUSTRIAL);
        add(TOOL_IND_WORKSHOP, "SCRAP WORKSHOP", "Placed, fixed tier 1", "", ic, C::INDUSTRIAL);
        add(TOOL_IND_FACTORY, "FACTORY PLANT", "Placed, fixed tier 2", "", ic, C::INDUSTRIAL);
        add(TOOL_IND_TECH, "CLEAN-FAB HUB", "Placed, fixed tier 3", "", ic, C::INDUSTRIAL);
        add(TOOL_FARM, "FARMS", "Barn & silo - best on rich soil", "F", sf::Color(150, 205, 90), C::INDUSTRIAL);
        add(TOOL_FARM_WINDMILL, "WINDMILL FARM", "Windmill - best on rich soil", "", sf::Color(150, 205, 90), C::INDUSTRIAL);
        add(TOOL_MINE, "MINE", "30 jobs on a coal / iron / stone deposit", "", sf::Color(150, 140, 130), C::INDUSTRIAL);

        add(TOOL_SCHOOL, "ELEMENTARY SCHOOL", "Education r5 - $25/day", "1", sf::Color(235, 87, 72), C::CIVICS);
        add(TOOL_CHURCH, "COMMUNITY CHURCH", "Faith r4 - $20/day", "2", sf::Color(170, 120, 220), C::CIVICS);
        add(TOOL_PARK, "NATURE PARK", "Health r4 - $10/day", "3", sf::Color(72, 200, 110), C::CIVICS);
        add(TOOL_HARBOUR, "TRADE HARBOUR", "Port jobs - $40/day", "H", sf::Color(64, 160, 230), C::CIVICS);
        add(TOOL_POLICE, "POLICE STATION", "Safety r7 - $12/day", "", sf::Color(70, 110, 220), C::CIVICS);
        add(TOOL_FIRE_STATION, "FIRE STATION", "Stops fires r7 - $12/day", "", sf::Color(230, 70, 50), C::CIVICS);
        add(TOOL_CLINIC, "CLINIC", "Health r7 - $15/day", "", sf::Color(235, 235, 240), C::CIVICS);

        add(TOOL_ROAD, "DIRT ROAD", "40 cars/day - $1/day - drag", "Space", sf::Color(190, 196, 204), C::INFRASTRUCTURE);
        add(TOOL_ROAD_ASPHALT, "ASPHALT ROAD", "120 cars/day - $2/day - upgrades", "", sf::Color(150, 156, 168), C::INFRASTRUCTURE);
        add(TOOL_ROAD_AVENUE, "AVENUE", "300 cars/day - $5/day - upgrades", "", sf::Color(226, 196, 70), C::INFRASTRUCTURE);
        add(TOOL_ROUNDABOUT, "ROUNDABOUT", "Junction capacity x2 - $2/day", "", sf::Color(110, 190, 120), C::INFRASTRUCTURE);
        add(TOOL_WATER, "WATER BODY", "1x1 - drag to fill", "W", sf::Color(64, 140, 220), C::INFRASTRUCTURE);
        add(TOOL_BULLDOZE, "DEMOLISH", "Clear tiles - drag", "X", sf::Color(235, 87, 72), C::INFRASTRUCTURE);
        add(TOOL_DEZONE, "DEZONE", "Remove zones - free - drag", "", sf::Color(200, 120, 90), C::INFRASTRUCTURE);
        add(TOOL_BUS_DEPOT, "BUS DEPOT", "30% fewer cars r8 - $15/day", "", sf::Color(240, 190, 50), C::INFRASTRUCTURE);

        const sf::Color power(245, 200, 60), water(64, 160, 230);
        add(TOOL_POWER_COAL, "COAL PLANT", "400 power, pollutes - $20/day", "U", power, C::UTILITIES);
        add(TOOL_POWER_WIND, "WIND TURBINES", "60 power, clean - $3/day", "", power, C::UTILITIES);
        add(TOOL_POWER_SOLAR, "SOLAR FARM", "120 power, less in winter", "", power, C::UTILITIES);
        add(TOOL_TRANSFORMER, "TRANSFORMER", "Delivers 150 power - $4/day", "", power, C::UTILITIES);
        add(TOOL_POWER_LINE, "POWER LINE", "$8 per tile - drag", "", power, C::UTILITIES);
        add(TOOL_WATER_WELL, "WATER WELL", "80 clean water - $1/day", "", water, C::UTILITIES);
        add(TOOL_WATER_PUMP, "UNDERGROUND PUMP", "400 raw water - $8/day", "", water, C::UTILITIES);
        add(TOOL_WATER_TREATMENT, "WATER TREATMENT", "Cleans 400 raw - $12/day", "", water, C::UTILITIES);
        add(TOOL_WATER_TOWER, "WATER TOWER", "Stores 300 water - $5/day", "", water, C::UTILITIES);
        add(TOOL_WATER_PIPE, "WATER PIPE", "Underground, $6 per tile - drag", "", water, C::UTILITIES);
        const sf::Color waste(150, 130, 90);
        add(TOOL_LANDFILL, "LANDFILL", "Stores 15,000 garbage - $8/day", "", waste, C::WASTE);
        add(TOOL_INCINERATOR, "INCINERATOR", "Burns 200/day, makes power - pollutes", "", waste, C::WASTE);
        add(TOOL_RECYCLING, "RECYCLING CENTRE", "Recycles 120/day cleanly - $20/day", "", waste, C::WASTE);
        add(TOOL_SEWAGE_OUTFLOW, "SEWAGE OUTFLOW", "300 sewage into water - pollutes", "", sf::Color(120, 110, 80), C::WASTE);
        add(TOOL_SEWAGE_PLANT, "SEWAGE TREATMENT", "Treats 300 sewage cleanly - $8/day", "", sf::Color(120, 110, 80), C::WASTE);
    }

    // ---- state ----
    BuildTool getActiveTool() const { return currentTool; }

    std::string titleOf(BuildTool tool) const {
        for (const auto& item : allTools)
            if (item.tool == tool) return uikit::titleCase(item.title);
        return "";
    }

    std::string getActiveToolName() const {
        for (const auto& item : allTools)
            if (item.tool == currentTool) return item.title;
        return "INSPECT";
    }

    // Hotkeys select directly without opening the flyout
    void setActiveTool(BuildTool tool) {
        currentTool = tool;
        activeCategory = (tool == TOOL_NONE) ? ToolbarCategory::INSPECT : categoryOf(tool);
        if (tool == TOOL_NONE) flyoutOpen = false;
    }

    bool isFlyoutOpen() const { return flyoutOpen; }
    void openFlyout(ToolbarCategory c) {
        if (c == ToolbarCategory::INSPECT) return;
        activeCategory = c;
        flyoutOpen = true;
    }
    ToolbarCategory getActiveCategory() const { return activeCategory; }

    // ---- layout ----
    sf::FloatRect sidebarBounds() const {
        return sf::FloatRect(SIDE_X, SIDE_Y, SIDE_W, 16.f + static_cast<float>(tabs.size()) * (BTN + BTN_GAP) - BTN_GAP);
    }
    sf::FloatRect tabBounds(ToolbarCategory c) const {
        const CategoryTab* t = tabFor(c);
        return t ? t->bounds : sf::FloatRect();
    }
    // Cards stack in one column; a category too tall for the window (above
    // the tool pill) wraps into two columns
    size_t flyoutColumns(size_t n) const {
        float room = windowH - SIDE_Y - FLY_HEADER - PILL_H - 24.f;
        size_t fit = static_cast<size_t>(std::max(1.f, std::floor(room / (CARD_H + CARD_GAP))));
        return n > fit ? 2 : 1;
    }
    size_t flyoutRows(size_t n) const {
        size_t cols = flyoutColumns(n);
        return (n + cols - 1) / cols;
    }
    sf::FloatRect flyoutBounds() const {
        size_t n = toolsIn(activeCategory).size();
        float h = FLY_HEADER + static_cast<float>(flyoutRows(n)) * (CARD_H + CARD_GAP) + 4.f;
        float w = static_cast<float>(flyoutColumns(n)) * (FLY_W - 8.f) + 8.f;
        return sf::FloatRect(FLY_X, SIDE_Y, w, h);
    }
    sf::FloatRect cardBoundsIn(size_t i, size_t n) const {
        size_t rows = flyoutRows(n);
        float col = static_cast<float>(i / rows), row = static_cast<float>(i % rows);
        return sf::FloatRect(FLY_X + 8.f + col * (FLY_W - 8.f), SIDE_Y + FLY_HEADER + row * (CARD_H + CARD_GAP), FLY_W - 16.f, CARD_H);
    }
    // Card for the i-th tool of the open category
    sf::FloatRect cardBounds(size_t i) const { return cardBoundsIn(i, toolsIn(activeCategory).size()); }
    sf::FloatRect cardBoundsFor(BuildTool tool) const {
        auto list = toolsIn(categoryOf(tool));
        for (size_t i = 0; i < list.size(); i++)
            if (list[i]->tool == tool) return cardBoundsIn(i, list.size());
        return sf::FloatRect();
    }
    sf::FloatRect pillBounds() const {
        return sf::FloatRect(SIDE_X, windowH - PILL_H - 12.f, PILL_W, PILL_H);
    }
    sf::FloatRect pillCloseBounds() const {
        sf::FloatRect p = pillBounds();
        return sf::FloatRect(p.left + p.width - 38.f, p.top + 9.f, 30.f, 30.f);
    }

    bool contains(const sf::Vector2f& point) const {
        if (sidebarBounds().contains(point)) return true;
        if (flyoutOpen && flyoutBounds().contains(point)) return true;
        if (currentTool != TOOL_NONE && pillBounds().contains(point)) return true;
        return false;
    }

    // ---- input ----
    bool handleClick(const sf::Vector2f& point) {
        // Active tool pill: X cancels
        if (currentTool != TOOL_NONE && pillBounds().contains(point)) {
            if (pillCloseBounds().contains(point)) setActiveTool(TOOL_NONE);
            return true;
        }

        // Category icons
        for (const auto& tab : tabs) {
            if (!tab.bounds.contains(point)) continue;
            if (tab.category == ToolbarCategory::INSPECT) {
                setActiveTool(TOOL_NONE);
            } else if (flyoutOpen && activeCategory == tab.category) {
                flyoutOpen = false; // second click closes it
            } else {
                activeCategory = tab.category;
                flyoutOpen = true;
            }
            return true;
        }

        // Build cards
        if (flyoutOpen) {
            auto list = toolsIn(activeCategory);
            for (size_t i = 0; i < list.size(); i++) {
                if (!cardBounds(i).contains(point)) continue;
                BuildTool picked = list[i]->tool;
                setActiveTool(currentTool == picked ? TOOL_NONE : picked);
                flyoutOpen = false;
                return true;
            }
            if (flyoutBounds().contains(point)) return true;
        }

        return sidebarBounds().contains(point);
    }

    // ---- drawing ----
    void draw(sf::RenderWindow& window, const sf::Font& font, const sf::Vector2f& mousePos) {
        (void)font;
        using namespace uikit;
        ToolbarCategory highlighted = flyoutOpen ? activeCategory
                                                 : (currentTool == TOOL_NONE ? ToolbarCategory::INSPECT : categoryOf(currentTool));

        // Sidebar
        panel(window, sidebarBounds(), 14.f);
        const CategoryTab* hoveredTab = nullptr;
        for (const auto& tab : tabs) {
            bool hovered = tab.bounds.contains(mousePos);
            bool active = (tab.category == highlighted);
            if (hovered) hoveredTab = &tab;
            sf::Color fill = active ? withAlpha(tab.accentColor, 70) : (hovered ? PANEL_HOVER : PANEL_RAISED);
            roundedRect(window, tab.bounds, 12.f, fill, active ? tab.accentColor : BORDER, active ? 2.f : 1.f);
            icon(window, tab.icon, tab.bounds.left + 8.f, tab.bounds.top + 8.f, 32.f);
        }

        // Flyout of build cards
        if (flyoutOpen) {
            const CategoryTab* tab = tabFor(activeCategory);
            sf::FloatRect fb = flyoutBounds();
            panel(window, fb, 12.f);
            if (tab) {
                roundedRect(window, sf::FloatRect(fb.left + 10.f, fb.top + 12.f, 4.f, 18.f), 2.f, tab->accentColor);
                textMid(window, tab->title, fb.left + 22.f, fb.top + 8.f, 26.f, 15, TEXT, true);
            }

            auto list = toolsIn(activeCategory);
            for (size_t i = 0; i < list.size(); i++) {
                const ToolItem& item = *list[i];
                sf::FloatRect cb = cardBounds(i);
                int cost = toolCost(item.tool);
                const bool locked = toolRank(item.tool) > unlockedRank;
                bool affordable = money >= cost && !locked;
                bool hovered = cb.contains(mousePos);
                bool active = (item.tool == currentTool);

                sf::Color fill = active ? withAlpha(item.accentColor, 55) : (hovered ? PANEL_HOVER : PANEL_RAISED);
                roundedRect(window, cb, 10.f, fill, active ? item.accentColor : BORDER, active ? 2.f : 1.f);

                // Thumbnail well
                sf::FloatRect well(cb.left + 6.f, cb.top + 6.f, 56.f, CARD_H - 12.f);
                roundedRect(window, well, 8.f, sf::Color(12, 16, 23, 230));
                std::string key = toolThumbnailKey(item.tool);
                sf::Color tint = affordable ? sf::Color::White : sf::Color(255, 255, 255, 110);
                if (key.rfind("ui_", 0) == 0) icon(window, key.substr(3), well.left + 12.f, well.top + 10.f, 32.f, tint);
                else thumbnail(window, key, sf::FloatRect(well.left + 3.f, well.top + 3.f, well.width - 6.f, well.height - 6.f), tint);

                float tx = cb.left + 72.f;
                float maxW = cb.left + cb.width - 8.f - tx - (item.hotkey.empty() ? 0.f : 44.f);
                text(window, fitText(titleCase(item.title), 13, maxW, true), tx, cb.top + 10.f, 13, affordable ? TEXT : TEXT_DIM, true);
                const CityRank& need = CITY_RANKS[std::min(CITY_RANK_COUNT - 1, toolRank(item.tool))];
                const std::string sub = locked ? std::string("Unlocks at ") + need.name + " (" + std::to_string(need.population) + " people)"
                                               : item.subtitle;
                text(window, fitText(sub, 11, cb.left + cb.width - 8.f - tx), tx, cb.top + 29.f, 11, locked ? WARN : TEXT_DIM);

                std::string price = locked ? "Locked" : (item.tool == TOOL_BULLDOZE) ? "$5+" : formatMoney(cost);
                text(window, price, tx, cb.top + 45.f, 12, affordable ? GOOD : BAD, true);

                if (!item.hotkey.empty()) {
                    float kw = textWidth(item.hotkey, 10, true) + 12.f;
                    sf::FloatRect kb(cb.left + cb.width - kw - 8.f, cb.top + 8.f, kw, 18.f);
                    roundedRect(window, kb, 5.f, sf::Color(10, 13, 19, 220), BORDER, 1.f);
                    textMid(window, item.hotkey, kb.left + 6.f, kb.top, kb.height, 10, TEXT_DIM, true);
                }
            }
        } else if (hoveredTab != nullptr) {
            // Tooltip with category name and hotkeys
            std::string label = hoveredTab->title + "  [" + hoveredTab->hotkeyHint + "]";
            float w = textWidth(label, 12, true) + 20.f;
            sf::FloatRect tb(SIDE_X + SIDE_W + 8.f, hoveredTab->bounds.top + 10.f, w, 28.f);
            panel(window, tb, 8.f, PANEL_RAISED);
            textMid(window, label, tb.left + 10.f, tb.top, tb.height, 12, TEXT, true);
        }

        // Active tool pill (bottom-left)
        if (currentTool != TOOL_NONE) {
            const ToolItem* item = nullptr;
            for (const auto& it : allTools) if (it.tool == currentTool) item = &it;
            sf::FloatRect pb = pillBounds();
            panel(window, pb, 12.f, PANEL, item ? item->accentColor : BORDER);

            std::string key = toolThumbnailKey(currentTool);
            sf::FloatRect well(pb.left + 6.f, pb.top + 6.f, 44.f, 36.f);
            roundedRect(window, well, 7.f, sf::Color(12, 16, 23, 230));
            if (key.rfind("ui_", 0) == 0) icon(window, key.substr(3), well.left + 6.f, well.top + 2.f, 32.f);
            else thumbnail(window, key, sf::FloatRect(well.left + 2.f, well.top + 2.f, well.width - 4.f, well.height - 4.f));

            std::string name = item ? titleCase(item->title) : "";
            text(window, pillNote.empty() ? std::string("Placing") : "Placing  -  " + pillNote, pb.left + 58.f, pb.top + 8.f, 10,
                 pillNote.empty() ? TEXT_DIM : sf::Color(120, 190, 245));
            text(window, name, pb.left + 58.f, pb.top + 22.f, 13, TEXT, true);
            std::string price = dragCostPreview >= 0 ? formatMoney(dragCostPreview)
                              : (currentTool == TOOL_BULLDOZE) ? "$5+" : formatMoney(toolCost(currentTool));
            textRight(window, price, pb.left + pb.width - 48.f, pb.top + 22.f, 12, GOOD, true);

            sf::FloatRect xb = pillCloseBounds();
            bool xHover = xb.contains(mousePos);
            roundedRect(window, xb, 8.f, xHover ? sf::Color(120, 40, 36) : PANEL_RAISED, BORDER, 1.f);
            textMid(window, "x", xb.left + 10.f, xb.top - 1.f, xb.height, 15, xHover ? sf::Color::White : TEXT_DIM, true);
        }
    }
};
