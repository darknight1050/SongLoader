#pragma once 
#include "beatsaber-hook/shared/utils/typedefs.h"

#include "custom-types/shared/macros.hpp" 

#include "SongLoaderBeatmapLevelsRepository.hpp"

#include "GlobalNamespace/BeatmapLevelPack.hpp"
#include "GlobalNamespace/BeatmapLevel.hpp"
#include "UnityEngine/Sprite.hpp" 
 
DECLARE_CLASS_CODEGEN(RuntimeSongLoader, SongLoaderCustomBeatmapLevelPack, Il2CppObject,
    
    DECLARE_INSTANCE_FIELD(ArrayW<GlobalNamespace::BeatmapLevel*>, CustomLevelsCollection);
    DECLARE_INSTANCE_FIELD(GlobalNamespace::BeatmapLevelPack*, CustomLevelsPack);

    DECLARE_CTOR(ctor, StringW packID, StringW packName, UnityEngine::Sprite* coverImage = nullptr);

    public:
        static SongLoaderCustomBeatmapLevelPack* Make_New(std::string const& packID, std::string_view packName, UnityEngine::Sprite* coverImage = nullptr);

        ArrayW<GlobalNamespace::BeatmapLevel*> GetCustomBeatmapLevels();
        void SetCustomBeatmapLevels(ArrayW<GlobalNamespace::BeatmapLevel*> customBeatmapLevels);

        void SortLevels();
        
        void AddTo(SongLoaderBeatmapLevelsRepository* customBeatmapLevelsRepository, bool addIfEmpty = false);

)