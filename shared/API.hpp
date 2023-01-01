#pragma once

#include <string>

#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/BeatmapDataBasicInfo.hpp"
#include "BeatmapSaveDataVersion3/BeatmapSaveData.hpp"
#include "CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp"
#include "CustomTypes/CustomLevelInfoSaveData.hpp"
#include "CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"

namespace RuntimeSongLoader::API {

    /// @brief Loads Songs on disk. This will reload already loaded songs
    void RefreshSongs();
    
    /// @brief Loads Songs on disk
    /// @tparam fullRefresh If it should reload already loaded songs
    /// @tparam songsLoaded gets called after songs got loaded
    void RefreshSongs(bool fullRefresh, std::function<void(std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> const&)> const& songsLoaded = nullptr);

    /// @brief Loads Packs on disk
    /// @tparam includeDefault If the default custom levels playlists should be added
    void RefreshPacks(bool includeDefault = true);

    /// @brief Add a loading callback that gets called after songs got loaded
    /// @tparam event Callback event
    void AddSongsLoadedEvent(std::function<void(std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> const&)> const& event);

    /// @brief Add a callback that gets called before level packs get refreshed
    /// @tparam event Callback event
    void AddRefreshLevelPacksEvent(std::function<void(SongLoaderBeatmapLevelPackCollectionSO*)> const& event);

    /// @brief Add a callback that gets called after it tried to load a BeatmapData
    /// @tparam event Callback event
    void AddBeatmapDataBasicInfoLoadedEvent(std::function<void(CustomJSONData::CustomLevelInfoSaveData*, std::string const&, BeatmapSaveDataVersion3::BeatmapSaveData*, GlobalNamespace::BeatmapDataBasicInfo*)> const& event);
    
    /// @brief Add a callback that gets called when a song is deleted
    /// @tparam event Callback event
    void AddSongDeletedEvent(std::function<void()> const& event);

    /// @brief Delets a song on filesystem (doesn't refresh songs)
    /// @tparam path Path to the song on filesystem
    /// @tparam finished Callback once done
    void DeleteSong(std::string_view path, std::function<void()> const& finished = nullptr);
    
    std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLoadedSongs();

    /// @brief If songs did get loaded
    bool HasLoadedSongs();

    /// @brief gets how far along the loading progress the songloader is
    float GetLoadingProgress();

    std::optional<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLevelByHash(std::string hash);
    
    std::optional<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLevelById(std::string_view levelID);

    std::string GetCustomLevelsPrefix();

    std::string GetCustomLevelPacksPrefix();

    std::string GetCustomLevelsPath();

    std::string GetCustomWIPLevelsPath();

    /// @brief gets custom levels pack from the songloader instance
    RuntimeSongLoader::SongLoaderCustomBeatmapLevelPack* GetCustomLevelsPack();

    /// @brief gets custom wip levels collection from the songloader instance
    RuntimeSongLoader::SongLoaderCustomBeatmapLevelPack* GetCustomWIPLevelsPack();
}