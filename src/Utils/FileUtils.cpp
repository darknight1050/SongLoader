#include "Utils/FileUtils.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include <fstream>
#include <iterator>
#include <dirent.h>

namespace FileUtils {
    
    std::string ReadAllText(const std::string& path) {
        if(!fileexists(path))
            return "";
        std::ifstream fileStream(path.c_str());
        std::string text((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
        return text;
    }

    std::vector<std::string> GetFolders(const std::string& path)
    {
        std::vector<std::string> directories;
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir (path.c_str())) != NULL) {
            while ((ent = readdir (dir)) != NULL) {
                std::string name = ent->d_name;
                if(name != "." && name != "..")
                    directories.push_back(path + "/" + name);
            }
            closedir (dir);
        }
        return directories;
    }

}