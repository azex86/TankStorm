#include "../header/play.h"
#include "../header/button.h"
#include <iostream>

void drawBackground(sf::RenderWindow& window)
{
    window.clear(sf::Color(113, 230, 249)); // cyan
}

void play(GameSettings* settings)
{
    std::cout << "Chargement et affichage du jeu ..." << std::endl;
    sf::RenderWindow* window = settings->window;
    init_font();
    window->setTitle("Jeu");

    bool quit = false;

    Tank* mytank = new Tank("res/tank.bmp", "res/canon.bmp");
    // Use window size to center tank or something? 
    // Original code: tank->pos.x = 0; tank->pos.y = 0;
    
    KeyboardSettings mysettings = {sf::Keyboard::Right, sf::Keyboard::Left, sf::Keyboard::Space};

    while (!quit && window->isOpen()) {
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

        mytank->update();
        
        // Update tank target point with mouse position
        sf::Vector2i mousePos = sf::Mouse::getPosition(*window);
        mytank->targetPoint(sf::Vector2f((float)mousePos.x, (float)mousePos.y));

        drawBackground(*window);
        
        Button::drawAll(*window);
        mytank->draw(*window);

        window->display();
    }
    
    Button::clearAll();
    delete mytank;
    std::cout << "Fin du jeu" << std::endl;
}
