#include "Utils/FileUtils.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include <fstream>
#include <iterator>

namespace FileUtils {
    
    std::string ReadAllText(std::string path) {
        if(!fileexists(path))
            return "";
        std::ifstream fileStream(path.c_str());
        std::string text((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
        return text;
    }

}