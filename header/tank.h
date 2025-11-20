#pragma once
#ifndef TANK_HEADER
#define TANK_HEADER
#include "tool.h"
#include <cmath>
#include <memory>

class Terrain;
class Missile;

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
    void update(float deltaTime, const Terrain* terrain = nullptr);

    sf::Vector2f getPos() const { return pos; }
    float getCanonAngle() const { return canon_angle; }
    
    // Physics
    void applyGravity(float deltaTime);
    bool checkTerrainCollision(const Terrain& terrain);
    void applyExplosionImpulse(float explosionX, float explosionY, float explosionForce, float explosionRadius);
    
    // Missile creation
    std::unique_ptr<Missile> createMissile();

private:
    sf::Vector2f pos;
    sf::Vector2i size;
    sf::Vector2i canon_size;
    float v;
    float angle;
    float canon_angle;
    
    // Physics
    sf::Vector2f velocity;
    bool onGround;
    
    sf::Texture corps_texture;
    sf::Sprite corps_sprite;
    sf::Texture canon_texture;
    sf::Sprite canon_sprite;

    int state;
    
    static constexpr float GRAVITY = 300.0f; // pixels/s^2
    static constexpr float MISSILE_SPEED = 400.0f; // pixels/s
};

#endif // !TANK_HEADER
