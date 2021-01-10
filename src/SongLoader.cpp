#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"

#include "UnityEngine/Resources.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/CancellationTokenSource.hpp"

#include "customlogger.hpp"
#include <unistd.h>
#include <chrono>

static ModInfo modInfo;

const Logger& getLogger() {
    static const Logger logger(modInfo, LoggerOptions(false, false));
    return logger;
}

using namespace UnityEngine;
using namespace GlobalNamespace;
using namespace System::Threading;
using namespace System::Threading::Tasks;

MAKE_HOOK_OFFSETLESS(BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync, Task_1<IBeatmapLevelPackCollection*>*, BeatmapLevelsModel* self, CancellationToken cancellationToken) {
    return BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync(self, cancellationToken);
}

MAKE_HOOK_OFFSETLESS(LevelFilteringNavigationController_SetupBeatmapLevelPacks, void, LevelFilteringNavigationController* self) {
    self->enableCustomLevels = true;
    LevelFilteringNavigationController_SetupBeatmapLevelPacks(self);
}

extern "C" void setup(ModInfo& info) {
    modInfo.id = "SongLoader";
    modInfo.version = VERSION;
    info = modInfo;
}

extern "C" void load() {
    LOG_INFO("Starting SongLoader installation...");
    il2cpp_functions::Init();
    INSTALL_HOOK_OFFSETLESS(BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "ReloadCustomLevelPackCollectionAsync", 1));
    INSTALL_HOOK_OFFSETLESS(LevelFilteringNavigationController_SetupBeatmapLevelPacks, il2cpp_utils::FindMethodUnsafe("", "LevelFilteringNavigationController", "SetupBeatmapLevelPacks", 0));
    LOG_INFO("Successfully installed SongLoader!");
}