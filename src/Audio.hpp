#pragma once
#include <SFML/Audio.hpp>
#include <memory>
#include <string>
#include <vector>
#include "_global.hpp"
#include "Settings.hpp"

// -------------------------------------------------------------
// Sound effects and ambience (assets/sounds, tools/gen_sounds.py)
// -------------------------------------------------------------
// Nothing plays until init() runs, so tests and headless benchmarks never
// open an audio device. Effects use a small pool of voices; the city
// ambience and the fire crackle are loops whose volume follows the
// settings (and the fire loop only plays while something burns).
class AudioSystem {
public:
    void init() {
        if (headless) return;
        for (const char* name : {"click", "build", "road", "bulldoze", "error", "alert", "milestone", "fire", "ambient"}) {
            sf::SoundBuffer b;
            const std::string file = std::string(name) + ".wav";
            if (b.loadFromFile("assets/sounds/" + file) || b.loadFromFile("../assets/sounds/" + file)) buffers[name] = b;
        }
        for (int i = 0; i < 12; i++) voices.push_back(std::make_unique<sf::Sound>());
        ambient = std::make_unique<sf::Sound>();
        fire = std::make_unique<sf::Sound>();
        if (buffers.count("ambient")) { ambient->setBuffer(buffers["ambient"]); ambient->setLoop(true); }
        if (buffers.count("fire")) { fire->setBuffer(buffers["fire"]); fire->setLoop(true); }
        ready = true;
    }

    void play(const std::string& name, float volumeScale = 1.f, float pitch = 1.f) {
        if (!ready || settings.sfxVolume <= 0 || !buffers.count(name)) return;
        for (auto& v : voices) {
            if (v->getStatus() == sf::Sound::Playing) continue;
            v->setBuffer(buffers[name]);
            v->setVolume(static_cast<float>(settings.sfxVolume) * volumeScale);
            v->setPitch(pitch);
            v->play();
            return;
        }
    }

    // Every frame: ambience on (quieter while paused), fire loop while burning
    void update(bool paused, bool burning) {
        if (!ready) return;
        const float amb = static_cast<float>(settings.ambientVolume) * (paused ? 0.4f : 1.f);
        if (amb > 0.f && ambient->getStatus() != sf::Sound::Playing) ambient->play();
        if (amb <= 0.f && ambient->getStatus() == sf::Sound::Playing) ambient->stop();
        ambient->setVolume(amb);
        const bool wantFire = burning && !paused && settings.sfxVolume > 0;
        if (wantFire && fire->getStatus() != sf::Sound::Playing) fire->play();
        if (!wantFire && fire->getStatus() == sf::Sound::Playing) fire->stop();
        fire->setVolume(static_cast<float>(settings.sfxVolume) * 0.5f);
    }

    bool isReady() const { return ready; }

private:
    bool ready = false;
    std::map<std::string, sf::SoundBuffer> buffers;
    std::vector<std::unique_ptr<sf::Sound>> voices;
    std::unique_ptr<sf::Sound> ambient, fire;
};

inline AudioSystem audio;
