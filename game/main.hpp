#pragma once
#include "../src/GameEngine.hpp"

class Main : public GameEngine {
    const std::string gameTitle = "Chime";
    const float version = 1.0;

    public:
    Main() {
        std::cout<<"\nIn main";
    }

    std::string getGameTitle() {
        return gameTitle;
    }
    
    int getGameVersion(){
        return version;
    }
};