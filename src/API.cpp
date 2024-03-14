#include "API.hpp"

#include "Paths.hpp"

#include "CustomTypes/SongLoader.hpp"

namespace RuntimeSongLoader::API {

    void RefreshSongs() {
        RefreshSongs(true, nullptr);
    }

    void RefreshSongs(bool fullRefresh, std::function<void(std::vector<GlobalNamespace::BeatmapLevel*> const&)> const& songsLoaded) {
        SongLoader::GetInstance()->RefreshSongs(fullRefresh, songsLoaded);
    }

    void RefreshPacks(bool includeDefault) {
        SongLoader::GetInstance()->RefreshLevelPacks(includeDefault);
    }

    void AddSongsLoadedEvent(std::function<void(std::vector<GlobalNamespace::BeatmapLevel*> const&)> const& event) {
        SongLoader::AddSongsLoadedEvent(event);
    }

    void AddRefreshLevelPacksEvent(std::function<void(SongLoaderBeatmapLevelsRepository*)> const& event) {
        SongLoader::AddRefreshLevelPacksEvent(event);
    }

    void AddSongDeletedEvent(std::function<void()> const& event) {
        SongLoader::AddSongDeletedEvent(event);
    }

    void DeleteSong(std::string_view path, std::function<void()> const& finished) {
        SongLoader::GetInstance()->DeleteSong(path, finished);
    }

    std::vector<GlobalNamespace::BeatmapLevel*> GetLoadedSongs() {
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

    std::optional<GlobalNamespace::BeatmapLevel*> GetLevelByHash(std::string hash) {
        std::transform(hash.begin(), hash.end(), hash.begin(), toupper);
        for(auto& song : RuntimeSongLoader::API::GetLoadedSongs()) {
            if(song->levelID.ends_with(hash))
                return song;
        }
        return std::nullopt;
    }

    std::optional<GlobalNamespace::BeatmapLevel*> GetLevelById(std::string_view levelID) {
        for(auto& song : RuntimeSongLoader::API::GetLoadedSongs()) {
            if(song->levelID == levelID)
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

    RuntimeSongLoader::SongLoaderCustomBeatmapLevelPack* GetCustomLevelsPack(){
        return SongLoader::GetInstance()->CustomLevelsPack;
    };

    RuntimeSongLoader::SongLoaderCustomBeatmapLevelPack* GetCustomWIPLevelsPack() {
        return SongLoader::GetInstance()->CustomWIPLevelsPack;
    };
}