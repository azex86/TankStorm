#include "../header/menu.h"
#include <iostream>

enum UserButtonPressed
{
    NONE,
    SETTINGS,
    PLAY,
};

UserButtonPressed user_command = NONE;

void settings_pressed(void)
{
    std::cout << "Settings pressed" << std::endl;
    user_command = SETTINGS;
}

void play_pressed(void)
{
    std::cout << "Play pressed" << std::endl;
    user_command = PLAY;
}

void menu(GameSettings* settings)
{
    std::cout << "Chargement et affichage du menu ..." << std::endl;
    sf::RenderWindow* window = settings->window;
    init_font();
    window->setTitle("Menu");

    bool quit = false;

    Button* settings_Button = new Button("Settings", 100, 100, 100, 50, settings_pressed);
    Button* play_Button = new Button("Play", 100, 200, 100, 50, play_pressed);

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
                    window->close();
                }
            }

            Button::checkAll(event, *window);
        }
        
        switch (user_command)
        {
        case NONE:
            break;
        case SETTINGS:
            break;
        case PLAY:
            {
                std::vector<Button*> buttons = Button::takeAll();
                play(settings);
                Button::restoreAll(buttons);
                // Re-init font if needed, or ensure resources are valid
                window->setTitle("Menu");
            }
            break;
        default:
            break;
        }
        user_command = NONE;

        window->clear(sf::Color::Black);
        Button::drawAll(*window);
        window->display();
    }

    Button::clearAll();
}
