#pragma once

#include "./_config.h"
#include <string>

#include "GlobalNamespace/BeatmapLevel.hpp"
#include "GlobalNamespace/FileSystemBeatmapLevelData.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/BeatmapDataBasicInfo.hpp"
#include "BeatmapSaveDataVersion3/BeatmapSaveData.hpp"
#include "CustomTypes/SongLoaderBeatmapLevelsRepository.hpp"
#include "CustomTypes/CustomLevelInfoSaveData.hpp"
#include "CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"

namespace RuntimeSongLoader::API {

    /// @brief Loads Songs on disk. This will reload already loaded songs
    SONGLOADER_EXPORT void RefreshSongs();

    /// @brief Loads Songs on disk
    /// @tparam fullRefresh If it should reload already loaded songs
    /// @tparam songsLoaded gets called after songs got loaded
    SONGLOADER_EXPORT void RefreshSongs(bool fullRefresh, std::function<void(std::vector<GlobalNamespace::BeatmapLevel*> const&)> const& songsLoaded = nullptr);

    /// @brief Loads Packs on disk
    /// @tparam includeDefault If the default custom levels playlists should be added
    SONGLOADER_EXPORT void RefreshPacks(bool includeDefault = true);

    /// @brief Add a loading callback that gets called after songs got loaded
    /// @tparam event Callback event
    SONGLOADER_EXPORT void AddSongsLoadedEvent(std::function<void(std::vector<GlobalNamespace::BeatmapLevel*> const&)> const& event);

    /// @brief Add a callback that gets called before level packs get refreshed
    /// @tparam event Callback event
    SONGLOADER_EXPORT void AddRefreshLevelPacksEvent(std::function<void(SongLoaderBeatmapLevelsRepository*)> const& event);

    /// @brief Add a callback that gets called when a song is deleted
    /// @tparam event Callback event
    SONGLOADER_EXPORT void AddSongDeletedEvent(std::function<void()> const& event);

    /// @brief Delets a song on filesystem (doesn't refresh songs)
    /// @tparam path Path to the song on filesystem
    /// @tparam finished Callback once done
    SONGLOADER_EXPORT void DeleteSong(std::string_view path, std::function<void()> const& finished = nullptr);

    SONGLOADER_EXPORT std::vector<GlobalNamespace::BeatmapLevel*> GetLoadedSongs();

    /// @brief If songs did get loaded
    SONGLOADER_EXPORT bool HasLoadedSongs();

    /// @brief gets how far along the loading progress the songloader is
    SONGLOADER_EXPORT float GetLoadingProgress();

    SONGLOADER_EXPORT std::optional<GlobalNamespace::BeatmapLevel*> GetLevelByHash(std::string hash);

    SONGLOADER_EXPORT std::optional<GlobalNamespace::BeatmapLevel*> GetLevelById(std::string_view levelID);

    SONGLOADER_EXPORT std::optional<std::string> GetLevelPathByHash(std::string hash);

    SONGLOADER_EXPORT std::optional<CustomJSONData::CustomLevelInfoSaveData*> GetCustomSaveDataByHash(std::string hash);

    SONGLOADER_EXPORT std::string GetCustomLevelsPrefix();

    SONGLOADER_EXPORT std::string GetCustomLevelPacksPrefix();

    SONGLOADER_EXPORT std::string GetCustomLevelsPath();

    SONGLOADER_EXPORT std::string GetCustomWIPLevelsPath();

    /// @brief gets custom levels pack from the songloader instance
    SONGLOADER_EXPORT RuntimeSongLoader::SongLoaderCustomBeatmapLevelPack* GetCustomLevelsPack();

    /// @brief gets custom wip levels collection from the songloader instance
    SONGLOADER_EXPORT RuntimeSongLoader::SongLoaderCustomBeatmapLevelPack* GetCustomWIPLevelsPack();
}
