#pragma once
#include "beatsaber-hook/shared/utils/typedefs.h"

#include "custom-types/shared/macros.hpp"
#include "GlobalNamespace/BeatmapLevelPack.hpp"
#include "GlobalNamespace/BeatmapLevelsRepository.hpp"

DECLARE_CLASS_CODEGEN(RuntimeSongLoader, SongLoaderBeatmapLevelsRepository, GlobalNamespace::BeatmapLevelsRepository,

    private:
        DECLARE_INSTANCE_FIELD(ListW<GlobalNamespace::BeatmapLevelPack*>, customBeatmapLevelPacks);

    public:
        static SongLoaderBeatmapLevelsRepository* CreateNew();
        void AddLevelPack(GlobalNamespace::BeatmapLevelPack* pack);
        void RemoveLevelPack(GlobalNamespace::BeatmapLevelPack* pack);
        void ClearLevelPacks();
        void UpdateArray();

    DECLARE_CTOR(ctor);

)
