#include "API.hpp"

#include "Paths.hpp"

#include "CustomTypes/SongLoader.hpp"
#include "CustomBeatmapLevelLoader.hpp"

namespace RuntimeSongLoader::API {

    void RefreshSongs() {
        RefreshSongs(true, nullptr);
    }

    void RefreshSongs(bool fullRefresh, std::function<void(std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> const&)> const& songsLoaded) {
        SongLoader::GetInstance()->RefreshSongs(fullRefresh, songsLoaded);
    }

    void RefreshPacks(bool includeDefault) {
        SongLoader::GetInstance()->RefreshLevelPacks(includeDefault);
    }

    void AddSongsLoadedEvent(std::function<void(std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> const&)> const& event) {
        SongLoader::AddSongsLoadedEvent(event);
    }

    void AddRefreshLevelPacksEvent(std::function<void(SongLoaderBeatmapLevelPackCollectionSO*)> const& event) {
        SongLoader::AddRefreshLevelPacksEvent(event);
    }

    void AddBeatmapDataLoadedEvent(std::function<void(CustomJSONData::CustomLevelInfoSaveData*, std::string const&, GlobalNamespace::BeatmapData*)> const& event) {
        CustomBeatmapLevelLoader::AddBeatmapDataLoadedEvent(event);
    }

    void DeleteSong(std::string_view path, std::function<void()> const& finished) {
        SongLoader::GetInstance()->DeleteSong(path, finished);
    }

    std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLoadedSongs() {
        return SongLoader::GetInstance()->GetLoadedLevels();
    }

    bool HasLoadedSongs() {
        return SongLoader::GetInstance()->HasLoaded;
    }

    float GetLoadingProgress()
    {
        auto instance = SongLoader::GetInstance();
        return (float)instance->CurrentFolder / (float)instance->MaxFolders;
    }

    std::optional<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLevelByHash(std::string hash) {
        std::transform(hash.begin(), hash.end(), hash.begin(), toupper);
        for(auto& song : RuntimeSongLoader::API::GetLoadedSongs()) {
            if(song->levelID.operator std::string().ends_with(hash))
                return song;
        }
        return std::nullopt;
    }

    std::optional<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLevelById(std::string_view levelID) {
        for(auto& song : RuntimeSongLoader::API::GetLoadedSongs()) {
            if(song->levelID.operator std::string() == levelID)
                return song;
        }
        return std::nullopt;
    }

    std::string GetCustomLevelsPrefix() {
        return CustomLevelPrefixID;
    }
    
    std::string GetCustomLevelPacksPrefix() {
        return CustomLevelPackPrefixID;
    }

    std::string GetCustomLevelsPath() {
        return GetBaseLevelsPath() + CustomLevelsFolder + "/";
    }

    std::string GetCustomWIPLevelsPath() {
        return GetBaseLevelsPath() + CustomWIPLevelsFolder + "/";
    }
}