#pragma once

#include "custom-types/shared/macros.hpp"
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollectionSO.hpp"

DECLARE_CLASS_CODEGEN(SongLoader, SongCoreBeatmapLevelPackCollectionSO, GlobalNamespace::BeatmapLevelPackCollectionSO,

    DECLARE_INSTANCE_FIELD(Array<CustomBeatmapLevelPack>*, customBeatmapLevelPacks);

    DECLARE_CTOR(ctor);

    DECLARE_STATIC_METHOD(CreateNew, SongCoreBeatmapLevelPackCollectionSO);
    DECLARE_INSTANCE_METHOD(AddLevelPack, CustomBeatmapLevelPack);

    REGISTER_FUNCTION(SongCoreBeatmapLevelPackCollectionSO,
        REGISTER_FIELD(customBeatmapLevelPacks);

        REGISTER_METHOD(ctor);
    )
)