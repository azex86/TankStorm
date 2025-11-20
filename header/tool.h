#ifndef TOOL_HEADER
#define TOOL_HEADER

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp>
#include <stdlib.h>
#include <iostream>

typedef unsigned int uint;

struct GameSettings 
{
    float fps;
    sf::RenderWindow* window;
};

extern sf::Font global_font;
void init_font(void);

#endif