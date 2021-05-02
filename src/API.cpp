#include "API.hpp"

#include "Paths.hpp"

#include "CustomTypes/SongLoader.hpp"

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

    std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLoadedSongs() {
        return SongLoader::GetInstance()->GetLoadedLevels();
    }

    void DeleteSong(std::string path, std::function<void()> finished) {
        SongLoader::GetInstance()->DeleteSong(path, finished);
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