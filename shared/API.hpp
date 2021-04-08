#pragma once

#include <string>

#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"

namespace RuntimeSongLoader::API {

    /// @brief Loads Songs on disk
    /// @tparam fullRefresh If it should reload already loaded songs
    void RefreshSongs(bool fullRefresh = true);

    /// @brief Loads Songs on disk. 
    /// Schedules RefreshSongs to be called on the main unity thread the next time it runs
    /// Can be called from a unity or native thread
    /// @tparam fullRefresh If it should reload already loaded songs
    void RefreshSongsThreadSafe(bool fullRefresh = true);

    /// @brief Add a loading callback that gets called after songs got loaded
    /// @tparam event Callback event
    void AddSongsLoadedEvent(std::function<void(const std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*>&)> event);

    std::string GetCustomLevelsPrefix();

    std::string GetCustomLevelsPath();

    std::string GetCustomWIPLevelsPath();

}