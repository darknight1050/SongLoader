#include "API.hpp"

#include "Paths.hpp"

#include "CustomTypes/SongLoader.hpp"
#include "CustomBeatmapLevelLoader.hpp"

namespace RuntimeSongLoader::API {

    void RefreshSongs() {
        RefreshSongs(true, nullptr);
    }

    void RefreshSongs(bool fullRefresh, std::function<void(const std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*>&)> songsLoaded) {
        SongLoader::GetInstance()->RefreshSongs(fullRefresh, songsLoaded);
    }

    void AddSongsLoadedEvent(std::function<void(const std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*>&)> event) {
        SongLoader::AddSongsLoadedEvent(event);
    }

    void AddRefreshLevelPacksEvent(std::function<void(SongLoaderBeatmapLevelPackCollectionSO*)> event) {
        SongLoader::AddRefreshLevelPacksEvent(event);
    }

    void AddBeatmapDataLoadedEvent(std::function<void(GlobalNamespace::StandardLevelInfoSaveData*, GlobalNamespace::BeatmapData*)> event) {
        CustomBeatmapLevelLoader::AddBeatmapDataLoadedEvent(event);
    }

    void DeleteSong(std::string path, std::function<void()> finished) {
        SongLoader::GetInstance()->DeleteSong(path, finished);
    }

    std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLoadedSongs() {
        return SongLoader::GetInstance()->GetLoadedLevels();
    }

    std::optional<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLevelByHash(std::string hash) {
        std::transform(hash.begin(), hash.end(), hash.begin(), toupper);
        for(auto& song : RuntimeSongLoader::API::GetLoadedSongs()) {
            if(to_utf8(csstrtostr(song->levelID)).ends_with(hash))
                return song;
        }
        return std::nullopt;
    }

    std::optional<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLevelById(std::string levelID) {
        for(auto& song : RuntimeSongLoader::API::GetLoadedSongs()) {
            if(to_utf8(csstrtostr(song->levelID)) == levelID)
                return song;
        }
        return std::nullopt;
    }

    std::string GetCustomLevelsPrefix() {
        return CustomLevelPrefixID;
    }

    std::string GetCustomLevelsPath() {
        return GetBaseLevelsPath() + CustomLevelsFolder + "/";
    }

    std::string GetCustomWIPLevelsPath() {
        return GetBaseLevelsPath() + CustomWIPLevelsFolder + "/";
    }
}