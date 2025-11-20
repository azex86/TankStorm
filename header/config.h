#pragma once
#ifndef CONFIG_HEADER
#define CONFIG_HEADER

#include <SFML/Window/Keyboard.hpp>
#include <string>
#include <map>

class Config {
public:
    Config();
    ~Config();

    // Load configuration from file
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename);
    
    // Control settings
    sf::Keyboard::Key getMoveLeftKey() const { return moveLeftKey; }
    sf::Keyboard::Key getMoveRightKey() const { return moveRightKey; }
    bool isShootMouseButton() const { return shootWithMouse; }
    
    // Game settings
    float getGravity() const { return gravity; }
    float getTankSpeed() const { return tankSpeed; }
    float getMissileSpeed() const { return missileSpeed; }
    float getExplosionRadius() const { return explosionRadius; }
    float getExplosionForce() const { return explosionForce; }
    
    // Setters for game settings
    void setGravity(float value) { gravity = value; }
    void setTankSpeed(float value) { tankSpeed = value; }
    void setMissileSpeed(float value) { missileSpeed = value; }
    void setExplosionRadius(float value) { explosionRadius = value; }
    void setExplosionForce(float value) { explosionForce = value; }
    
    // Window settings
    int getWindowWidth() const { return windowWidth; }
    int getWindowHeight() const { return windowHeight; }
    int getFPS() const { return fps; }

private:
    // Controls
    sf::Keyboard::Key moveLeftKey;
    sf::Keyboard::Key moveRightKey;
    bool shootWithMouse;
    
    // Game settings
    float gravity;
    float tankSpeed;
    float missileSpeed;
    float explosionRadius;
    float explosionForce;
    
    // Window settings
    int windowWidth;
    int windowHeight;
    int fps;
    
    // Helper methods
    sf::Keyboard::Key stringToKey(const std::string& keyName);
    std::string trim(const std::string& str);
};

#endif // CONFIG_HEADER
