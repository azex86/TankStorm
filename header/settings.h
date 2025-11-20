#pragma once
#ifndef SETTINGS_HEADER
#define SETTINGS_HEADER

#include "tool.h"
#include "config.h"
#include <string>
#include <vector>

class TextField {
public:
    TextField(float x, float y, float width, float height, const std::string& label, float initialValue);
    
    void draw(sf::RenderWindow& window, const sf::Font& font);
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void setActive(bool active);
    bool isActive() const { return active; }
    float getValue() const;
    void setValue(float value);
    bool contains(float x, float y) const;
    
private:
    sf::RectangleShape box;
    std::string label;
    std::string text;
    bool active;
    float x, y, width, height;
};

void settings(GameSettings* settings, Config* config);

#endif // SETTINGS_HEADER
