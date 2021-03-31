#pragma once 
#include "beatsaber-hook/shared/utils/typedefs.h"

#include "custom-types/shared/macros.hpp" 

#include "SongLoaderBeatmapLevelPackCollectionSO.hpp" 
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp" 
#include "GlobalNamespace/CustomBeatmapLevelCollection.hpp" 
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp" 
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp" 
#include "GlobalNamespace/EnvironmentInfoSO.hpp" 
#include "GlobalNamespace/BeatmapDataLoader.hpp" 
#include "UnityEngine/MonoBehaviour.hpp" 
#include "System/Collections/Generic/Dictionary_2.hpp"

#include <vector>

namespace RuntimeSongLoader {
    using DictionaryType = ::System::Collections::Generic::Dictionary_2<Il2CppString*, ::GlobalNamespace::CustomPreviewBeatmapLevel*>*;
}

DECLARE_CLASS_CODEGEN(RuntimeSongLoader, SongLoader, UnityEngine::MonoBehaviour, 
    private:
        static SongLoader* Instance;

        std::vector<std::function<void(const std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*>&)>> LoadedEvents;

        DECLARE_INSTANCE_FIELD_DEFAULT(DictionaryType, CustomLevels, nullptr);
        DECLARE_INSTANCE_FIELD_DEFAULT(DictionaryType, CustomWIPLevels, nullptr);

        DECLARE_INSTANCE_FIELD_DEFAULT(GlobalNamespace::BeatmapDataLoader*, beatmapDataLoader, nullptr);

        DECLARE_INSTANCE_FIELD_DEFAULT(GlobalNamespace::CustomBeatmapLevelCollection*, CustomLevelsCollection, nullptr);
        DECLARE_INSTANCE_FIELD_DEFAULT(GlobalNamespace::CustomBeatmapLevelCollection*, CustomWIPLevelsCollection, nullptr);

        DECLARE_INSTANCE_FIELD_DEFAULT(GlobalNamespace::CustomBeatmapLevelPack*, CustomLevelsPack, nullptr);
        DECLARE_INSTANCE_FIELD_DEFAULT(GlobalNamespace::CustomBeatmapLevelPack*, CustomWIPLevelsPack, nullptr);

        DECLARE_INSTANCE_FIELD_DEFAULT(SongLoaderBeatmapLevelPackCollectionSO*, CustomBeatmapLevelPackCollectionSO, nullptr);

        DECLARE_INSTANCE_FIELD_DEFAULT(bool, NeedsRefresh, false);

        DECLARE_INSTANCE_FIELD_DEFAULT(bool, IsLoading, false);
        DECLARE_INSTANCE_FIELD_DEFAULT(bool, HasLoaded, false);

        DECLARE_INSTANCE_FIELD_DEFAULT(bool, LoadingCancelled, false); //TODO: Implement this

        DECLARE_INSTANCE_FIELD_DEFAULT(int, MaxFolders, 0);
        DECLARE_INSTANCE_FIELD_DEFAULT(int, CurrentFolder, 0);

        void MenuLoaded();

        GlobalNamespace::StandardLevelInfoSaveData* GetStandardLevelInfoSaveData(const std::string& customLevelPath);
        GlobalNamespace::EnvironmentInfoSO* LoadEnvironmentInfo(Il2CppString* environmentName, bool allDirections);
        GlobalNamespace::CustomPreviewBeatmapLevel* LoadCustomPreviewBeatmapLevel(const std::string& customLevelPath, bool wip, GlobalNamespace::StandardLevelInfoSaveData* standardLevelInfoSaveData, std::string& outHash);
        
        void UpdateSongDuration(GlobalNamespace::CustomPreviewBeatmapLevel* level, const std::string& customLevelPath);
        float GetLengthFromMap(GlobalNamespace::CustomPreviewBeatmapLevel* level, const std::string& customLevelPath);

        List<GlobalNamespace::CustomPreviewBeatmapLevel*>* LoadSongsFromPath(std::string_view path, std::vector<std::string>& loadedPaths);

    public:
        static SongLoader* GetInstance();

        void AddSongsLoadedEvent(std::function<void(const std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*>&)> event);

        void RefreshLevelPacks();
        
        void RefreshSongs(bool fullRefresh = true);
        
        DECLARE_CTOR(ctor);
        DECLARE_METHOD(void, Finalize);
        DECLARE_METHOD(void, Awake);
        DECLARE_METHOD(void, Update);

    REGISTER_FUNCTION(SongLoader,
        REGISTER_FIELD(beatmapDataLoader);

        REGISTER_FIELD(CustomLevelsCollection);
        REGISTER_FIELD(CustomWIPLevelsCollection);

        REGISTER_FIELD(CustomLevelsPack);
        REGISTER_FIELD(CustomWIPLevelsPack);

        REGISTER_FIELD(CustomBeatmapLevelPackCollectionSO);

        REGISTER_FIELD(NeedsRefresh);

        REGISTER_FIELD(IsLoading);
        REGISTER_FIELD(HasLoaded);
        REGISTER_FIELD(LoadingCancelled);

        REGISTER_FIELD(MaxFolders);
        REGISTER_FIELD(CurrentFolder);

        REGISTER_METHOD(ctor);
        REGISTER_METHOD(Finalize);
        REGISTER_METHOD(Awake);
        REGISTER_METHOD(Update);
    )
)