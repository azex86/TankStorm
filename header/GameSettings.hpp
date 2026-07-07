#include <string>
#include <map>

class GameSettings
{
    private:
        std::string file_path;
        std::map<std::string,std::string> values;


    public:
        GameSettings(std::string& file_path, std::map<std::string,std::string>& dictionnary);

        static GameSettings load_from_file(std::string file_path);
        static GameSettings init();

        std::string get(std::string key,std::string default_value = "")const;
        const std::string& operator[](std::string key)const;
        std::string& operator[](std::string key);
};