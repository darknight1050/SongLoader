
#pragma once

#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
namespace RuntimeSongLoader::CustomBeatmapLevelLoader {

    void AddBeatmapDataLoadedEvent(std::function<void(GlobalNamespace::StandardLevelInfoSaveData*, GlobalNamespace::BeatmapData*)> event);

    void InstallHooks();

}