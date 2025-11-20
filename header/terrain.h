#pragma once
#ifndef TERRAIN_HEADER
#define TERRAIN_HEADER

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class Terrain {
public:
    Terrain(unsigned int width, unsigned int height);
    ~Terrain();

    // Load terrain from image file
    bool loadFromFile(const std::string& filename);
    
    // Generate procedural terrain
    void generateTerrain();
    
    // Check if a point collides with solid terrain
    bool isColliding(int x, int y) const;
    
    // Check if a circle collides with terrain
    bool isCircleColliding(float x, float y, float radius) const;
    
    // Destroy terrain in a circular area
    void destroyCircle(float x, float y, float radius);
    
    // Draw the terrain
    void draw(sf::RenderWindow& window);
    
    // Get terrain dimensions
    unsigned int getWidth() const { return width; }
    unsigned int getHeight() const { return height; }
    
    // Update texture from image data (call after destruction)
    void updateTexture();

private:
    unsigned int width;
    unsigned int height;
    
    sf::Image terrainImage;      // Pixel data for collision
    sf::Texture terrainTexture;  // Texture for rendering
    sf::Sprite terrainSprite;    // Sprite for drawing
    
    bool needsUpdate;            // Flag to update texture
};

#endif // TERRAIN_HEADER
