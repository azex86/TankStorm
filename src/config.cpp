#include "../header/config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

Config::Config()
    : moveLeftKey(sf::Keyboard::Left),
      moveRightKey(sf::Keyboard::Right),
      shootWithMouse(true),
      gravity(300.0f),
      tankSpeed(150.0f),
      missileSpeed(400.0f),
      explosionRadius(30.0f),
      explosionForce(500.0f),
      windowWidth(640),
      windowHeight(480),
      fps(60)
{
}

Config::~Config()
{
}

std::string Config::trim(const std::string& str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

sf::Keyboard::Key Config::stringToKey(const std::string& keyName)
{
    static std::map<std::string, sf::Keyboard::Key> keyMap = {
        {"Left", sf::Keyboard::Left},
        {"Right", sf::Keyboard::Right},
        {"Up", sf::Keyboard::Up},
        {"Down", sf::Keyboard::Down},
        {"Space", sf::Keyboard::Space},
        {"A", sf::Keyboard::A},
        {"D", sf::Keyboard::D},
        {"W", sf::Keyboard::W},
        {"S", sf::Keyboard::S},
        {"Q", sf::Keyboard::Q},
        {"E", sf::Keyboard::E}
    };
    
    auto it = keyMap.find(keyName);
    if (it != keyMap.end()) {
        return it->second;
    }
    return sf::Keyboard::Unknown;
}

bool Config::loadFromFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config file: " << filename << std::endl;
        std::cerr << "Using default configuration." << std::endl;
        return false;
    }
    
    std::string line;
    std::string currentSection;
    
    while (std::getline(file, line)) {
        line = trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Check for section header
        if (line[0] == '[' && line[line.length() - 1] == ']') {
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }
        
        // Parse key=value pairs
        size_t equalPos = line.find('=');
        if (equalPos != std::string::npos) {
            std::string key = trim(line.substr(0, equalPos));
            std::string value = trim(line.substr(equalPos + 1));
            
            // Controls section
            if (currentSection == "Controls") {
                if (key == "move_left") {
                    moveLeftKey = stringToKey(value);
                } else if (key == "move_right") {
                    moveRightKey = stringToKey(value);
                } else if (key == "shoot") {
                    shootWithMouse = (value == "Mouse_Left");
                }
            }
            // Game section
            else if (currentSection == "Game") {
                if (key == "gravity") {
                    gravity = std::stof(value);
                } else if (key == "tank_speed") {
                    tankSpeed = std::stof(value);
                } else if (key == "missile_speed") {
                    missileSpeed = std::stof(value);
                } else if (key == "explosion_radius") {
                    explosionRadius = std::stof(value);
                } else if (key == "explosion_force") {
                    explosionForce = std::stof(value);
                }
            }
            // Window section
            else if (currentSection == "Window") {
                if (key == "width") {
                    windowWidth = std::stoi(value);
                } else if (key == "height") {
                    windowHeight = std::stoi(value);
                } else if (key == "fps") {
                    fps = std::stoi(value);
                }
            }
        }
    }
    
    file.close();
    std::cout << "Configuration loaded from: " << filename << std::endl;
    return true;
}

bool Config::saveToFile(const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file for writing: " << filename << std::endl;
        return false;
    }
    
    file << "# TankStorm Configuration File\n\n";
    
    file << "[Controls]\n";
    file << "# Movement keys\n";
    file << "move_left=Left\n";
    file << "move_right=Right\n\n";
    file << "# Shooting\n";
    file << "shoot=Mouse_Left\n\n";
    
    file << "[Game]\n";
    file << "# Physics constants\n";
    file << "gravity=" << gravity << "\n";
    file << "tank_speed=" << tankSpeed << "\n";
    file << "missile_speed=" << missileSpeed << "\n\n";
    file << "# Explosion settings\n";
    file << "explosion_radius=" << explosionRadius << "\n";
    file << "explosion_force=" << explosionForce << "\n\n";
    
    file << "[Window]\n";
    file << "# Default window size\n";
    file << "width=" << windowWidth << "\n";
    file << "height=" << windowHeight << "\n";
    file << "fps=" << fps << "\n";
    
    file.close();
    std::cout << "Configuration saved to: " << filename << std::endl;
    return true;
}
