#pragma once

class Entity {
    int _id;
    std::string entityName;
    
    float _deltaTime;

    //
    public:

    bool isActive;
    int level;
    int posX, posY;

    sf::Vector2f* position;

    sf::Sprite* sprite=NULL;
    sf::Shape* shape=NULL;
    sf::Text* text=NULL;

    Entity();
   virtual ~Entity();

    void setName(std::string entityName);
    void setDeltaTime(float deltaTime);

    std::string getName();
    int getID();
    float getDeltaTime();

    void draw();

    virtual void update(float deltaTime);

};

Entity::Entity() {
    _id=++entityCount;
    entityName="Unnamed";

    position = new sf::Vector2f();

    posX=-1;
    posY=-1;

    shape = new sf::RectangleShape(sf::Vector2f(tileSizeX, tileSizeY));
    shape->setOutlineColor(sf::Color::Red);
    shape->setOutlineThickness(4);
    shape->setFillColor(sf::Color(0, 0, 0, 0));
    
    text = new sf::Text;
    text->setFont(m_fonts["../assets/Hack.ttf"]);
    text->setCharacterSize(15);
    text->setFillColor(sf::Color::White);
    text->setString(std::to_string(tileX) + " " + std::to_string(tileY));
}

void Entity::setName(std::string entityName) {
    this->entityName = entityName;
}

void Entity::setDeltaTime(float deltaTime) {
    this->_deltaTime = deltaTime;
}

std::string Entity::getName() {
    return entityName;
}

int Entity::getID() {
    return _id;
}

float Entity::getDeltaTime() {
    return _deltaTime;
}

void Entity::draw() {
    if(shape!=NULL) {
        shape->setPosition(position->x, position->y);
        _window.draw(*shape);
    }
    if(sprite!=NULL) {
        sprite->setPosition(position->x, position->y);
        _window.draw(*sprite);
    }
    if(text!=NULL) {
        text->setPosition(position->x, position->y);
        _window.draw(*text);
    }
}

void Entity::update(float deltaTime) {
    draw();
}

Entity::~Entity() {
    std::cout<<getName()<<" deleted!\n";
}