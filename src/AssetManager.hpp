#pragma once
#include "_global.hpp"

class AssetManager
{
public:
    void addTexture(std::string name, std::string path);
    void addFont(std::string path);
    void addSound(std::string path);
};

void AssetManager::addTexture(std::string name, std::string path)
{
    sf::Texture texture;

    if (!texture.loadFromFile("assets/sprites/" + path) &&
        !texture.loadFromFile("../assets/sprites/" + path) &&
        !texture.loadFromFile(path))
    {
        std::cout << "ASSET: Not found - " << path << " not found\n";
    }

    m_textures[name] = texture;
}

void AssetManager::addFont(std::string path)
{
    sf::Font font;

    if (!font.loadFromFile(path) &&
        !font.loadFromFile("assets/" + path) &&
        !font.loadFromFile("../assets/" + path))
    {
        std::cout << "FONT: Not found - " << path << " not found\n";
    }

    m_fonts[path] = font;
}

void AssetManager::addSound(std::string path)
{
    sf::SoundBuffer s;

    if (!s.loadFromFile("assets/sounds/" + path) &&
        !s.loadFromFile("../assets/sounds/" + path) &&
        !s.loadFromFile(path))
    {
        std::cout << "Sound: Not found - " << path << " not found\n";
    }

    m_sounds[path] = s;
}
