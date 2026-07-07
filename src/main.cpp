#include <iostream>
#include <string>
#include <cstring>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "GameSettings.hpp"
#include "Window.hpp"
#include "Page.hpp"


int main(int argc, char *argv[]) {


    GameSettings settings = GameSettings::init();
    
    Window window = Window::init(settings);
    
    Page menu = Menu(settings);

    window.run(menu);

    return 0;
}