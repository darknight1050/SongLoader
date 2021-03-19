#include "Utils/FileUtils.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include <fstream>
#include <iterator>
#include <dirent.h>

namespace FileUtils {
    
    std::string ReadAllText(std::string_view path) {
        if(!fileexists(path))
            return "";
        std::ifstream fileStream(path.data(), std::ifstream::in | std::ios::binary);
        std::string text((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
        return text;
    }

    const char* ReadAllBytes(std::string_view path, size_t& outSize) {
        outSize = 0;
        if(!fileexists(path))
            return nullptr;
        std::ifstream fileStream(path.data(), std::ifstream::in | std::ios::binary | std::ifstream::ate);
        outSize = fileStream.tellg();
        if(!fileStream.is_open())
            return nullptr;
        char* data = new char[outSize];
        fileStream.seekg(fileStream.beg);
        fileStream.read(data, outSize);
        return data;
    }

    std::vector<std::string> GetFolders(std::string_view path) {
        std::vector<std::string> directories;
        std::string fullPath(path);
        DIR *dir;
        struct dirent *ent;
        if((dir = opendir (fullPath.c_str())) != nullptr) {
            while((ent = readdir (dir)) != nullptr) {
                std::string name = ent->d_name;
                if(name != "." && name != "..")
                    directories.push_back(fullPath + "/" + name);
            }
            closedir (dir);
        }
        return directories;
    }

}