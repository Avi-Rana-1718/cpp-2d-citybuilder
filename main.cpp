#include <string>
#include <iostream>
#include <cmath>
#include <functional>
#include <cstdlib>
#include <cstdio>

#include "game/main.hpp"

int main(int argc, char** argv)
{
    bool forceShow = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--headless") headless = true;
        else if (arg == "--show") forceShow = true;
        else if (arg == "--bench-city") benchCity = true;
        else if (arg == "--bench-town") benchTown = true;
        else if (arg == "--starter") starterCity = true;
        else if (arg == "--budget") startBudget = true;
        else if (arg == "--messages") startLog = true;
    }

    // --seed N generates a reproducible world; omitted or 0 = random
    for (int i = 1; i + 1 < argc; i++) {
        if (std::string(argv[i]) == "--seed") {
            worldSeed = static_cast<uint32_t>(std::strtoul(argv[i + 1], nullptr, 10));
        } else if (std::string(argv[i]) == "--bench") {
            benchSeconds = static_cast<float>(std::atof(argv[i + 1]));
        } else if (std::string(argv[i]) == "--shot") {
            benchShot = argv[i + 1];
        } else if (std::string(argv[i]) == "--month") {
            // Start mid-way through month N (1-12), e.g. --month 1 for winter
            int m = std::atoi(argv[i + 1]);
            if (m >= 1 && m <= 12) gameClock.setDate(1, m, 15);
        } else if (std::string(argv[i]) == "--menu") {
            startMenu = std::atoi(argv[i + 1]);
        } else if (std::string(argv[i]) == "--view") {
            startView = std::atoi(argv[i + 1]);
        } else if (std::string(argv[i]) == "--inspect" && i + 2 < argc) {
            startInspectX = std::atoi(argv[i + 1]);
            startInspectY = std::atoi(argv[i + 2]);
        } else if (std::string(argv[i]) == "--save") {
            savePath = argv[i + 1];
        } else if (std::string(argv[i]) == "--load") {
            loadPath = argv[i + 1];
        } else if (std::string(argv[i]) == "--fast-forward") {
            fastForwardDays = std::atoi(argv[i + 1]);
        } else if (std::string(argv[i]) == "--size") {
            // --size 1920x1080: requested window size (still fitted to the desktop)
            int w = 0, h = 0;
            if (std::sscanf(argv[i + 1], "%dx%d", &w, &h) == 2 && w >= MIN_WINDOW_W && h >= MIN_WINDOW_H) {
                preferredW = w;
                preferredH = h;
            }
        } else if (std::string(argv[i]) == "--zoom") {
            startZoom = static_cast<float>(std::atof(argv[i + 1]));
        }
    }

    // Benchmarks run off-screen unless --show is given
    if (benchSeconds > 0.f && !forceShow) headless = true;
    if (forceShow) headless = false;

    Main game;
    game.run();

    return 0;
}
