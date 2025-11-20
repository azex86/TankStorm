#include "../header/button.h"
#include <iostream>

std::vector<Button*> Button::all_buttons;

Button::Button(const std::string& text, float x, float y, float w, float h, Callback cb)
    : callback(cb), hovered(false)
{
    shape.setPosition(x, y);
    shape.setSize(sf::Vector2f(w, h));
    shape.setFillColor(sf::Color(100, 100, 100));
    shape.setOutlineColor(sf::Color::White);
    shape.setOutlineThickness(2);

    label.setFont(global_font);
    label.setString(text);
    label.setCharacterSize(20);
    label.setFillColor(sf::Color::White);
    
    sf::FloatRect textRect = label.getLocalBounds();
    label.setOrigin(textRect.left + textRect.width/2.0f,
                    textRect.top  + textRect.height/2.0f);
    label.setPosition(x + w/2.0f, y + h/2.0f);

    all_buttons.push_back(this);
}

Button::~Button()
{
    for (auto it = all_buttons.begin(); it != all_buttons.end(); ++it) {
        if (*it == this) {
            all_buttons.erase(it);
            break;
        }
    }
}

void Button::draw(sf::RenderWindow& window)
{
    if (hovered) {
        shape.setFillColor(sf::Color(150, 150, 150));
    } else {
        shape.setFillColor(sf::Color(100, 100, 100));
    }
    window.draw(shape);
    window.draw(label);
}

void Button::check(const sf::Event& event, const sf::RenderWindow& window)
{
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
        if (shape.getGlobalBounds().contains(mousePosF)) {
            hovered = true;
        } else {
            hovered = false;
        }
    }
    
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
        if (shape.getGlobalBounds().contains(mousePosF)) {
            if (callback) {
                callback();
            }
        }
    }
}

void Button::drawAll(sf::RenderWindow& window)
{
    for (auto btn : all_buttons) {
        btn->draw(window);
    }
}

void Button::checkAll(const sf::Event& event, const sf::RenderWindow& window)
{
    std::vector<Button*> current_buttons = all_buttons;
    for (auto btn : current_buttons) {
        bool exists = false;
        for (auto b : all_buttons) {
            if (b == btn) {
                exists = true;
                break;
            }
        }
        if (exists) {
            btn->check(event, window);
        }
    }
}

void Button::clearAll()
{
    while (!all_buttons.empty()) {
        delete all_buttons.back();
    }
}

std::vector<Button*> Button::takeAll()
{
    std::vector<Button*> temp = all_buttons;
    all_buttons.clear();
    return temp;
}

void Button::restoreAll(const std::vector<Button*>& buttons)
{
    all_buttons = buttons;
}
