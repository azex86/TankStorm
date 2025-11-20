#pragma once
#ifndef MISSILE_HEADER
#define MISSILE_HEADER

#include <SFML/Graphics.hpp>
#include "terrain.h"

class Missile {
public:
    Missile(float x, float y, float angle, float speed);
    ~Missile();

    // Update missile physics
    void update(float deltaTime);
    
    // Draw the missile
    void draw(sf::RenderWindow& window);
    
    // Check collision with terrain
    bool checkTerrainCollision(const Terrain& terrain);
    
    // Check if missile is out of bounds
    bool isOutOfBounds(unsigned int worldWidth, unsigned int worldHeight) const;
    
    // Get explosion radius for terrain destruction
    float getExplosionRadius() const { return explosionRadius; }
    
    // Get position
    sf::Vector2f getPosition() const { return position; }
    
    // Check if missile is active
    bool isActive() const { return active; }
    void deactivate() { active = false; }

private:
    sf::Vector2f position;
    sf::Vector2f velocity;
    float angle;
    float explosionRadius;
    bool active;
    
    sf::CircleShape shape;
    
    static constexpr float GRAVITY = 300.0f; // pixels/s^2
};

#endif // MISSILE_HEADER
