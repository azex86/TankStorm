#include "../header/tank.h"
#include "../header/terrain.h"
#include "../header/missile.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Tank::Tank(const std::string& corps_filename, const std::string& canon_filename)
    : velocity(0.0f, 0.0f), onGround(false)
{
    if (!corps_texture.loadFromFile(corps_filename)) {
        std::cerr << "Error during load of corps texture" << std::endl;
    }
    corps_sprite.setTexture(corps_texture);

    if (!canon_texture.loadFromFile(canon_filename)) {
        std::cerr << "Error during load of canon texture" << std::endl;
    }
    canon_sprite.setTexture(canon_texture);

    size = sf::Vector2i(corps_texture.getSize());
    canon_size = sf::Vector2i(canon_texture.getSize());
    
    // Set origins to center for rotation
    corps_sprite.setOrigin(size.x / 2.0f, size.y / 2.0f);
    canon_sprite.setOrigin(0, canon_size.y / 2.0f);

    pos = {100, 100};
    angle = 0;
    canon_angle = 0;
    v = 150.0f; // Increased speed for better gameplay
    state = STOP;

    std::cout << "Tank initialized : pos = " << pos.x << ";" << pos.y << " size = " << size.x << ";" << size.y << std::endl;
}

Tank::~Tank()
{
}

void Tank::draw(sf::RenderWindow& window)
{
    corps_sprite.setPosition(pos.x + size.x / 2.0f, pos.y + size.y / 2.0f);
    corps_sprite.setRotation(angle);
    window.draw(corps_sprite);

    canon_sprite.setPosition(pos.x + size.x / 2.0f, pos.y + size.y / 2.0f);
    canon_sprite.setRotation(canon_angle - 90);
    window.draw(canon_sprite);
}

void Tank::setSize(sf::Vector2i new_size)
{
    size = new_size;
}

void Tank::setPos(sf::Vector2f new_pos)
{
    pos = new_pos;
}

void Tank::move(float m)
{
    float angle_rad = angle * M_PI / 180.0f;
    float x = m * std::cos(angle_rad);
    float y = m * std::sin(angle_rad);
    pos.x += x;
    pos.y += y;
}

void Tank::setRotation(float new_angle)
{
    angle = new_angle;
}

void Tank::rotate(float da)
{
    angle += da;
}

void Tank::setRotationCanon(float new_angle)
{
    canon_angle = new_angle;
}

void Tank::rotateCanon(float da)
{
    canon_angle += da;
}

void Tank::setVitesse(float vitesse)
{
    v = vitesse;
}

void Tank::targetPoint(sf::Vector2f point)
{
    float dx = point.x - (pos.x + size.x / 2.0f);
    float dy = point.y - (pos.y + size.y / 2.0f);
    float angle_rad = std::atan2(dy, dx);
    canon_angle = angle_rad * 180.0f / M_PI;
}

void Tank::moveLeft()
{
    state |= MOVE_LEFT;
}

void Tank::moveLeftStop()
{
    state &= ~MOVE_LEFT;
}

void Tank::moveRight()
{
    state |= MOVE_RIGHT;
}

void Tank::moveRightStop()
{
    state &= ~MOVE_RIGHT;
}

void Tank::shoot()
{
    state |= SHOOT;
}

void Tank::applyGravity(float deltaTime)
{
    if (!onGround) {
        velocity.y += GRAVITY * deltaTime;
    }
}

bool Tank::checkTerrainCollision(const Terrain& terrain)
{
    // Check bottom of tank for ground collision
    int checkPoints = 5;
    for (int i = 0; i < checkPoints; i++) {
        float xOffset = (i / (float)(checkPoints - 1)) * size.x;
        int checkX = static_cast<int>(pos.x + xOffset);
        int checkY = static_cast<int>(pos.y + size.y + 1);
        
        if (terrain.isColliding(checkX, checkY)) {
            return true;
        }
    }
    return false;
}

void Tank::applyExplosionImpulse(float explosionX, float explosionY, float explosionForce, float explosionRadius)
{
    // Calculate tank center
    float tankCenterX = pos.x + size.x / 2.0f;
    float tankCenterY = pos.y + size.y / 2.0f;
    
    // Calculate distance from explosion
    float dx = tankCenterX - explosionX;
    float dy = tankCenterY - explosionY;
    float distance = std::sqrt(dx * dx + dy * dy);
    
    std::cout << "Explosion check - Tank: (" << tankCenterX << "," << tankCenterY 
              << ") Explosion: (" << explosionX << "," << explosionY 
              << ") Distance: " << distance << " Radius: " << explosionRadius << std::endl;
    
    // Only apply impulse if within explosion radius
    if (distance < explosionRadius && distance > 0.1f) {
        // Calculate impulse magnitude (inverse square law with minimum distance)
        float normalizedDistance = distance / explosionRadius;
        float impulseMagnitude = explosionForce * (1.0f - normalizedDistance);
        
        // Calculate impulse direction (away from explosion)
        float dirX = dx / distance;
        float dirY = dy / distance;
        
        // Apply impulse to velocity
        velocity.x += dirX * impulseMagnitude;
        velocity.y += dirY * impulseMagnitude;
        
        // Tank is now airborne
        onGround = false;
        
        std::cout << ">>> Tank hit by explosion! Impulse: " << impulseMagnitude 
                  << " Velocity: (" << velocity.x << "," << velocity.y << ")" << std::endl;
    } else {
        std::cout << ">>> Tank NOT affected (too far)" << std::endl;
    }
}


std::unique_ptr<Missile> Tank::createMissile()
{
    // Create missile at cannon tip
    float cannonLength = canon_size.x;
    float angleRad = canon_angle * M_PI / 180.0f;
    
    float missileX = pos.x + size.x / 2.0f + std::cos(angleRad) * cannonLength;
    float missileY = pos.y + size.y / 2.0f + std::sin(angleRad) * cannonLength;
    
    return std::make_unique<Missile>(missileX, missileY, canon_angle, MISSILE_SPEED);
}

void Tank::update(float deltaTime, const Terrain* terrain)
{
    // Handle movement
    if (state & MOVE_LEFT)
    {
        move(-v * deltaTime);
    }
    if (state & MOVE_RIGHT)
    {
        move(v * deltaTime);
    }
    
    // Apply physics
    if (terrain) {
        applyGravity(deltaTime);
        
        // Apply velocity (both horizontal and vertical)
        pos.x += velocity.x * deltaTime;
        pos.y += velocity.y * deltaTime;
        
        // Apply friction to horizontal velocity
        velocity.x *= 0.95f;
        
        // Check terrain collision
        if (checkTerrainCollision(*terrain)) {
            // Move tank up until not colliding
            while (checkTerrainCollision(*terrain) && pos.y > 0) {
                pos.y -= 1.0f;
            }
            velocity.y = 0;
            onGround = true;
        } else {
            onGround = false;
        }
    }
    
    // Shooting is handled externally now (creates missile)
    if (state & SHOOT)
    {
        state &= ~SHOOT;
    }
}
