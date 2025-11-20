#include <iostream>
#include <string>
#include <cstring>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "../header/menu.h"
#include "../header/tool.h"

int main(int argc, char *argv[]) {

    std::cout << "Analyse des " << argc << " parametres la ligne de commande ..." << std::endl;
    int screen_width_init = 640;
    int screen_height_init = 480;
    float fps = 60; // Default to 60 for SFML
    int screen_id_init = 0;

    for(int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if(arg == "--size"){
            if(argc < i+3){
                std::cerr << "Erreur, --size flag wants two parameters !" << std::endl;
                return -1;
            }else{
                screen_width_init = std::stoi(argv[++i]);
                screen_height_init = std::stoi(argv[++i]);
            }
        }else if(arg == "--fps"){
            if(argc < i+2){
                std::cerr << "Erreur, --fps flag wants one parameter !" << std::endl;
                return -1;
            }else{
                fps = std::stof(argv[++i]);
            }
        }else if(arg == "--screen"){
            if(argc < i+2){
                std::cerr << "Erreur, --screen flag wants one parameter !" << std::endl;
                return -1;
            }else{
                screen_id_init = std::stoi(argv[++i]);
            }
        }
    }

    GameSettings settings;
    settings.fps = fps;
    settings.originalWidth = screen_width_init;
    settings.originalHeight = screen_height_init;
    
    std::cout << "Initialisation de la SFML ...." << std::endl;
    
    // Create resizable window
    sf::RenderWindow window(sf::VideoMode(screen_width_init, screen_height_init), "TankStorm", sf::Style::Default);
    window.setFramerateLimit(static_cast<unsigned int>(fps));
    
    settings.window = &window;

    menu(&settings);

    std::cout << "Nettoyage des ressources avant cloture du programme ..." << std::endl;
    // SFML handles cleanup automatically with RAII

    return 0;
}