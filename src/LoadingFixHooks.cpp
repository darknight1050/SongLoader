#include "LoadingFixHooks.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "CustomLogger.hpp"
#include "Paths.hpp"

#include "GlobalNamespace/AdditionalContentModel.hpp"
#include "GlobalNamespace/SinglePlayerLevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/IBeatmapLevelPack.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollection.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollectionSO.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollectionContainerSO.hpp"
#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Linq/Enumerable.hpp"

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace System::Collections::Generic;
using namespace System::Threading;
using namespace Tasks;

namespace LoadingFixHooks {

    MAKE_HOOK_OFFSETLESS(BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync, Task_1<IBeatmapLevelPackCollection*>*, BeatmapLevelsModel* self, CancellationToken cancellationToken) {
        LOG_DEBUG("BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync");
        return Task_1<IBeatmapLevelPackCollection*>::New_ctor(self->customLevelPackCollection);
    }

    MAKE_HOOK_OFFSETLESS(BeatmapLevelsModel_UpdateAllLoadedBeatmapLevelPacks, void, BeatmapLevelsModel* self) {
        LOG_DEBUG("BeatmapLevelsModel_UpdateAllLoadedBeatmapLevelPacks Start");
        List<IBeatmapLevelPack*>* list = List<IBeatmapLevelPack*>::New_ctor();
        if(self->ostAndExtrasPackCollection)
            list->AddRange(reinterpret_cast<IEnumerable_1<IBeatmapLevelPack*>*>(self->ostAndExtrasPackCollection->get_beatmapLevelPacks_NEW()));
        if(self->dlcLevelPackCollectionContainer && self->dlcLevelPackCollectionContainer->beatmapLevelPackCollection)
            list->AddRange(reinterpret_cast<IEnumerable_1<IBeatmapLevelPack*>*>(self->dlcLevelPackCollectionContainer->beatmapLevelPackCollection->get_beatmapLevelPacks_NEW()));
        self->allLoadedBeatmapLevelWithoutCustomLevelPackCollection = reinterpret_cast<IBeatmapLevelPackCollection*>(BeatmapLevelPackCollection::New_ctor(list->ToArray()));
        if (self->customLevelPackCollection)
            list->AddRange(reinterpret_cast<IEnumerable_1<IBeatmapLevelPack*>*>(self->customLevelPackCollection->get_beatmapLevelPacks_NEW()));
        self->allLoadedBeatmapLevelPackCollection = reinterpret_cast<IBeatmapLevelPackCollection*>(BeatmapLevelPackCollection::New_ctor(list->ToArray()));
        LOG_DEBUG("BeatmapLevelsModel_UpdateAllLoadedBeatmapLevelPacks Stop");
    }

    MAKE_HOOK_OFFSETLESS(AdditionalContentModel_GetLevelEntitlementStatusAsync, Task_1<AdditionalContentModel::EntitlementStatus>*, AdditionalContentModel* self, Il2CppString* levelID, CancellationToken cancellationToken) {
        LOG_DEBUG("AdditionalContentModel_GetLevelEntitlementStatusAsync Start %s", to_utf8(csstrtostr(levelID)).c_str());
        if(to_utf8(csstrtostr(levelID)).starts_with(CustomLevelPrefixID))
            return Task_1<AdditionalContentModel::EntitlementStatus>::New_ctor(AdditionalContentModel::EntitlementStatus::Owned);
        return AdditionalContentModel_GetLevelEntitlementStatusAsync(self, levelID, cancellationToken);
    }

    MAKE_HOOK_OFFSETLESS(AdditionalContentModel_GetPackEntitlementStatusAsync, Task_1<AdditionalContentModel::EntitlementStatus>*, AdditionalContentModel* self, Il2CppString* levelPackId, CancellationToken cancellationToken) {
        LOG_DEBUG("AdditionalContentModel_GetPackEntitlementStatusAsync Start %s", to_utf8(csstrtostr(levelPackId)).c_str());
        if(to_utf8(csstrtostr(levelPackId)).starts_with(CustomLevelPackPrefixID))
            return Task_1<AdditionalContentModel::EntitlementStatus>::New_ctor(AdditionalContentModel::EntitlementStatus::Owned);
        return AdditionalContentModel_GetPackEntitlementStatusAsync(self, levelPackId, cancellationToken);
    }

    MAKE_HOOK_OFFSETLESS(SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels, bool, SinglePlayerLevelSelectionFlowCoordinator* self) {
        LOG_DEBUG("SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels");
        return true;
    }

    MAKE_HOOK_OFFSETLESS(FileHelpers_GetEscapedURLForFilePath, Il2CppString*, Il2CppString* filePath) {
        LOG_DEBUG("FileHelpers_GetEscapedURLForFilePath");
        return il2cpp_utils::createcsstr("file:///" + to_utf8(csstrtostr(filePath)));
    }

    void InstallHooks() {
        INSTALL_HOOK_OFFSETLESS(getLogger(), BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "ReloadCustomLevelPackCollectionAsync", 1));
        INSTALL_HOOK_OFFSETLESS(getLogger(), BeatmapLevelsModel_UpdateAllLoadedBeatmapLevelPacks, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "UpdateAllLoadedBeatmapLevelPacks", 0));
        INSTALL_HOOK_OFFSETLESS(getLogger(), AdditionalContentModel_GetLevelEntitlementStatusAsync, il2cpp_utils::FindMethodUnsafe("", "AdditionalContentModel", "GetLevelEntitlementStatusAsync", 2));
        INSTALL_HOOK_OFFSETLESS(getLogger(), AdditionalContentModel_GetPackEntitlementStatusAsync, il2cpp_utils::FindMethodUnsafe("", "AdditionalContentModel", "GetPackEntitlementStatusAsync", 2));
        INSTALL_HOOK_OFFSETLESS(getLogger(), SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels, il2cpp_utils::FindMethodUnsafe("", "SinglePlayerLevelSelectionFlowCoordinator", "get_enableCustomLevels", 0)); 
        INSTALL_HOOK_OFFSETLESS(getLogger(), FileHelpers_GetEscapedURLForFilePath, il2cpp_utils::FindMethodUnsafe("", "FileHelpers", "GetEscapedURLForFilePath", 1));
    }
}