#pragma once
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"

namespace RuntimeSongLoader::FindComponentsUtils {

    #define CacheFindComponentDeclare(namespace, name) namespace::name* Get##name();

    CacheFindComponentDeclare(GlobalNamespace, LevelSelectionNavigationController)

    void ClearCache();

}