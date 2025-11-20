#pragma once
#ifndef TANK_HEADER
#define TANK_HEADER
#include "tool.h"
#include <cmath>

enum TankState 
{
    STOP = 0b1,
    MOVE_RIGHT = 0b10,
    MOVE_LEFT = 0b100,
    SHOOT = 0b1000,
};

class Tank
{
public:
    Tank(const std::string& corps_filename, const std::string& canon_filename);
    ~Tank();

    void draw(sf::RenderWindow& window);
    
    void setSize(sf::Vector2i size);
    void setPos(sf::Vector2f pos);
    void move(float m);
    void setRotation(float angle);
    void rotate(float angle);
    void setRotationCanon(float angle);
    void rotateCanon(float angle);
    void setVitesse(float vitesse);

    void targetPoint(sf::Vector2f point);

    void moveLeft();
    void moveLeftStop();
    void moveRight();
    void moveRightStop();
    void shoot();
    void update();

    sf::Vector2f getPos() const { return pos; }

private:
    sf::Vector2f pos;
    sf::Vector2i size;
    sf::Vector2i canon_size;
    float v;
    float angle;
    float canon_angle;
    
    sf::Texture corps_texture;
    sf::Sprite corps_sprite;
    sf::Texture canon_texture;
    sf::Sprite canon_sprite;

    int state;
};

#endif // !TANK_HEADER
