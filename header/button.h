#ifndef BUTTON_HEADER
#define BUTTON_HEADER

#include <SFML/Graphics.hpp>
#include <functional>
#include <vector>
#include <string>
#include "tool.h"

class Button {
public:
    using Callback = std::function<void()>;

    Button(const std::string& text, float x, float y, float w, float h, Callback callback);
    ~Button();

    void draw(sf::RenderWindow& window);
    void check(const sf::Event& event, const sf::RenderWindow& window);

    static void drawAll(sf::RenderWindow& window);
    static void checkAll(const sf::Event& event, const sf::RenderWindow& window);
    static void clearAll();
    static std::vector<Button*> takeAll();
    static void restoreAll(const std::vector<Button*>& buttons);

private:
    sf::RectangleShape shape;
    sf::Text label;
    Callback callback;
    bool hovered;
    
    static std::vector<Button*> all_buttons;
};

#endif
