#include "GameSettings.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

using namespace std;

GameSettings::GameSettings(std::string &file_path, std::map<std::string, std::string> &dictionnary)
{
    this->file_path = file_path;
    this->values = values;
}

GameSettings GameSettings::load_from_file(std::string file_path)
{
    
    try
    {
        map<string,string>dict;
        fstream file(file_path);
        string line;

        while (getline(file,line))
        {
            string key = "",value = "";
            size_t split_index = line.find_first_of("=");
            key = line.substr(0,split_index);
            value = line.substr(split_index);
            
            dict[key]=value;
        }
        
        return GameSettings(file_path,dict);

    }
    catch(const std::exception& e)
    {
        std::cerr <<"Erreur lors du chargement des paramètres du jeu depuis le fichier " <<file_path <<" : " << e.what() << '\n';
        throw e;
    }
}

#define GAME_SETTINGS_FILE_PATH "TankStorm.ini"

GameSettings GameSettings::init()
{
    /*
        on essaie de charger dans l'odre les fichiers: 
            ./TankStorm.ini
            ../TankStorm.ini
    */

    string file_path = "";

    for(auto path : filesystem::directory_iterator())
    {
        if(path.is_regular_file() && path.path() == GAME_SETTINGS_FILE_PATH)
        {
            file_path = path.path();
            break;
        }
    }

    for(auto path : filesystem::directory_iterator(".."))
    {
        if(path.is_regular_file() && path.path() == GAME_SETTINGS_FILE_PATH)
        {
            file_path = path.path();
            break;
        }
    }

    if(file_path=="")
    {
        cerr <<"Creates " <<GAME_SETTINGS_FILE_PATH <<endl;
        ofstream f(GAME_SETTINGS_FILE_PATH);
        f.close();
    }


    return GameSettings::load_from_file(file_path);
}

std::string GameSettings::get(std::string key, std::string default_value) const
{
    return values[key];
}