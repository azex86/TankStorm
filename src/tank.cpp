#include "../header/tank.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Tank::Tank(const std::string& corps_filename, const std::string& canon_filename)
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
    canon_sprite.setOrigin(0, canon_size.y / 2.0f); // Canon rotates around its base (assumed left side)

    pos = {0, 0};
    angle = 0;
    canon_angle = 0;
    v = 0.1f;
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

    // Canon position needs to be adjusted based on tank center
    // Assuming canon is mounted on top center of tank
    canon_sprite.setPosition(pos.x + size.x / 2.0f, pos.y + size.y / 2.0f);
    canon_sprite.setRotation(canon_angle);
    window.draw(canon_sprite);
}

void Tank::setSize(sf::Vector2i new_size)
{
    size = new_size;
    // Scale sprite if needed, but usually we just use texture size
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

    // Recalculate canon orientation based on mouse
    sf::Vector2i mousePos = sf::Mouse::getPosition();
    // Note: this uses global mouse position, might need window relative if available
    // But for now we don't have window reference here easily without passing it.
    // The original code used SDL_GetMouseState which is window relative if window has focus?
    // Let's rely on update() calling targetPoint with correct coordinates.
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
    // Calculate angle to point from tank center
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

void Tank::update()
{
    if (state & MOVE_LEFT)
    {
        move(-v);
    }
    if (state & MOVE_RIGHT)
    {
        move(v);
    }
    if (state & SHOOT)
    {
        std::cout << "Pew pew" << std::endl;
        state &= ~SHOOT;
    }

    // std::cout << "Tank : pos = (" << pos.x << ";" << pos.y << ") angle = " << angle << " canon_angle = " << canon_angle << "\r";
}
