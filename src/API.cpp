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

    std::optional<std::string> GetLevelPathByHash(std::string hash) {
        try {
            auto levelData = SongLoader::GetInstance()->LevelDatas->get_Item(hash);
            auto fileSystemLevelData = reinterpret_cast<GlobalNamespace::FileSystemBeatmapLevelData*>(levelData);
            std::string audioPath = fileSystemLevelData->_audioClipPath;
            auto directory = audioPath.substr(0, audioPath.find_last_of('/'));
            return directory;
        } catch(...) {
            return std::nullopt;
        }
    }

    std::optional<CustomJSONData::CustomLevelInfoSaveData*> GetCustomSaveDataByHash(std::string hash) {
        try {
            auto saveData = SongLoader::GetInstance()->LevelSaveDatas->get_Item(hash);
            return saveData;
        } catch(...) {
            return std::nullopt;
        }
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