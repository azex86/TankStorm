#ifndef KEYBOARD_PLAYER_HEADER
#define KEYBOARD_PLAYER_HEADER

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Event.hpp>
#include "tank.h"

struct KeyboardSettings
{
    sf::Keyboard::Key right;
    sf::Keyboard::Key left;
    sf::Keyboard::Key shoot;
};

void keyboardPlayerCheck(Tank* tank, KeyboardSettings settings, const sf::Event& event);

#endif
