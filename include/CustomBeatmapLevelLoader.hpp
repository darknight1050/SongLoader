
#pragma once

#include "GlobalNamespace/BeatmapDataBasicInfo.hpp"
#include "BeatmapSaveDataVersion3/BeatmapSaveData.hpp"

#include "CustomTypes/CustomLevelInfoSaveData.hpp"
namespace RuntimeSongLoader::CustomBeatmapLevelLoader {

    void AddBeatmapDataBasicInfoLoadedEvent(std::function<void(CustomJSONData::CustomLevelInfoSaveData*, std::string const&, BeatmapSaveDataVersion3::BeatmapSaveData*, GlobalNamespace::BeatmapDataBasicInfo*)> const& event);

    void InstallHooks();

}