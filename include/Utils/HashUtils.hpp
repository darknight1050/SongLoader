#pragma once
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "CustomTypes/CustomLevelInfoSaveData.hpp"

#include <string>

namespace RuntimeSongLoader::HashUtils {
    
    std::optional<std::string> GetCustomLevelHash(CustomJSONData::CustomLevelInfoSaveData* level, std::string const& customLevelPath);
    std::optional<int> GetDirectoryHash(std::string_view path);
}