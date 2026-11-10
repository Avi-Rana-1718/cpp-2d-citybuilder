#pragma once
#include <vector>
#include <random>
#include <cmath>
#include <SFML/Graphics.hpp>
#include "./RoadNetwork.hpp"
#include "../simulation/TrafficSystem.hpp"

// -------------------------------------------------------------
// Cars: visual traffic driven by the commute model
// -------------------------------------------------------------
// Purely cosmetic. The number of cars follows the day's trips; they
// spawn on busy tiles and, at each tile centre, turn towards busier
// neighbours, so they gather where TrafficSystem routes commuters.
// Congested tiles slow them down. Cars live only on road tiles and
// disappear when the road under them changes. Each bus depot keeps
// BUSES_PER_DEPOT buses (model BUS) driving out from it.

class CarRenderer {
public:
    static const int MAX_CARS = 300;
    static const int BUS = 5;   // model index of a bus

    struct Car {
        int x = 0, y = 0;  // tile the car is leaving
        int dir = 0;       // ROAD_DIR heading
        float t = 0.f;     // 0 = tile centre, 1 = next tile centre
        int model = 0;
        int life = 0;      // tiles left before it parks (despawns)
    };

    std::vector<Car> cars;

    void clear() { cars.clear(); spawnCredit = 0.f; busCredit = 0.f; }

    // Cars wanted for today's traffic: a few per commuter, at most half the road tiles
    static int targetCount(int trips, int roadTiles) {
        return std::max(0, std::min({MAX_CARS, trips / 3, roadTiles / 2}));
    }

    // gameSeconds: game time passed this frame (0 while paused)
    void update(float gameSeconds) {
        if (gameSeconds <= 0.f) return;
        gameSeconds = std::min(gameSeconds, 0.25f);

        // Move, turn at tile centres, drop cars whose road vanished
        for (size_t c = 0; c < cars.size();) {
            Car& car = cars[c];
            RoadTile* here = RoadTile::at(car.x, car.y);
            int nx = car.x + ROAD_DIR[car.dir][0], ny = car.y + ROAD_DIR[car.dir][1];
            RoadTile* next = RoadTile::at(nx, ny);
            if (here == NULL || next == NULL || !(here->mask & (1 << car.dir))) {
                cars[c] = cars.back();
                cars.pop_back();
                continue;
            }
            float cong = std::max(1.f, traffic.congestion[car.x][car.y]);
            car.t += gameSeconds * 1.2f * ROAD_SPEED[here->tier] / cong;
            bool parked = false;
            while (car.t >= 1.f && !parked) {
                car.t -= 1.f;
                car.x = nx;
                car.y = ny;
                if (--car.life <= 0) { parked = true; break; }
                int d = chooseTurn(car.x, car.y, car.dir);
                if (d < 0) { parked = true; break; }
                car.dir = d;
                nx = car.x + ROAD_DIR[d][0];
                ny = car.y + ROAD_DIR[d][1];
            }
            if (parked) {
                cars[c] = cars.back();
                cars.pop_back();
                continue;
            }
            c++;
        }

        // Spawn towards today's target, a few per second
        int roadTiles = 0;
        float totalLoad = 0.f;
        for (int i = 0; i < tileMapW; i++)
            for (int j = 0; j < tileMapH; j++)
                if (roadNet.comp[i][j] >= 0) { roadTiles++; totalLoad += traffic.load[i][j]; }
        int target = targetCount(traffic.totalTrips, roadTiles);
        int buses = 0;
        for (const Car& c : cars) buses += c.model == BUS ? 1 : 0;
        spawnCredit += gameSeconds * 20.f;
        while (static_cast<int>(cars.size()) - buses < target && spawnCredit >= 1.f && totalLoad > 0.f) {
            spawnCredit -= 1.f;
            spawnOne(totalLoad);
        }
        if (spawnCredit > 5.f) spawnCredit = 5.f;

        // Buses leave from their depots
        busCredit += gameSeconds * 0.5f;
        if (busCredit >= 1.f) {
            busCredit = 0.f;
            std::vector<sf::Vector2i> depotRoads;
            for (int i = 0; i < tileMapW; i++)
                for (int j = 0; j < tileMapH; j++) {
                    ServiceZone* s = dynamic_cast<ServiceZone*>(entities[i][j]);
                    if (s == NULL || s->isSubTile || s->kind != SERVICE_BUS_DEPOT || !s->isActive) continue;
                    for (const auto& o : BORDER_2X2) {
                        RoadTile* r = RoadTile::at(i + o[0], j + o[1]);
                        if (r != NULL && r->mask != 0) { depotRoads.push_back({i + o[0], j + o[1]}); break; }
                    }
                }
            if (buses < static_cast<int>(depotRoads.size()) * BUSES_PER_DEPOT && !depotRoads.empty()) {
                sf::Vector2i at = depotRoads[static_cast<size_t>(rand01() * static_cast<float>(depotRoads.size())) % depotRoads.size()];
                Car bus;
                bus.x = at.x;
                bus.y = at.y;
                bus.dir = chooseTurn(at.x, at.y, (static_cast<int>(rand01() * 4.f) + 2) % 4);
                if (bus.dir >= 0) {
                    bus.model = BUS;
                    bus.life = 30 + static_cast<int>(rand01() * 20.f);
                    cars.push_back(bus);
                }
            }
        }
    }

    // Car at the heading's sprite, offset into its lane (drive on the right)
    void drawCar(sf::RenderTarget& target, const Car& car, sf::Sprite& sprite) const {
        static const char* MODELS[6] = {"car", "car_sedan", "car_coupe", "car_taxi", "car_van", "bus"};
        static const char* HEAD[4] = {"ne", "se", "sw", "nw"};
        sf::Vector2f a = centre(car.x, car.y);
        sf::Vector2f b = centre(car.x + ROAD_DIR[car.dir][0], car.y + ROAD_DIR[car.dir][1]);
        sf::Vector2f v = b - a;
        float len = std::sqrt(v.x * v.x + v.y * v.y);
        sf::Vector2f right(-v.y / len, v.x / len);
        RoadTile* r = RoadTile::at(car.x, car.y);
        float lane = (r != NULL && r->tier == 2) ? 9.f : 5.f;
        sf::Vector2f p = a + v * car.t + right * lane;

        const sf::Texture& tex = m_textures[std::string(MODELS[car.model]) + "_" + HEAD[car.dir]];
        sprite.setTexture(tex, true);
        sf::Vector2u ts = tex.getSize();
        sprite.setOrigin(static_cast<float>(ts.x) / 2.f, static_cast<float>(ts.y) * 0.7f);
        sprite.setPosition(std::round(p.x), std::round(p.y));
        target.draw(sprite);
    }

    // Tile a car is drawn with (its depth slot in the building pass)
    static sf::Vector2i drawTile(const Car& car) {
        if (car.t < 0.5f) return {car.x, car.y};
        return {car.x + ROAD_DIR[car.dir][0], car.y + ROAD_DIR[car.dir][1]};
    }

private:
    std::mt19937 rng{20240611u};
    float spawnCredit = 0.f;
    float busCredit = 0.f;

    static sf::Vector2f centre(int x, int y) {
        sf::Vector2f p = tileScreenPos(x, y);
        return sf::Vector2f(p.x + 32.f, p.y + 16.f);
    }

    float rand01() { return std::uniform_real_distribution<float>(0.f, 1.f)(rng); }

    // Next heading at a tile centre: any linked edge except a U-turn
    // (unless it is a dead end), weighted by the load of the tile it leads to
    int chooseTurn(int x, int y, int cameFrom) {
        RoadTile* r = RoadTile::at(x, y);
        if (r == NULL) return -1;
        float w[4] = {0.f, 0.f, 0.f, 0.f}, sum = 0.f;
        int back = (cameFrom + 2) % 4;
        for (int d = 0; d < 4; d++) {
            if (!(r->mask & (1 << d)) || d == back) continue;
            w[d] = 1.f + traffic.load[x + ROAD_DIR[d][0]][y + ROAD_DIR[d][1]];
            sum += w[d];
        }
        if (sum <= 0.f) return (r->mask & (1 << back)) ? back : -1;
        float pick = rand01() * sum;
        for (int d = 0; d < 4; d++) {
            if (w[d] <= 0.f) continue;
            if (pick < w[d]) return d;
            pick -= w[d];
        }
        return back;
    }

    void spawnOne(float totalLoad) {
        float pick = rand01() * totalLoad;
        for (int i = 0; i < tileMapW; i++) {
            for (int j = 0; j < tileMapH; j++) {
                if (roadNet.comp[i][j] < 0 || traffic.load[i][j] <= 0.f) continue;
                pick -= traffic.load[i][j];
                if (pick > 0.f) continue;
                RoadTile* r = RoadTile::at(i, j);
                if (r == NULL || r->mask == 0) return;
                Car car;
                car.x = i;
                car.y = j;
                car.dir = chooseTurn(i, j, (static_cast<int>(rand01() * 4.f) + 2) % 4);
                if (car.dir < 0) return;
                car.t = rand01() * 0.5f;
                car.model = static_cast<int>(rand01() * 5.f) % 5;
                car.life = 6 + static_cast<int>(rand01() * 20.f);
                cars.push_back(car);
                return;
            }
        }
    }
};
