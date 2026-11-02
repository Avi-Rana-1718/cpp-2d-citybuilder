#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

enum BuildTool {
    TOOL_NONE,
    TOOL_RESIDENTIAL,
    TOOL_COMMERCIAL,
    TOOL_INDUSTRIAL,
    TOOL_FARM,
    TOOL_ROAD,
    TOOL_HARBOUR,
    TOOL_WATER,
    TOOL_SCHOOL,
    TOOL_CHURCH,
    TOOL_PARK,
    TOOL_BULLDOZE
};

enum class ToolbarCategory {
    INSPECT,
    ZONING,
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
    ToolbarCategory activeCategory = ToolbarCategory::ZONING;

    sf::FloatRect dockBounds;
    std::vector<CategoryTab> tabs;
    std::vector<ToolItem> allTools;

public:
    Toolbar() = default;

    void init(float windowW, float windowH) {
        float dockW = 860.f;
        float dockH = 96.f;
        dockBounds.left = (windowW - dockW) / 2.f;
        dockBounds.top = windowH - dockH - 12.f;
        dockBounds.width = dockW;
        dockBounds.height = dockH;

        // 1. Setup Category Tabs (Top Row of Dock)
        tabs.clear();
        float tabW = 190.f;
        float tabH = 28.f;
        float tabGap = 10.f;
        float tabStartX = dockBounds.left + (dockW - (4 * tabW + 3 * tabGap)) / 2.f;
        float tabY = dockBounds.top + 8.f;

        tabs.push_back({ToolbarCategory::INSPECT, "INSPECT [Esc]", sf::Color(52, 152, 219),
            sf::FloatRect(tabStartX + 0 * (tabW + tabGap), tabY, tabW, tabH)});
        tabs.push_back({ToolbarCategory::ZONING, "ZONING", sf::Color(46, 204, 113),
            sf::FloatRect(tabStartX + 1 * (tabW + tabGap), tabY, tabW, tabH)});
        tabs.push_back({ToolbarCategory::CIVICS, "CIVICS / SERVICES", sf::Color(230, 126, 34),
            sf::FloatRect(tabStartX + 2 * (tabW + tabGap), tabY, tabW, tabH)});
        tabs.push_back({ToolbarCategory::INFRASTRUCTURE, "INFRASTRUCTURE", sf::Color(149, 165, 166),
            sf::FloatRect(tabStartX + 3 * (tabW + tabGap), tabY, tabW, tabH)});

        // 2. Setup Tool Items for each Category (Bottom Row of Dock)
        allTools.clear();
        float btnY = dockBounds.top + 42.f;
        float btnH = 46.f;

        // Zoning Category (4 tools)
        float zBtnW = 195.f;
        float zGap = 10.f;
        float zStartX = dockBounds.left + (dockW - (4 * zBtnW + 3 * zGap)) / 2.f;

        allTools.push_back({TOOL_RESIDENTIAL, "RESIDENTIAL", "Zone [R]", sf::Color(46, 204, 113), ToolbarCategory::ZONING,
            sf::FloatRect(zStartX + 0 * (zBtnW + zGap), btnY, zBtnW, btnH)});
        allTools.push_back({TOOL_COMMERCIAL, "COMMERCIAL", "Zone [C]", sf::Color(241, 196, 15), ToolbarCategory::ZONING,
            sf::FloatRect(zStartX + 1 * (zBtnW + zGap), btnY, zBtnW, btnH)});
        allTools.push_back({TOOL_INDUSTRIAL, "INDUSTRIAL", "Zone [I]", sf::Color(230, 126, 34), ToolbarCategory::ZONING,
            sf::FloatRect(zStartX + 2 * (zBtnW + zGap), btnY, zBtnW, btnH)});
        allTools.push_back({TOOL_FARM, "FARMS", "Agri [F]", sf::Color(135, 211, 124), ToolbarCategory::ZONING,
            sf::FloatRect(zStartX + 3 * (zBtnW + zGap), btnY, zBtnW, btnH)});

        // Civics Category (3 tools)
        float cBtnW = 260.f;
        float cGap = 12.f;
        float cStartX = dockBounds.left + (dockW - (3 * cBtnW + 2 * cGap)) / 2.f;

        allTools.push_back({TOOL_SCHOOL, "ELEMENTARY SCHOOL", "Edu [1] (2x2, R:5)", sf::Color(231, 76, 60), ToolbarCategory::CIVICS,
            sf::FloatRect(cStartX + 0 * (cBtnW + cGap), btnY, cBtnW, btnH)});
        allTools.push_back({TOOL_CHURCH, "COMMUNITY CHURCH", "Faith [2] (2x2, R:4)", sf::Color(155, 89, 182), ToolbarCategory::CIVICS,
            sf::FloatRect(cStartX + 1 * (cBtnW + cGap), btnY, cBtnW, btnH)});
        allTools.push_back({TOOL_PARK, "NATURE PARK", "Health [3] (R:4)", sf::Color(39, 174, 96), ToolbarCategory::CIVICS,
            sf::FloatRect(cStartX + 2 * (cBtnW + cGap), btnY, cBtnW, btnH)});

        // Infrastructure Category (4 tools)
        float iBtnW = 195.f;
        float iGap = 10.f;
        float iStartX = dockBounds.left + (dockW - (4 * iBtnW + 3 * iGap)) / 2.f;

        allTools.push_back({TOOL_ROAD, "DIRT ROAD", "Build [Space]", sf::Color(189, 195, 199), ToolbarCategory::INFRASTRUCTURE,
            sf::FloatRect(iStartX + 0 * (iBtnW + iGap), btnY, iBtnW, btnH)});
        allTools.push_back({TOOL_HARBOUR, "TRADE HARBOUR", "Port [H] (+$120)", sf::Color(52, 152, 219), ToolbarCategory::INFRASTRUCTURE,
            sf::FloatRect(iStartX + 1 * (iBtnW + iGap), btnY, iBtnW, btnH)});
        allTools.push_back({TOOL_WATER, "WATER BODY", "Terrain [W]", sf::Color(41, 128, 185), ToolbarCategory::INFRASTRUCTURE,
            sf::FloatRect(iStartX + 2 * (iBtnW + iGap), btnY, iBtnW, btnH)});
        allTools.push_back({TOOL_BULLDOZE, "DEMOLISH", "Clear [X]", sf::Color(231, 76, 60), ToolbarCategory::INFRASTRUCTURE,
            sf::FloatRect(iStartX + 3 * (iBtnW + iGap), btnY, iBtnW, btnH)});
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
        } else if (tool == TOOL_RESIDENTIAL || tool == TOOL_COMMERCIAL || tool == TOOL_INDUSTRIAL || tool == TOOL_FARM) {
            activeCategory = ToolbarCategory::ZONING;
        } else if (tool == TOOL_SCHOOL || tool == TOOL_CHURCH || tool == TOOL_PARK) {
            activeCategory = ToolbarCategory::CIVICS;
        } else if (tool == TOOL_ROAD || tool == TOOL_HARBOUR || tool == TOOL_WATER || tool == TOOL_BULLDOZE) {
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
