#pragma once 
#include "beatsaber-hook/shared/utils/typedefs.h"

#include "custom-types/shared/macros.hpp" 

#include "SongLoaderBeatmapLevelPackCollectionSO.hpp" 
#include "GlobalNamespace/CustomBeatmapLevelCollection.hpp" 
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp" 
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp" 
#include "GlobalNamespace/EnvironmentInfoSO.hpp" 
#include "GlobalNamespace/BeatmapDataLoader.hpp" 
#include "UnityEngine/MonoBehaviour.hpp" 

DECLARE_CLASS_CODEGEN(RuntimeSongLoader, SongLoader, UnityEngine::MonoBehaviour, 
 
    private:
        static SongLoader* Instance;

        DECLARE_INSTANCE_FIELD_DEFAULT(GlobalNamespace::BeatmapDataLoader*, beatmapDataLoader, nullptr);

        DECLARE_INSTANCE_FIELD_DEFAULT(GlobalNamespace::CustomBeatmapLevelCollection*, CustomLevelsCollection, nullptr);
        DECLARE_INSTANCE_FIELD_DEFAULT(GlobalNamespace::CustomBeatmapLevelCollection*, CustomWIPLevelsCollection, nullptr);

        DECLARE_INSTANCE_FIELD_DEFAULT(GlobalNamespace::CustomBeatmapLevelPack*, CustomLevelsPack, nullptr);
        DECLARE_INSTANCE_FIELD_DEFAULT(GlobalNamespace::CustomBeatmapLevelPack*, CustomWIPLevelsPack, nullptr);

        DECLARE_INSTANCE_FIELD_DEFAULT(SongLoaderBeatmapLevelPackCollectionSO*, CustomBeatmapLevelPackCollectionSO, nullptr);

        DECLARE_INSTANCE_FIELD_DEFAULT(bool, NeedRefresh, false);

        DECLARE_INSTANCE_FIELD_DEFAULT(bool, IsLoading, false);

        DECLARE_INSTANCE_FIELD_DEFAULT(int, MaxFolders, 0);
        DECLARE_INSTANCE_FIELD_DEFAULT(int, CurrentFolder, 0);

        GlobalNamespace::StandardLevelInfoSaveData* GetStandardLevelInfoSaveData(const std::string& customLevelPath);
        GlobalNamespace::EnvironmentInfoSO* LoadEnvironmentInfo(Il2CppString* environmentName, bool allDirections);
        GlobalNamespace::CustomPreviewBeatmapLevel* LoadCustomPreviewBeatmapLevel(const std::string& customLevelPath, GlobalNamespace::StandardLevelInfoSaveData* standardLevelInfoSaveData, std::string& outHash);
        
        void UpdateSongDuration(GlobalNamespace::CustomPreviewBeatmapLevel* level, const std::string& customLevelPath);
        float GetLengthFromMap(GlobalNamespace::CustomPreviewBeatmapLevel* level, const std::string& customLevelPath);

        List<GlobalNamespace::CustomPreviewBeatmapLevel*>* LoadSongsFromPath(std::string_view path, std::vector<std::string>& loadedPaths);

    public:
        static SongLoader* GetInstance();
        void RefreshLevelPacks();
        
        void RefreshSongs(bool fullRefresh = false);
        
    DECLARE_CTOR(ctor);
    DECLARE_METHOD(void, Update);

    REGISTER_FUNCTION(SongLoader,
        REGISTER_FIELD(beatmapDataLoader);

        REGISTER_FIELD(CustomLevelsCollection);
        REGISTER_FIELD(CustomWIPLevelsCollection);

        REGISTER_FIELD(CustomLevelsPack);
        REGISTER_FIELD(CustomWIPLevelsPack);

        REGISTER_FIELD(CustomBeatmapLevelPackCollectionSO);

        REGISTER_FIELD(NeedRefresh);

        REGISTER_FIELD(IsLoading);

        REGISTER_FIELD(MaxFolders);
        REGISTER_FIELD(CurrentFolder);

        REGISTER_METHOD(ctor);
        REGISTER_METHOD(Update);
    )
)