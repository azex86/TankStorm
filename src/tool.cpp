#include "../header/tool.h"

sf::Font global_font;

void init_font(void)
{
    if (!global_font.loadFromFile("res/font/arial.ttf"))
    {
        std::cerr << "Error during font loading" << std::endl;
        exit(1);
    }
}