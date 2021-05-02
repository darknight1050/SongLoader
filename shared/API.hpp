#pragma once

#include <string>

#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp"

namespace RuntimeSongLoader::API {

    /// @brief Loads Songs on disk. This will reload already loaded songs
    void RefreshSongs();
    
    /// @brief Loads Songs on disk
    /// @tparam fullRefresh If it should reload already loaded songs
    /// @tparam songsLoaded gets called after songs got loaded
    void RefreshSongs(bool fullRefresh, std::function<void(const std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*>&)> songsLoaded = nullptr);

    /// @brief Add a loading callback that gets called after songs got loaded
    /// @tparam event Callback event
    void AddSongsLoadedEvent(std::function<void(const std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*>&)> event);

    /// @brief Add a callback that gets called before level packs get refreshed
    /// @tparam event Callback event
    void AddRefreshLevelPacksEvent(std::function<void(SongLoaderBeatmapLevelPackCollectionSO*)> event);

    void DeleteSong(std::string path, std::function<void()> finished = nullptr);
    
    std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLoadedSongs();

    std::optional<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLevelByHash(std::string hash);
    
    std::optional<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLevelById(std::string levelID);

    std::string GetCustomLevelsPrefix();

    std::string GetCustomLevelsPath();

    std::string GetCustomWIPLevelsPath();

}