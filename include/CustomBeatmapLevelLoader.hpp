
#pragma once

#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/BeatmapData.hpp"

#include "CustomTypes/CustomLevelInfoSaveData.hpp"
namespace RuntimeSongLoader::CustomBeatmapLevelLoader {

    void AddBeatmapDataLoadedEvent(std::function<void(CustomJSONData::CustomLevelInfoSaveData*, std::string const&, GlobalNamespace::BeatmapData*)> const& event);

    void InstallHooks();

}