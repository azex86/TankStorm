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

sf::View getLetterboxView(sf::View view, int windowWidth, int windowHeight)
{
    // Get the original view size
    sf::FloatRect viewport(0.f, 0.f, 1.f, 1.f);
    
    float windowRatio = windowWidth / (float) windowHeight;
    float viewRatio = view.getSize().x / (float) view.getSize().y;
    float sizeX = 1;
    float sizeY = 1;
    float posX = 0;
    float posY = 0;

    bool horizontalSpacing = true;
    if (windowRatio < viewRatio)
        horizontalSpacing = false;

    // If horizontalSpacing is true, the black bars will appear on the left and right side.
    // Otherwise, the black bars will appear on the top and bottom.

    if (horizontalSpacing) {
        sizeX = viewRatio / windowRatio;
        posX = (1 - sizeX) / 2.f;
    }
    else {
        sizeY = windowRatio / viewRatio;
        posY = (1 - sizeY) / 2.f;
    }

    viewport = sf::FloatRect(posX, posY, sizeX, sizeY);
    view.setViewport(viewport);

    return view;
}