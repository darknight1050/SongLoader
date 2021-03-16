#pragma once
#include <string>

namespace FileUtils {
    
    std::string ReadAllText(const std::string& path);
    
    std::vector<std::string> GetFolders(const std::string& path);

}