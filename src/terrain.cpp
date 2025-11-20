#include "../header/terrain.h"
#include <cmath>
#include <iostream>
#include <random>

Terrain::Terrain(unsigned int w, unsigned int h)
    : width(w), height(h), needsUpdate(false)
{
    terrainImage.create(width, height, sf::Color::Transparent);
    terrainTexture.create(width, height);
    terrainSprite.setTexture(terrainTexture);
}

Terrain::~Terrain()
{
}

bool Terrain::loadFromFile(const std::string& filename)
{
    if (!terrainImage.loadFromFile(filename)) {
        std::cerr << "Failed to load terrain from: " << filename << std::endl;
        return false;
    }
    
    width = terrainImage.getSize().x;
    height = terrainImage.getSize().y;
    
    terrainTexture.loadFromImage(terrainImage);
    terrainSprite.setTexture(terrainTexture, true);
    
    return true;
}

void Terrain::generateTerrain()
{
    // Generate simple procedural terrain with perlin-like noise
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    // Create a simple hill-based terrain
    std::vector<int> heights(width);
    
    // Generate base heights using sine waves for smooth terrain
    for (unsigned int x = 0; x < width; x++) {
        float t = static_cast<float>(x) / width;
        float h = std::sin(t * 3.14159f * 4.0f) * 0.2f + 0.5f; // Oscillating height
        h += std::sin(t * 3.14159f * 2.0f) * 0.1f; // Add variation
        heights[x] = static_cast<int>(h * height * 0.6f + height * 0.3f);
    }
    
    // Fill terrain image
    for (unsigned int x = 0; x < width; x++) {
        for (unsigned int y = 0; y < height; y++) {
            if (y >= heights[x]) {
                // Gradient color based on depth
                int depth = y - heights[x];
                sf::Uint8 green = 200 - std::min(depth * 2, 100);
                sf::Uint8 brown = 100 + std::min(depth, 100);
                terrainImage.setPixel(x, y, sf::Color(brown, green, 50));
            } else {
                terrainImage.setPixel(x, y, sf::Color::Transparent);
            }
        }
    }
    
    terrainTexture.loadFromImage(terrainImage);
    terrainSprite.setTexture(terrainTexture, true);
}

bool Terrain::isColliding(int x, int y) const
{
    if (x < 0 || x >= static_cast<int>(width) || 
        y < 0 || y >= static_cast<int>(height)) {
        return false;
    }
    
    sf::Color pixel = terrainImage.getPixel(x, y);
    return pixel.a > 128; // Solid if alpha > 128
}

bool Terrain::isCircleColliding(float x, float y, float radius) const
{
    // Check multiple points around the circle
    int numChecks = 8;
    for (int i = 0; i < numChecks; i++) {
        float angle = (i / static_cast<float>(numChecks)) * 2.0f * 3.14159f;
        int checkX = static_cast<int>(x + std::cos(angle) * radius);
        int checkY = static_cast<int>(y + std::sin(angle) * radius);
        
        if (isColliding(checkX, checkY)) {
            return true;
        }
    }
    
    // Also check center
    return isColliding(static_cast<int>(x), static_cast<int>(y));
}

void Terrain::destroyCircle(float x, float y, float radius)
{
    int minX = std::max(0, static_cast<int>(x - radius));
    int maxX = std::min(static_cast<int>(width), static_cast<int>(x + radius + 1));
    int minY = std::max(0, static_cast<int>(y - radius));
    int maxY = std::min(static_cast<int>(height), static_cast<int>(y + radius + 1));
    
    for (int px = minX; px < maxX; px++) {
        for (int py = minY; py < maxY; py++) {
            float dx = px - x;
            float dy = py - y;
            float dist = std::sqrt(dx * dx + dy * dy);
            
            if (dist <= radius) {
                terrainImage.setPixel(px, py, sf::Color::Transparent);
            }
        }
    }
    
    needsUpdate = true;
}

void Terrain::updateTexture()
{
    if (needsUpdate) {
        terrainTexture.loadFromImage(terrainImage);
        needsUpdate = false;
    }
}

void Terrain::draw(sf::RenderWindow& window)
{
    updateTexture();
    window.draw(terrainSprite);
}
