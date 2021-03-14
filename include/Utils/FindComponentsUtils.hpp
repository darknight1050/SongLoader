#pragma once
#include "GlobalNamespace/CustomLevelLoader.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/CachedMediaAsyncLoader.hpp"

namespace FindComponentsUtils {

    #define CacheFindComponentDeclare(namespace, name) namespace::name* Get##name();

    CacheFindComponentDeclare(GlobalNamespace, CustomLevelLoader)
    CacheFindComponentDeclare(GlobalNamespace, BeatmapLevelsModel)
    CacheFindComponentDeclare(GlobalNamespace, CachedMediaAsyncLoader)

    void ClearCache();

}