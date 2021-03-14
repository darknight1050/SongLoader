#pragma once
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"

#include <string>

namespace HashUtils {
    
    std::string GetCustomLevelHash(GlobalNamespace::StandardLevelInfoSaveData* level, std::string customLevelPath);

}