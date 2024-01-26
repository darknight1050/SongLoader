#include "Utils/FindComponentsUtils.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "CustomLogger.hpp"

#include "UnityEngine/Resources.hpp"

using namespace GlobalNamespace;
using namespace HMUI;
using namespace UnityEngine;

namespace RuntimeSongLoader::FindComponentsUtils {

    #define CacheNotFoundWarningLog(type) getLogger().warning("Can't find '" #type "'! (This shouldn't happen and can cause unexpected behaviour)");

    #define CacheFindComponentDefineFirst(name) \
    name* _##name = nullptr; \
    name* Get##name() { \
        if(!_##name) \
            _##name = Resources::FindObjectsOfTypeAll<name*>()->FirstOrDefault(); \
        if(!_##name) \
            CacheNotFoundWarningLog(_##name) \
        return _##name; \
    }
    #define CacheFindComponentDefineLast(name) \
    name* _##name = nullptr; \
    name* Get##name() { \
        if(!_##name) \
            _##name = Resources::FindObjectsOfTypeAll<name*>()->LastOrDefault(); \
        if(!_##name) \
            CacheNotFoundWarningLog(_##name) \
        return _##name; \
    }
    #define CacheClearComponent(name) _##name = nullptr;

    CacheFindComponentDefineFirst(CustomLevelLoader)
    CacheFindComponentDefineFirst(BeatmapLevelsModel)
    CacheFindComponentDefineFirst(CachedMediaAsyncLoader)
    CacheFindComponentDefineLast(SimpleDialogPromptViewController)
    CacheFindComponentDefineFirst(LevelSelectionNavigationController)

    void ClearCache() {
        CacheClearComponent(CustomLevelLoader)
        CacheClearComponent(BeatmapLevelsModel)
        CacheClearComponent(CachedMediaAsyncLoader)
        CacheClearComponent(SimpleDialogPromptViewController)
        CacheClearComponent(LevelSelectionNavigationController)
    }

}
