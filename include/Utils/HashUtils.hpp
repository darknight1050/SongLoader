#pragma once
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"

#include <string>

namespace RuntimeSongLoader::HashUtils {
    
    std::optional<std::string> GetCustomLevelHash(GlobalNamespace::StandardLevelInfoSaveData* level, std::string customLevelPath);
    std::optional<int> GetDirectoryHash(std::string_view path);
}