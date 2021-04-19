#pragma once
#include "GlobalNamespace/CustomLevelLoader.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/CachedMediaAsyncLoader.hpp"
#include "GlobalNamespace/SimpleDialogPromptViewController.hpp"
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
#include "HMUI/ScreenSystem.hpp"

namespace RuntimeSongLoader::FindComponentsUtils {

    #define CacheFindComponentDeclare(namespace, name) namespace::name* Get##name();

    CacheFindComponentDeclare(GlobalNamespace, CustomLevelLoader)
    CacheFindComponentDeclare(GlobalNamespace, BeatmapLevelsModel)
    CacheFindComponentDeclare(GlobalNamespace, CachedMediaAsyncLoader)
    CacheFindComponentDeclare(GlobalNamespace, SimpleDialogPromptViewController)
    CacheFindComponentDeclare(GlobalNamespace, LevelSelectionNavigationController)
    CacheFindComponentDeclare(HMUI, ScreenSystem)

    void ClearCache();

}