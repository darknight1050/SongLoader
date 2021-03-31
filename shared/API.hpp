#pragma once

#include <string>

#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"

namespace RuntimeSongLoader::API {

    void RefreshSongs(bool fullRefresh = true);

    void AddSongsLoadedEvent(std::function<void(const std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*>&)> event);

    std::string GetCustomLevelsPath();

    std::string GetCustomWIPLevelsPath();

}