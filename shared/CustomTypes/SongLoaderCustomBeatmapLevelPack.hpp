#pragma once 
#include "beatsaber-hook/shared/utils/typedefs.h"

#include "custom-types/shared/macros.hpp" 

#include "SongLoaderBeatmapLevelPackCollectionSO.hpp"

#include "GlobalNamespace/CustomBeatmapLevelPack.hpp" 
#include "GlobalNamespace/CustomBeatmapLevelCollection.hpp" 
#include "UnityEngine/Sprite.hpp" 
 
DECLARE_CLASS_CODEGEN(RuntimeSongLoader, SongLoaderCustomBeatmapLevelPack, Il2CppObject, 
 
    static SongLoaderCustomBeatmapLevelPack* New_ctor(std::string packID, std::string packName, UnityEngine::Sprite* coverImage = nullptr);

    DECLARE_INSTANCE_FIELD(GlobalNamespace::CustomBeatmapLevelCollection*, CustomLevelsCollection);
    DECLARE_INSTANCE_FIELD(GlobalNamespace::CustomBeatmapLevelPack*, CustomLevelsPack);

    DECLARE_CTOR(ctor, Il2CppString* packID, Il2CppString* packName, UnityEngine::Sprite* coverImage);

    void SortLevels();

    public:
        Array<GlobalNamespace::CustomPreviewBeatmapLevel*>* GetCustomPreviewBeatmapLevels();
        void SetCustomPreviewBeatmapLevels(Array<GlobalNamespace::CustomPreviewBeatmapLevel*>* customPreviewBeatmapLevels);

        void AddTo(SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO, bool addIfEmpty = false);

    REGISTER_FUNCTION(
        REGISTER_FIELD(CustomLevelsCollection);
        REGISTER_FIELD(CustomLevelsPack);

        REGISTER_METHOD(ctor);
    )
)