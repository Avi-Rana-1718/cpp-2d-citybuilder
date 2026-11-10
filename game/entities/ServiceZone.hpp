#pragma once
#include "./_Zone.hpp"
#include "../../src/Entity.hpp"
#include "../world/WorldData.hpp"

// -------------------------------------------------------------
// Service buildings (2x2): police, fire, clinic, landfill, mine, bus depot
// -------------------------------------------------------------
// One class for six kinds (ServiceKind in EconomyData.hpp). Each kind has
// its own entity name so tileKind() / the inspect card can tell them apart.
// ServiceSystem reads them to build coverage maps, handle garbage, mines
// and buses. The mine is also an employer: low-wealth industry with
// SERVICE_MINE_JOBS jobs whose output depends on the deposit underneath.

inline const char* serviceEntityName(int kind) {
    static const char* names[SERVICE_KIND_COUNT] = {"PoliceZone", "FireZone", "ClinicZone",
                                                    "LandfillZone", "MineZone", "BusDepotZone",
                                                    "IncineratorZone", "RecyclingZone"};
    return (kind >= 0 && kind < SERVICE_KIND_COUNT) ? names[kind] : "PoliceZone";
}
inline const char* serviceTitle(int kind) {
    static const char* titles[SERVICE_KIND_COUNT] = {"Police Station", "Fire Station", "Clinic",
                                                     "Landfill", "Mine", "Bus Depot",
                                                     "Incinerator", "Recycling Centre"};
    return (kind >= 0 && kind < SERVICE_KIND_COUNT) ? titles[kind] : "Service";
}
inline const char* serviceTextureKey(int kind) {
    static const char* keys[SERVICE_KIND_COUNT] = {"svc_police", "svc_fire", "svc_clinic",
                                                   "svc_landfill", "svc_mine", "svc_bus",
                                                   "svc_incinerator", "svc_recycling"};
    return (kind >= 0 && kind < SERVICE_KIND_COUNT) ? keys[kind] : "svc_police";
}
inline int serviceKindOfName(const std::string& name) {
    for (int k = 0; k < SERVICE_KIND_COUNT; k++)
        if (name == serviceEntityName(k)) return k;
    return -1;
}

inline bool isGarbageFacility(int kind) {
    return kind == SERVICE_LANDFILL || kind == SERVICE_INCINERATOR || kind == SERVICE_RECYCLING;
}

class ServiceZone : public Zone {
public:
    int kind = SERVICE_POLICE;
    float stored = 0.f;       // landfill: garbage dumped so far
    float handled = 0.f;      // garbage taken today (landfill / incinerator / recycling)

    ServiceZone(int posX, int posY, int kind_);
    ~ServiceZone() = default;

    void update(float deltaTime) override { setDeltaTime(deltaTime); }
    int dailyUpkeep() const override { return SERVICE_UPKEEP[kind]; }
    int sector() const override { return kind == SERVICE_MINE ? SECTOR_I : -1; }
    int wealth() const override { return WEALTH_LOW; }
    // Mines sell ore: richer deposits pay more per miner
    float incomeMultiplier() const override { return kind == SERVICE_MINE ? 0.5f + 1.5f * depositRichness() : 1.f; }
    void drawGround(sf::RenderWindow& window) override;
    void drawBuilding(sf::RenderWindow& window) override;
    void checkValid() override { isActive = hasAdjacentRoad(posX, posY); }

    // Average deposit amount (0..1) under the footprint (mines)
    float depositRichness() const {
        float sum = 0.f;
        for (const auto& o : FOOTPRINT_2X2) {
            int tx = posX + o[0], ty = posY + o[1];
            if (inWorldBounds(tx, ty) && world[tx][ty].deposit != Deposit::None) sum += world[tx][ty].depositAmount;
        }
        return sum / 400.f;
    }
};

inline ServiceZone::ServiceZone(int posX, int posY, int kind_) {
    kind = kind_;
    setName(serviceEntityName(kind));
    isActive = false;
    isMultiTile = true;
    isSubTile = false;
    footprintW = 2;
    footprintH = 2;
    rootX = posX;
    rootY = posY;
    level = 0;
    capacity = kind == SERVICE_MINE ? SERVICE_MINE_JOBS : 0;
    population = 0;
    fixedTier = true;
    this->posX = posX;
    this->posY = posY;
    position->x = tileSizeX * posX - posX * (tileSizeX / 2);
    position->y = tileSizeY * posY + posX * (tileSizeY / 2);

    delete shape;
    shape = NULL;
    delete text;
    text = NULL;

    zoneBaseTexture = "grass";
    sprite = new sf::Sprite;
    sprite->setTexture(m_textures["grass"]);
    sprite->setScale(sf::Vector2f(2.f, 2.f));

    const std::string key = serviceTextureKey(kind);
    buildingSprite = new sf::Sprite;
    if (m_textures.count(key)) {
        const sf::Texture& tex = m_textures[key];
        buildingSprite->setTexture(tex);
        if (tex.getSize().y > 40) buildingSprite->setOrigin(0.f, static_cast<float>(tex.getSize().y - 40));
        buildingSprite->setScale(sf::Vector2f(2.f, 2.f));
    }
}

inline void ServiceZone::drawGround(sf::RenderWindow& window) {
    if (sprite != NULL) {
        sprite->setPosition(position->x, position->y);
        window.draw(*sprite);
    }
}

inline void ServiceZone::drawBuilding(sf::RenderWindow& window) {
    if (buildingSprite != NULL) {
        buildingSprite->setPosition(position->x - 16.f, position->y);
        buildingSprite->setColor(isActive ? sf::Color(255, 255, 255, 255) : sf::Color(200, 200, 200, 140));
        window.draw(*buildingSprite);
    }
}
