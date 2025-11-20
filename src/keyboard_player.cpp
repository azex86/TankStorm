#include "../header/keyboard_player.h"

void keyboardPlayerCheck(Tank* tank, KeyboardSettings settings, const sf::Event& event)
{
    if (event.type == sf::Event::KeyPressed)
    {
        if (event.key.code == settings.right)
        {
            tank->moveRight();
        }
        else if (event.key.code == settings.left)
        {
            tank->moveLeft();
        }
        else if (event.key.code == settings.shoot)
        {
            tank->shoot();
        }
    }
    else if (event.type == sf::Event::KeyReleased)
    {
        if (event.key.code == settings.right)
        {
            tank->moveRightStop();
        }
        else if (event.key.code == settings.left)
        {
            tank->moveLeftStop();
        }
    }
}
