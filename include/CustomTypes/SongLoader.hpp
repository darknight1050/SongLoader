#pragma once 
#include "beatsaber-hook/shared/utils/typedefs.h"

#include "custom-types/shared/macros.hpp" 

#include "SongLoaderBeatmapLevelPackCollectionSO.hpp" 
#include "GlobalNamespace/CustomBeatmapLevelCollection.hpp" 
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp" 
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp" 
#include "GlobalNamespace/EnvironmentInfoSO.hpp" 
#include "UnityEngine/MonoBehaviour.hpp" 

DECLARE_CLASS_CODEGEN(RuntimeSongLoader, SongLoader, UnityEngine::MonoBehaviour, 
 
    private:
        static SongLoader* Instance;
        
        GlobalNamespace::CustomBeatmapLevelCollection* CustomLevelsCollection = nullptr;
        GlobalNamespace::CustomBeatmapLevelCollection* CustomWIPLevelsCollection = nullptr;

        GlobalNamespace::CustomBeatmapLevelPack* CustomLevelsPack = nullptr;
        GlobalNamespace::CustomBeatmapLevelPack* CustomWIPLevelsPack = nullptr;

        SongLoaderBeatmapLevelPackCollectionSO* CustomBeatmapLevelPackCollectionSO = nullptr;

        bool NeedRefresh = false;

        GlobalNamespace::StandardLevelInfoSaveData* GetStandardLevelInfoSaveData(std::string customLevelPath);
        GlobalNamespace::EnvironmentInfoSO* LoadEnvironmentInfo(Il2CppString* environmentName, bool allDirections);
        GlobalNamespace::CustomPreviewBeatmapLevel* LoadCustomPreviewBeatmapLevel(std::string customLevelPath, GlobalNamespace::StandardLevelInfoSaveData* standardLevelInfoSaveData, std::string& outHash);

        List<GlobalNamespace::CustomPreviewBeatmapLevel*>* LoadSongsFromPath(std::string path);

    public:
        static SongLoader* GetInstance();
        void RefreshLevelPacks();
        
        void RefreshSongs(bool fullRefresh = false);
        
    DECLARE_CTOR(ctor);
    DECLARE_METHOD(void, Update);

    REGISTER_FUNCTION(SongLoader,
        REGISTER_METHOD(ctor);
        REGISTER_METHOD(Update);
    )
)