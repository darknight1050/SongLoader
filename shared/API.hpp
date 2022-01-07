#pragma once

#include <string>

#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp"
#include "CustomTypes/CustomLevelInfoSaveData.hpp"

namespace RuntimeSongLoader::API {

    /// @brief Loads Songs on disk. This will reload already loaded songs
    void RefreshSongs();
    
    /// @brief Loads Songs on disk
    /// @tparam fullRefresh If it should reload already loaded songs
    /// @tparam songsLoaded gets called after songs got loaded
    void RefreshSongs(bool fullRefresh, std::function<void(const std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*>&)> songsLoaded = nullptr);

    /// @brief Loads Packs on disk
    /// @tparam includeDefault If the default custom levels playlists should be added
    void RefreshPacks(bool includeDefault = true);

    /// @brief Add a loading callback that gets called after songs got loaded
    /// @tparam event Callback event
    void AddSongsLoadedEvent(std::function<void(const std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*>&)> event);

    /// @brief Add a callback that gets called before level packs get refreshed
    /// @tparam event Callback event
    void AddRefreshLevelPacksEvent(std::function<void(SongLoaderBeatmapLevelPackCollectionSO*)> event);

    /// @brief Add a callback that gets called after it tried to load a BeatmapData
    /// @tparam event Callback event
    void AddBeatmapDataLoadedEvent(std::function<void(CustomJSONData::CustomLevelInfoSaveData*, const std::string&, GlobalNamespace::BeatmapData*)> event);
    
    /// @brief Delets a song on filesystem (doesn't refresh songs)
    /// @tparam path Path to the song on filesystem
    /// @tparam finished Callback once done
    void DeleteSong(std::string path, std::function<void()> finished = nullptr);
    
    std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLoadedSongs();

    /// @brief If songs did get loaded
    bool HasLoadedSongs();

    /// @brief gets how far along the loading progress the songloader is
    float GetLoadingProgress();

    std::optional<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLevelByHash(std::string hash);
    
    std::optional<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLevelById(std::string levelID);

    std::string GetCustomLevelsPrefix();

    std::string GetCustomLevelPacksPrefix();

    std::string GetCustomLevelsPath();

    std::string GetCustomWIPLevelsPath();

}