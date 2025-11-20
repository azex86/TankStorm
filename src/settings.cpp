#include "../header/settings.h"
#include "../header/button.h"
#include <iostream>
#include <sstream>
#include <iomanip>

TextField::TextField(float x, float y, float width, float height, const std::string& label, float initialValue)
    : label(label), active(false), x(x), y(y), width(width), height(height)
{
    box.setPosition(x, y);
    box.setSize(sf::Vector2f(width, height));
    box.setFillColor(sf::Color::White);
    box.setOutlineColor(sf::Color::Black);
    box.setOutlineThickness(2);
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << initialValue;
    text = oss.str();
}

void TextField::draw(sf::RenderWindow& window, const sf::Font& font)
{
    // Draw box
    if (active) {
        box.setOutlineColor(sf::Color::Blue);
        box.setOutlineThickness(3);
    } else {
        box.setOutlineColor(sf::Color::Black);
        box.setOutlineThickness(2);
    }
    window.draw(box);
    
    // Draw label
    sf::Text labelText;
    labelText.setFont(font);
    labelText.setString(label);
    labelText.setCharacterSize(18);
    labelText.setFillColor(sf::Color::Black);
    labelText.setPosition(x, y - 25);
    window.draw(labelText);
    
    // Draw value
    sf::Text valueText;
    valueText.setFont(font);
    valueText.setString(text);
    valueText.setCharacterSize(20);
    valueText.setFillColor(sf::Color::Black);
    valueText.setPosition(x + 10, y + 10);
    window.draw(valueText);
}

void TextField::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (event.type == sf::Event::MouseButtonPressed) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        if (contains(mousePos.x, mousePos.y)) {
            active = true;
        } else {
            active = false;
        }
    }
    
    if (active && event.type == sf::Event::TextEntered) {
        if (event.text.unicode == 8) { // Backspace
            if (!text.empty()) {
                text.pop_back();
            }
        } else if (event.text.unicode == 13) { // Enter
            active = false;
        } else if ((event.text.unicode >= '0' && event.text.unicode <= '9') || event.text.unicode == '.') {
            if (text.length() < 10) {
                text += static_cast<char>(event.text.unicode);
            }
        }
    }
}

void TextField::setActive(bool active)
{
    this->active = active;
}

float TextField::getValue() const
{
    try {
        return std::stof(text);
    } catch (...) {
        return 0.0f;
    }
}

void TextField::setValue(float value)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << value;
    text = oss.str();
}

bool TextField::contains(float px, float py) const
{
    return px >= x && px <= x + width && py >= y && py <= y + height;
}

void settings(GameSettings* gameSettings, Config* config)
{
    std::cout << "Chargement et affichage des paramètres ..." << std::endl;
    sf::RenderWindow* window = gameSettings->window;
    init_font();
    window->setTitle("Settings");
    
    Button::clearAll();
    
    bool quit = false;
    
    // Create text fields for settings
    std::vector<TextField> fields;
    fields.push_back(TextField(200, 100, 200, 40, "Gravity", config->getGravity()));
    fields.push_back(TextField(200, 170, 200, 40, "Tank Speed", config->getTankSpeed()));
    fields.push_back(TextField(200, 240, 200, 40, "Missile Speed", config->getMissileSpeed()));
    fields.push_back(TextField(200, 310, 200, 40, "Explosion Radius", config->getExplosionRadius()));
    fields.push_back(TextField(200, 380, 200, 40, "Explosion Force", config->getExplosionForce()));
    
    // Create buttons
    Button* saveButton = new Button("Save", 250, 450, 140, 40, [&quit, config, &fields]() {
        config->setGravity(fields[0].getValue());
        config->setTankSpeed(fields[1].getValue());
        config->setMissileSpeed(fields[2].getValue());
        config->setExplosionRadius(fields[3].getValue());
        config->setExplosionForce(fields[4].getValue());
        config->saveToFile("config.ini");
        std::cout << "Settings saved!" << std::endl;
        quit = true;
    });
    
    Button* backButton = new Button("Back", 250, 500, 140, 40, [&quit]() {
        quit = true;
    });
    
    while (!quit && window->isOpen()) {
        sf::Event event;
        while (window->pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                quit = true;
                window->close();
            }
            else if (event.type == sf::Event::Resized)
            {
                // Update view to maintain aspect ratio
                sf::View view = window->getView();
                view.setSize(gameSettings->originalWidth, gameSettings->originalHeight);
                view.setCenter(gameSettings->originalWidth / 2.f, gameSettings->originalHeight / 2.f);
                view = getLetterboxView(view, event.size.width, event.size.height);
                window->setView(view);
            }
            else if (event.type == sf::Event::KeyReleased) {
                if (event.key.code == sf::Keyboard::Escape) {
                    quit = true;
                }
            }
            
            Button::checkAll(event, *window);
            
            // Handle text field events
            for (auto& field : fields) {
                field.handleEvent(event, *window);
            }
        }
        
        // Draw
        window->clear(sf::Color(200, 200, 200));
        
        // Draw title
        sf::Text title;
        title.setFont(global_font);
        title.setString("Settings");
        title.setCharacterSize(32);
        title.setFillColor(sf::Color::Black);
        title.setPosition(250, 20);
        window->draw(title);
        
        // Draw text fields
        for (auto& field : fields) {
            field.draw(*window, global_font);
        }
        
        Button::drawAll(*window);
        window->display();
    }
    
    Button::clearAll();
    std::cout << "Fermeture des paramètres" << std::endl;
}
