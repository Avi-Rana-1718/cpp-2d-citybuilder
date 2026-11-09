#include <string>
#include <iostream>
#include <cmath>
#include <functional>
#include <cstdlib>

#include "game/main.hpp"

int main(int argc, char** argv)
{
    bool forceShow = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--headless") headless = true;
        else if (arg == "--show") forceShow = true;
        else if (arg == "--bench-city") benchCity = true;
    }

    // --seed N generates a reproducible world; omitted or 0 = random
    for (int i = 1; i + 1 < argc; i++) {
        if (std::string(argv[i]) == "--seed") {
            worldSeed = static_cast<uint32_t>(std::strtoul(argv[i + 1], nullptr, 10));
        } else if (std::string(argv[i]) == "--bench") {
            benchSeconds = static_cast<float>(std::atof(argv[i + 1]));
        } else if (std::string(argv[i]) == "--shot") {
            benchShot = argv[i + 1];
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
