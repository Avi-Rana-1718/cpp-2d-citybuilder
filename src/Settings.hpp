#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

// -------------------------------------------------------------
// Player settings, kept in settings.cfg (key=value lines) next to the game
// -------------------------------------------------------------
struct GameSettings {
    int sfxVolume = 70;        // 0-100
    int ambientVolume = 35;    // 0-100
    float cameraSpeed = 1.f;   // 0.5-2.0
    bool autosave = true;
    bool tips = true;
    bool dayNight = true;      // day / night lighting cycle
    bool minimap = true;
    int difficulty = 1;        // 0 easy, 1 normal, 2 hard (starting money)

    static int startingMoneyFor(int difficulty) {
        return difficulty == 0 ? 50000 : (difficulty == 2 ? 10000 : 20000);
    }

    void load(const std::string& path = "settings.cfg") {
        std::ifstream in(path);
        std::string line;
        while (std::getline(in, line)) {
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::istringstream value(line.substr(eq + 1));
            if (key == "sfx") value >> sfxVolume;
            else if (key == "ambient") value >> ambientVolume;
            else if (key == "camera") value >> cameraSpeed;
            else if (key == "autosave") value >> autosave;
            else if (key == "tips") value >> tips;
            else if (key == "difficulty") value >> difficulty;
            else if (key == "daynight") value >> dayNight;
            else if (key == "minimap") value >> minimap;
        }
        clamp();
    }

    void save(const std::string& path = "settings.cfg") const {
        std::ofstream out(path, std::ios::trunc);
        out << "sfx=" << sfxVolume << "\nambient=" << ambientVolume << "\ncamera=" << cameraSpeed
            << "\nautosave=" << autosave << "\ntips=" << tips << "\ndifficulty=" << difficulty
            << "\ndaynight=" << dayNight << "\nminimap=" << minimap << "\n";
    }

    void clamp() {
        sfxVolume = std::max(0, std::min(100, sfxVolume));
        ambientVolume = std::max(0, std::min(100, ambientVolume));
        cameraSpeed = std::max(0.5f, std::min(2.f, cameraSpeed));
        difficulty = std::max(0, std::min(2, difficulty));
    }
};

inline GameSettings settings;
