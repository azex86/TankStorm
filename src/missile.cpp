#include "../header/missile.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Missile::Missile(float x, float y, float angle, float speed)
    : position(x, y), angle(angle), explosionRadius(30.0f), active(true)
{
    // Convert angle to radians and calculate velocity
    float angleRad = angle * M_PI / 180.0f;
    velocity.x = std::cos(angleRad) * speed;
    velocity.y = std::sin(angleRad) * speed;
    
    // Visual representation
    shape.setRadius(3.0f);
    shape.setFillColor(sf::Color::Red);
    shape.setOrigin(3.0f, 3.0f);
    shape.setPosition(position);
}

Missile::~Missile()
{
}

void Missile::update(float deltaTime)
{
    if (!active) return;
    
    // Apply gravity
    velocity.y += GRAVITY * deltaTime;
    
    // Update position
    position += velocity * deltaTime;
    
    // Update visual
    shape.setPosition(position);
}

void Missile::draw(sf::RenderWindow& window)
{
    if (active) {
        window.draw(shape);
    }
}

bool Missile::checkTerrainCollision(const Terrain& terrain)
{
    if (!active) return false;
    
    // Check if missile center collides with terrain
    return terrain.isColliding(
        static_cast<int>(position.x), 
        static_cast<int>(position.y)
    );
}

bool Missile::isOutOfBounds(unsigned int worldWidth, unsigned int worldHeight) const
{
    return position.x < 0 || position.x > worldWidth ||
           position.y < 0 || position.y > worldHeight;
}
