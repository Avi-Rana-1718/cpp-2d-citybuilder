#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

enum BuildTool {
    TOOL_NONE,
    // Individual Residential Buildings
    TOOL_RES_COTTAGES,    // T1 Suburban Cottages ($80)
    TOOL_RES_APARTMENTS,  // T2 Brick Apartments ($200)
    TOOL_RES_TOWER,       // T3 Luxury Highrise ($500)

    // Individual Commercial Buildings
    TOOL_COM_GROCERY,     // T1 Corner Grocery ($80)
    TOOL_COM_PLAZA,       // T2 Shopping Plaza ($220)
    TOOL_COM_TOWER,       // T3 Banking Tower ($550)

    // Individual Industrial Buildings
    TOOL_IND_WORKSHOP,    // T1 Scrap Workshop ($70)
    TOOL_IND_FACTORY,     // T2 Manufacturing Plant ($220)
    TOOL_IND_TECH,        // T3 Microchip Clean-Fab ($500)

    // Agricultural Farms
    TOOL_FARM_BARN,       // Red Barn & Silo ($60)
    TOOL_FARM_WINDMILL,   // Windmill Homestead ($70)

    // Civics
    TOOL_SCHOOL,          // Elementary School ($150)
    TOOL_CHURCH,          // Community Church ($120)
    TOOL_PARK,            // Nature Park ($80)
    TOOL_HARBOUR,         // Trade Harbour ($250)

    // Infrastructure
    TOOL_ROAD,            // Dirt Road ($10)
    TOOL_WATER,           // Water Body ($10)
    TOOL_BULLDOZE,        // Demolish ($5)

    // Compatibility Aliases
    TOOL_RESIDENTIAL = TOOL_RES_COTTAGES,
    TOOL_COMMERCIAL  = TOOL_COM_GROCERY,
    TOOL_INDUSTRIAL  = TOOL_IND_WORKSHOP,
    TOOL_FARM        = TOOL_FARM_BARN
};

enum class ToolbarCategory {
    INSPECT,
    RESIDENTIAL,
    COMMERCIAL,
    INDUSTRIAL,
    CIVICS,
    INFRASTRUCTURE
};

struct ToolItem {
    BuildTool tool;
    std::string title;
    std::string shortcut;
    sf::Color accentColor;
    ToolbarCategory category;
    sf::FloatRect bounds;
};

struct CategoryTab {
    ToolbarCategory category;
    std::string title;
    sf::Color accentColor;
    sf::FloatRect bounds;
};

class Toolbar {
    BuildTool currentTool = TOOL_RESIDENTIAL;
    ToolbarCategory activeCategory = ToolbarCategory::RESIDENTIAL;

    sf::FloatRect dockBounds;
    std::vector<CategoryTab> tabs;
    std::vector<ToolItem> allTools;

public:
    Toolbar() = default;

    void init(float windowW, float windowH) {
        float dockW = 1040.f;
        float dockH = 96.f;
        dockBounds.left = (windowW - dockW) / 2.f;
        dockBounds.top = windowH - dockH - 12.f;
        dockBounds.width = dockW;
        dockBounds.height = dockH;

        // 1. Setup Category Tabs (Top Row of Dock - 6 Categories)
        tabs.clear();
        float tabW = 164.f;
        float tabH = 28.f;
        float tabGap = 8.f;
        float tabStartX = dockBounds.left + (dockW - (6 * tabW + 5 * tabGap)) / 2.f;
        float tabY = dockBounds.top + 8.f;

        tabs.push_back({ToolbarCategory::INSPECT, "INSPECT [Esc]", sf::Color(52, 152, 219),
            sf::FloatRect(tabStartX + 0 * (tabW + tabGap), tabY, tabW, tabH)});
        tabs.push_back({ToolbarCategory::RESIDENTIAL, "RESIDENTIAL", sf::Color(46, 204, 113),
            sf::FloatRect(tabStartX + 1 * (tabW + tabGap), tabY, tabW, tabH)});
        tabs.push_back({ToolbarCategory::COMMERCIAL, "COMMERCIAL", sf::Color(241, 196, 15),
            sf::FloatRect(tabStartX + 2 * (tabW + tabGap), tabY, tabW, tabH)});
        tabs.push_back({ToolbarCategory::INDUSTRIAL, "INDUSTRY / FARM", sf::Color(230, 126, 34),
            sf::FloatRect(tabStartX + 3 * (tabW + tabGap), tabY, tabW, tabH)});
        tabs.push_back({ToolbarCategory::CIVICS, "CIVICS / PORTS", sf::Color(155, 89, 182),
            sf::FloatRect(tabStartX + 4 * (tabW + tabGap), tabY, tabW, tabH)});
        tabs.push_back({ToolbarCategory::INFRASTRUCTURE, "INFRASTRUCTURE", sf::Color(149, 165, 166),
            sf::FloatRect(tabStartX + 5 * (tabW + tabGap), tabY, tabW, tabH)});

        // 2. Setup Individual Building Tools for each Category (Bottom Row)
        allTools.clear();
        float btnY = dockBounds.top + 42.f;
        float btnH = 46.f;

        // Residential Category (3 Individual Buildings)
        float rBtnW = 325.f;
        float rGap = 12.f;
        float rStartX = dockBounds.left + (dockW - (3 * rBtnW + 2 * rGap)) / 2.f;

        allTools.push_back({TOOL_RES_COTTAGES, "SUBURBAN COTTAGES", "[2x2] Tier 1 [R] ($80)", sf::Color(46, 204, 113), ToolbarCategory::RESIDENTIAL,
            sf::FloatRect(rStartX + 0 * (rBtnW + rGap), btnY, rBtnW, btnH)});
        allTools.push_back({TOOL_RES_APARTMENTS, "BRICK APARTMENTS", "[2x2] Tier 2 ($200)", sf::Color(39, 174, 96), ToolbarCategory::RESIDENTIAL,
            sf::FloatRect(rStartX + 1 * (rBtnW + rGap), btnY, rBtnW, btnH)});
        allTools.push_back({TOOL_RES_TOWER, "LUXURY HIGHRISE", "[2x2] Tier 3 ($500)", sf::Color(26, 188, 156), ToolbarCategory::RESIDENTIAL,
            sf::FloatRect(rStartX + 2 * (rBtnW + rGap), btnY, rBtnW, btnH)});

        // Commercial Category (3 Individual Buildings)
        float cBtnW = 325.f;
        float cGap = 12.f;
        float cStartX = dockBounds.left + (dockW - (3 * cBtnW + 2 * cGap)) / 2.f;

        allTools.push_back({TOOL_COM_GROCERY, "CORNER GROCERY", "[2x2] Tier 1 [C] ($80)", sf::Color(241, 196, 15), ToolbarCategory::COMMERCIAL,
            sf::FloatRect(cStartX + 0 * (cBtnW + cGap), btnY, cBtnW, btnH)});
        allTools.push_back({TOOL_COM_PLAZA, "SHOPPING PLAZA", "[2x2] Tier 2 ($220)", sf::Color(243, 156, 18), ToolbarCategory::COMMERCIAL,
            sf::FloatRect(cStartX + 1 * (cBtnW + cGap), btnY, cBtnW, btnH)});
        allTools.push_back({TOOL_COM_TOWER, "CORPORATE TOWER", "[2x2] Tier 3 ($550)", sf::Color(230, 126, 34), ToolbarCategory::COMMERCIAL,
            sf::FloatRect(cStartX + 2 * (cBtnW + cGap), btnY, cBtnW, btnH)});

        // Industrial & Farm Category (5 Individual Buildings)
        float ifBtnW = 195.f;
        float ifGap = 10.f;
        float ifStartX = dockBounds.left + (dockW - (5 * ifBtnW + 4 * ifGap)) / 2.f;

        allTools.push_back({TOOL_IND_WORKSHOP, "SCRAP WORKSHOP", "[2x2] Tier 1 [I] ($70)", sf::Color(211, 84, 0), ToolbarCategory::INDUSTRIAL,
            sf::FloatRect(ifStartX + 0 * (ifBtnW + ifGap), btnY, ifBtnW, btnH)});
        allTools.push_back({TOOL_IND_FACTORY, "FACTORY PLANT", "[2x2] Tier 2 ($220)", sf::Color(192, 57, 43), ToolbarCategory::INDUSTRIAL,
            sf::FloatRect(ifStartX + 1 * (ifBtnW + ifGap), btnY, ifBtnW, btnH)});
        allTools.push_back({TOOL_IND_TECH, "CLEAN-FAB HUB", "[2x2] Tier 3 ($500)", sf::Color(52, 152, 219), ToolbarCategory::INDUSTRIAL,
            sf::FloatRect(ifStartX + 2 * (ifBtnW + ifGap), btnY, ifBtnW, btnH)});
        allTools.push_back({TOOL_FARM_BARN, "FARMS", "[2x2] Barn & Silo [F] ($60)", sf::Color(135, 211, 124), ToolbarCategory::INDUSTRIAL,
            sf::FloatRect(ifStartX + 3 * (ifBtnW + ifGap), btnY, ifBtnW, btnH)});
        allTools.push_back({TOOL_FARM_WINDMILL, "WINDMILL FARM", "[2x2] Windmill ($70)", sf::Color(39, 174, 96), ToolbarCategory::INDUSTRIAL,
            sf::FloatRect(ifStartX + 4 * (ifBtnW + ifGap), btnY, ifBtnW, btnH)});

        // Civics Category (4 Individual Buildings)
        float civBtnW = 245.f;
        float civGap = 10.f;
        float civStartX = dockBounds.left + (dockW - (4 * civBtnW + 3 * civGap)) / 2.f;

        allTools.push_back({TOOL_SCHOOL, "ELEMENTARY SCHOOL", "[2x2] Edu [1] ($150)", sf::Color(231, 76, 60), ToolbarCategory::CIVICS,
            sf::FloatRect(civStartX + 0 * (civBtnW + civGap), btnY, civBtnW, btnH)});
        allTools.push_back({TOOL_CHURCH, "COMMUNITY CHURCH", "[2x2] Faith [2] ($120)", sf::Color(155, 89, 182), ToolbarCategory::CIVICS,
            sf::FloatRect(civStartX + 1 * (civBtnW + civGap), btnY, civBtnW, btnH)});
        allTools.push_back({TOOL_PARK, "NATURE PARK", "[2x2] Health [3] ($80)", sf::Color(46, 204, 113), ToolbarCategory::CIVICS,
            sf::FloatRect(civStartX + 2 * (civBtnW + civGap), btnY, civBtnW, btnH)});
        allTools.push_back({TOOL_HARBOUR, "TRADE HARBOUR", "[2x2] Port [H] ($250)", sf::Color(52, 152, 219), ToolbarCategory::CIVICS,
            sf::FloatRect(civStartX + 3 * (civBtnW + civGap), btnY, civBtnW, btnH)});

        // Infrastructure Category (3 Tools)
        float infBtnW = 325.f;
        float infGap = 12.f;
        float infStartX = dockBounds.left + (dockW - (3 * infBtnW + 2 * infGap)) / 2.f;

        allTools.push_back({TOOL_ROAD, "DIRT ROAD", "[1x1] Path [Space] ($10)", sf::Color(189, 195, 199), ToolbarCategory::INFRASTRUCTURE,
            sf::FloatRect(infStartX + 0 * (infBtnW + infGap), btnY, infBtnW, btnH)});
        allTools.push_back({TOOL_WATER, "WATER BODY", "[1x1] Water [W] ($10)", sf::Color(41, 128, 185), ToolbarCategory::INFRASTRUCTURE,
            sf::FloatRect(infStartX + 1 * (infBtnW + infGap), btnY, infBtnW, btnH)});
        allTools.push_back({TOOL_BULLDOZE, "DEMOLISH", "Clear [X] ($5)", sf::Color(231, 76, 60), ToolbarCategory::INFRASTRUCTURE,
            sf::FloatRect(infStartX + 2 * (infBtnW + infGap), btnY, infBtnW, btnH)});
    }

    BuildTool getActiveTool() const {
        return currentTool;
    }

    std::string getActiveToolName() const {
        for (const auto& item : allTools) {
            if (item.tool == currentTool) return item.title;
        }
        return "INSPECT";
    }

    void setActiveTool(BuildTool tool) {
        currentTool = tool;

        // Automatically sync active category tab
        if (tool == TOOL_NONE) {
            activeCategory = ToolbarCategory::INSPECT;
        } else if (tool == TOOL_RES_COTTAGES || tool == TOOL_RES_APARTMENTS || tool == TOOL_RES_TOWER) {
            activeCategory = ToolbarCategory::RESIDENTIAL;
        } else if (tool == TOOL_COM_GROCERY || tool == TOOL_COM_PLAZA || tool == TOOL_COM_TOWER) {
            activeCategory = ToolbarCategory::COMMERCIAL;
        } else if (tool == TOOL_IND_WORKSHOP || tool == TOOL_IND_FACTORY || tool == TOOL_IND_TECH ||
                   tool == TOOL_FARM_BARN || tool == TOOL_FARM_WINDMILL) {
            activeCategory = ToolbarCategory::INDUSTRIAL;
        } else if (tool == TOOL_SCHOOL || tool == TOOL_CHURCH || tool == TOOL_PARK || tool == TOOL_HARBOUR) {
            activeCategory = ToolbarCategory::CIVICS;
        } else if (tool == TOOL_ROAD || tool == TOOL_WATER || tool == TOOL_BULLDOZE) {
            activeCategory = ToolbarCategory::INFRASTRUCTURE;
        }
    }

    bool contains(const sf::Vector2f& point) const {
        return dockBounds.contains(point);
    }

    bool handleClick(const sf::Vector2f& point) {
        if (!dockBounds.contains(point)) return false;

        // 1. Check Category Tabs
        for (const auto& tab : tabs) {
            if (tab.bounds.contains(point)) {
                activeCategory = tab.category;
                if (tab.category == ToolbarCategory::INSPECT) {
                    currentTool = TOOL_NONE;
                }
                return true;
            }
        }

        // 2. Check Active Category Tool Buttons
        for (const auto& toolItem : allTools) {
            if (toolItem.category == activeCategory && toolItem.bounds.contains(point)) {
                if (currentTool == toolItem.tool) {
                    currentTool = TOOL_NONE;
                    activeCategory = ToolbarCategory::INSPECT;
                } else {
                    currentTool = toolItem.tool;
                }
                return true;
            }
        }

        return true;
    }

    void draw(sf::RenderWindow& window, const sf::Font& font, const sf::Vector2f& mousePos) {
        // Shelf dock frame
        sf::RectangleShape dockShape(sf::Vector2f(dockBounds.width, dockBounds.height));
        dockShape.setPosition(dockBounds.left, dockBounds.top);
        dockShape.setFillColor(sf::Color(16, 20, 26, 245));
        dockShape.setOutlineColor(sf::Color(55, 68, 85, 230));
        dockShape.setOutlineThickness(2.f);
        window.draw(dockShape);

        sf::Text titleText;
        titleText.setFont(font);

        sf::Text subText;
        subText.setFont(font);

        // Render Category Tabs (Top Row)
        for (const auto& tab : tabs) {
            bool isHovered = tab.bounds.contains(mousePos);
            bool isTabActive = (activeCategory == tab.category);

            sf::RectangleShape tabShape(sf::Vector2f(tab.bounds.width, tab.bounds.height));
            tabShape.setPosition(tab.bounds.left, tab.bounds.top);

            if (isTabActive) {
                tabShape.setFillColor(sf::Color(tab.accentColor.r / 3, tab.accentColor.g / 3, tab.accentColor.b / 3, 255));
                tabShape.setOutlineColor(tab.accentColor);
                tabShape.setOutlineThickness(2.f);
            } else if (isHovered) {
                tabShape.setFillColor(sf::Color(35, 45, 58, 230));
                tabShape.setOutlineColor(sf::Color(tab.accentColor.r, tab.accentColor.g, tab.accentColor.b, 180));
                tabShape.setOutlineThickness(1.f);
            } else {
                tabShape.setFillColor(sf::Color(22, 28, 36, 200));
                tabShape.setOutlineColor(sf::Color(45, 55, 70, 160));
                tabShape.setOutlineThickness(1.f);
            }
            window.draw(tabShape);

            titleText.setCharacterSize(11);
            titleText.setStyle(isTabActive ? sf::Text::Bold : sf::Text::Regular);
            titleText.setString(tab.title);
            titleText.setFillColor(isTabActive ? sf::Color::White : (isHovered ? sf::Color(230, 235, 240) : sf::Color(160, 170, 185)));

            sf::FloatRect tb = titleText.getLocalBounds();
            titleText.setPosition(
                std::round(tab.bounds.left + (tab.bounds.width - tb.width) / 2.f - tb.left),
                std::round(tab.bounds.top + (tab.bounds.height - tb.height) / 2.f - tb.top - 2.f)
            );
            window.draw(titleText);
        }

        // Render Tool Buttons for the active category (Bottom Row)
        if (activeCategory == ToolbarCategory::INSPECT) {
            sf::Text inspectHint;
            inspectHint.setFont(font);
            inspectHint.setCharacterSize(13);
            inspectHint.setFillColor(sf::Color(180, 210, 240));
            inspectHint.setString("Inspect Mode: Hover over tiles to view citizen happiness & service coverage.");
            sf::FloatRect ib = inspectHint.getLocalBounds();
            inspectHint.setPosition(
                std::round(dockBounds.left + (dockBounds.width - ib.width) / 2.f - ib.left),
                std::round(dockBounds.top + 55.f)
            );
            window.draw(inspectHint);
            return;
        }

        for (const auto& item : allTools) {
            if (item.category != activeCategory) continue;

            bool isHovered = item.bounds.contains(mousePos);
            bool isActive = (currentTool == item.tool);

            sf::RectangleShape btnShape(sf::Vector2f(item.bounds.width, item.bounds.height));
            btnShape.setPosition(item.bounds.left, item.bounds.top);

            if (isActive) {
                btnShape.setFillColor(sf::Color(item.accentColor.r / 4, item.accentColor.g / 4, item.accentColor.b / 4, 255));
                btnShape.setOutlineColor(item.accentColor);
                btnShape.setOutlineThickness(2.5f);
            } else if (isHovered) {
                btnShape.setFillColor(sf::Color(38, 48, 62, 240));
                btnShape.setOutlineColor(sf::Color(item.accentColor.r, item.accentColor.g, item.accentColor.b, 200));
                btnShape.setOutlineThickness(1.5f);
            } else {
                btnShape.setFillColor(sf::Color(25, 31, 40, 215));
                btnShape.setOutlineColor(sf::Color(50, 60, 75, 180));
                btnShape.setOutlineThickness(1.f);
            }
            window.draw(btnShape);

            // Accent strip at top
            sf::RectangleShape strip(sf::Vector2f(item.bounds.width - 6.f, 3.f));
            strip.setPosition(item.bounds.left + 3.f, item.bounds.top + 2.f);
            strip.setFillColor(isActive ? item.accentColor : sf::Color(item.accentColor.r, item.accentColor.g, item.accentColor.b, isHovered ? 200 : 120));
            window.draw(strip);

            // Button Title
            titleText.setCharacterSize(12);
            titleText.setStyle(sf::Text::Bold);
            titleText.setString(item.title);
            titleText.setFillColor(isActive ? sf::Color::White : (isHovered ? sf::Color(245, 245, 250) : sf::Color(190, 200, 210)));

            sf::FloatRect tb = titleText.getLocalBounds();
            titleText.setPosition(
                std::round(item.bounds.left + (item.bounds.width - tb.width) / 2.f - tb.left),
                std::round(item.bounds.top + 10.f)
            );
            window.draw(titleText);

            // Subtext / shortcut
            subText.setCharacterSize(10);
            subText.setStyle(sf::Text::Regular);
            subText.setString(item.shortcut);
            subText.setFillColor(isActive ? item.accentColor : sf::Color(135, 150, 170));

            sf::FloatRect sb = subText.getLocalBounds();
            subText.setPosition(
                std::round(item.bounds.left + (item.bounds.width - sb.width) / 2.f - sb.left),
                std::round(item.bounds.top + 27.f)
            );
            window.draw(subText);
        }
    }
};
