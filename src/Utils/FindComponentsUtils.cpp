#include "Utils/FindComponentsUtils.hpp"
#include "Utils/ArrayUtil.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "customlogger.hpp"

#include "UnityEngine/Resources.hpp"

using namespace GlobalNamespace;
using namespace UnityEngine;

namespace FindComponentsUtils {

    #define CacheNotFoundWarningLog(type) getLogger().warning("Can't find '" #type "'! (This shouldn't happen and can cause unexpected behaviour)");

    #define CacheFindComponentDefine(name) \
    name* _##name = nullptr; \
    name* Get##name() { \
        if(!_##name) \
            _##name = ArrayUtil::First(Resources::FindObjectsOfTypeAll<name*>()); \
        if(!_##name) \
            CacheNotFoundWarningLog(_##name) \
        return _##name; \
    }
    #define CacheClearComponent(name) _##name = nullptr;

    CacheFindComponentDefine(CustomLevelLoader)
    CacheFindComponentDefine(BeatmapLevelsModel)
    CacheFindComponentDefine(CachedMediaAsyncLoader)
    CacheFindComponentDefine(AlwaysOwnedContentContainerSO)

    void ClearCache() {
        CacheClearComponent(CustomLevelLoader)
        CacheClearComponent(BeatmapLevelsModel)
        CacheClearComponent(CachedMediaAsyncLoader)
        CacheClearComponent(AlwaysOwnedContentContainerSO)
    }
 
}