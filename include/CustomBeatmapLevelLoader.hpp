
#pragma once

#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/BeatmapData.hpp"

#include "CustomTypes/CustomLevelInfoSaveData.hpp"
namespace RuntimeSongLoader::CustomBeatmapLevelLoader {

    void AddBeatmapDataLoadedEvent(const std::function<void(CustomJSONData::CustomLevelInfoSaveData*, const std::string&, GlobalNamespace::BeatmapData*)>& event);

    void InstallHooks();

}