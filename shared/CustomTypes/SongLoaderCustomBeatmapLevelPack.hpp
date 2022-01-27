#pragma once 
#include "beatsaber-hook/shared/utils/typedefs.h"

#include "custom-types/shared/macros.hpp" 

#include "SongLoaderBeatmapLevelPackCollectionSO.hpp"

#include "GlobalNamespace/CustomBeatmapLevelPack.hpp" 
#include "GlobalNamespace/CustomBeatmapLevelCollection.hpp" 
#include "UnityEngine/Sprite.hpp" 
 
DECLARE_CLASS_CODEGEN(RuntimeSongLoader, SongLoaderCustomBeatmapLevelPack, Il2CppObject,
    
    DECLARE_INSTANCE_FIELD(GlobalNamespace::CustomBeatmapLevelCollection*, CustomLevelsCollection);
    DECLARE_INSTANCE_FIELD(GlobalNamespace::CustomBeatmapLevelPack*, CustomLevelsPack);

    DECLARE_CTOR(ctor, StringW packID, StringW packName, UnityEngine::Sprite* coverImage = nullptr);

    public:
        static SongLoaderCustomBeatmapLevelPack* Make_New(std::string const& packID, std::string_view packName, UnityEngine::Sprite* coverImage = nullptr);

        ArrayW<GlobalNamespace::CustomPreviewBeatmapLevel*> GetCustomPreviewBeatmapLevels();
        void SetCustomPreviewBeatmapLevels(ArrayW<GlobalNamespace::CustomPreviewBeatmapLevel*> customPreviewBeatmapLevels);

        void SortLevels();
        
        void AddTo(SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO, bool addIfEmpty = false);

)