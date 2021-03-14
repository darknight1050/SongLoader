#pragma once 
#include "beatsaber-hook/shared/utils/typedefs.h"

#include "custom-types/shared/macros.hpp" 
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp" 
#include "GlobalNamespace/BeatmapLevelPackCollectionSO.hpp" 
 
DECLARE_CLASS_CODEGEN(RuntimeSongLoader, SongLoaderBeatmapLevelPackCollectionSO, GlobalNamespace::BeatmapLevelPackCollectionSO, 
 
    private:
        List<GlobalNamespace::CustomBeatmapLevelPack*>* customBeatmapLevelPacks;

    public:
        static SongLoaderBeatmapLevelPackCollectionSO* CreateNew();
        void AddLevelPack(GlobalNamespace::CustomBeatmapLevelPack* pack);
        void RemoveLevelPack(GlobalNamespace::CustomBeatmapLevelPack* pack);
        void UpdateArray();
 
    DECLARE_CTOR(ctor);

    REGISTER_FUNCTION(SongLoaderBeatmapLevelPackCollectionSO,
        REGISTER_METHOD(ctor);
    )
)