#pragma once 
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "beatsaber-hook/shared/utils/gc-alloc.hpp"

#include "custom-types/shared/macros.hpp" 

#include "CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp" 
#include "CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"
#include "CustomTypes/CustomLevelInfoSaveData.hpp"

#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp" 
#include "GlobalNamespace/CustomBeatmapLevelCollection.hpp" 
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp" 
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp" 
#include "GlobalNamespace/EnvironmentInfoSO.hpp" 
#include "GlobalNamespace/BeatmapDataLoader.hpp" 
#include "UnityEngine/MonoBehaviour.hpp" 
#include "System/Collections/Generic/Dictionary_2.hpp"

#include <vector>
#include <string_view>
#include <unordered_map>

namespace RuntimeSongLoader {
    using DictionaryType = std::unordered_map<const std::string_view, ::GlobalNamespace::CustomPreviewBeatmapLevel *, 
        std::hash<std::string_view>, 
        std::equal_to<const std::string_view>, 
        gc_allocator<std::pair<const std::string_view, ::GlobalNamespace::CustomPreviewBeatmapLevel *>>
    >;
}

DECLARE_CLASS_CODEGEN(RuntimeSongLoader, SongLoader, UnityEngine::MonoBehaviour,
    private:
        static SongLoader* Instance;

        static std::vector<std::function<void(std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> const&)>> LoadedEvents;
        static std::mutex LoadedEventsMutex;

        static std::vector<std::function<void(SongLoaderBeatmapLevelPackCollectionSO*)>> RefreshLevelPacksEvents;
        static std::mutex RefreshLevelPacksEventsMutex;
        
        static std::vector<std::function<void()>> SongDeletedEvents;
        static std::mutex SongDeletedEventsMutex;

        std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> LoadedLevels;

        std::optional<bool> queuedRefresh = std::nullopt;
        std::function<void(std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> const&)> queuedCallback = nullptr;

        CustomJSONData::CustomLevelInfoSaveData* GetStandardLevelInfoSaveData(std::string const& customLevelPath);
        GlobalNamespace::EnvironmentInfoSO* LoadEnvironmentInfo(StringW environmentName, bool allDirections);
        GlobalNamespace::CustomPreviewBeatmapLevel* LoadCustomPreviewBeatmapLevel(std::string const& customLevelPath, bool wip, CustomJSONData::CustomLevelInfoSaveData* standardLevelInfoSaveData, std::string& outHash);
        
        void UpdateSongDuration(GlobalNamespace::CustomPreviewBeatmapLevel* level, std::string const& customLevelPath);
        float GetLengthFromMap(GlobalNamespace::CustomPreviewBeatmapLevel* level, std::string const& customLevelPath);

        List<GlobalNamespace::CustomPreviewBeatmapLevel*>* LoadSongsFromPath(std::string_view path, std::vector<std::string>& loadedPaths);

        DictionaryType CustomLevels;
        DictionaryType CustomWIPLevels;

        DECLARE_INSTANCE_FIELD(GlobalNamespace::BeatmapDataLoader*, beatmapDataLoader);

        DECLARE_INSTANCE_FIELD(SongLoaderCustomBeatmapLevelPack*, CustomLevelsPack);
        DECLARE_INSTANCE_FIELD(SongLoaderCustomBeatmapLevelPack*, CustomWIPLevelsPack);

        DECLARE_INSTANCE_FIELD(SongLoaderBeatmapLevelPackCollectionSO*, CustomBeatmapLevelPackCollectionSO);

        DECLARE_INSTANCE_FIELD(bool, IsLoading);
        DECLARE_INSTANCE_FIELD(bool, HasLoaded);

        DECLARE_INSTANCE_FIELD(bool, LoadingCancelled); //TODO: Implement this

        DECLARE_INSTANCE_FIELD(int, MaxFolders);
        DECLARE_INSTANCE_FIELD(int, CurrentFolder);

    public:
        static SongLoader* GetInstance();

        std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> GetLoadedLevels();

        static void AddSongsLoadedEvent(std::function<void(std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> const&)> const& event) {
            std::lock_guard<std::mutex> lock(LoadedEventsMutex);
            LoadedEvents.push_back(event);
        }

        static void AddRefreshLevelPacksEvent(std::function<void(SongLoaderBeatmapLevelPackCollectionSO*)> const& event) {
            std::lock_guard<std::mutex> lock(RefreshLevelPacksEventsMutex);
            RefreshLevelPacksEvents.push_back(event);
        }

        static void AddSongDeletedEvent(std::function<void()> const& event) {
            std::lock_guard<std::mutex> lock(SongDeletedEventsMutex);
            SongDeletedEvents.push_back(event);
        }

        void RefreshLevelPacks(bool includeDefault) const;
        
        void RefreshSongs(bool fullRefresh, std::function<void(std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> const&)> const& songsLoaded = nullptr);

        void DeleteSong(std::string_view path, std::function<void()> const& finished);

        void MenuLoaded();
        
        DECLARE_CTOR(ctor);
        DECLARE_SIMPLE_DTOR();

        DECLARE_INSTANCE_METHOD(void, Awake);
        DECLARE_INSTANCE_METHOD(void, Update);
)