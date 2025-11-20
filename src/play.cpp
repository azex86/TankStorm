#include "../header/play.h"
#include "../header/button.h"
#include <iostream>
#include <vector>
#include <memory>

void drawBackground(sf::RenderWindow& window)
{
    window.clear(sf::Color(135, 206, 235)); // Sky blue
}

void play(GameSettings* settings)
{
    std::cout << "Chargement et affichage du jeu ..." << std::endl;
    sf::RenderWindow* window = settings->window;
    init_font();
    window->setTitle("Jeu");

    bool quit = false;
    
    // Get window size
    sf::Vector2u windowSize = window->getSize();

    // Create terrain
    Terrain* terrain = new Terrain(windowSize.x, windowSize.y);
    terrain->generateTerrain();
    
    // Create tank
    Tank* mytank = new Tank("res/tank.bmp", "res/canon.bmp");
    mytank->setPos(sf::Vector2f(windowSize.x / 2.0f - 40, 50)); // Start in air to test gravity
    
    KeyboardSettings mysettings = {sf::Keyboard::Right, sf::Keyboard::Left, sf::Keyboard::Space};
    
    // Missile management
    std::vector<std::unique_ptr<Missile>> missiles;
    
    // Delta time tracking
    sf::Clock clock;

    while (!quit && window->isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        
        sf::Event event;
        while (window->pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                quit = true;
                window->close();
            }
            else if (event.type == sf::Event::KeyReleased)
            {
                if (event.key.code == sf::Keyboard::Escape)
                {
                    quit = true;
                }
            }
            
            Button::checkAll(event, *window);
            keyboardPlayerCheck(mytank, mysettings, event);
        }

        // Check if tank wants to shoot - use mouse click instead of keyboard
        if (mytank->getPos().x >= 0) { // Simple check to see if tank exists
            // Handle shooting with left mouse button
            static bool mouseWasPressed = false;
            bool mouseIsPressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);
            
            if (mouseIsPressed && !mouseWasPressed) {
                // Create new missile
                missiles.push_back(mytank->createMissile());
                std::cout << "Missile fired! Total missiles: " << missiles.size() << std::endl;
            }
            mouseWasPressed = mouseIsPressed;
        }
        
        // Update tank
        mytank->update(deltaTime, terrain);
        
        // Update tank target point with mouse position
        sf::Vector2i mousePos = sf::Mouse::getPosition(*window);
        mytank->targetPoint(sf::Vector2f((float)mousePos.x, (float)mousePos.y));
        
        // Update missiles
        for (auto it = missiles.begin(); it != missiles.end(); ) {
            (*it)->update(deltaTime);
            
            // Check collision with terrain
            if ((*it)->checkTerrainCollision(*terrain)) {
                // Destroy terrain at impact point
                sf::Vector2f missilePos = (*it)->getPosition();
                terrain->destroyCircle(missilePos.x, missilePos.y, (*it)->getExplosionRadius());
                
                std::cout << "Missile exploded at (" << missilePos.x << ", " << missilePos.y << ")" << std::endl;
                
                // Remove missile
                it = missiles.erase(it);
            }
            // Check if out of bounds
            else if ((*it)->isOutOfBounds(windowSize.x, windowSize.y)) {
                it = missiles.erase(it);
            }
            else {
                ++it;
            }
        }

        // Draw everything
        drawBackground(*window);
        terrain->draw(*window);
        
        Button::drawAll(*window);
        mytank->draw(*window);
        
        // Draw missiles
        for (auto& missile : missiles) {
            missile->draw(*window);
        }

        window->display();
    }
    
    Button::clearAll();
    delete mytank;
    delete terrain;
    std::cout << "Fin du jeu" << std::endl;
}
