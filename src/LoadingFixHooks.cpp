#include "LoadingFixHooks.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "CustomLogger.hpp"

#include "Paths.hpp"

#include "Utils/FindComponentsUtils.hpp"

#include "CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"

#include "GlobalNamespace/AdditionalContentModel.hpp"
#include "GlobalNamespace/SinglePlayerLevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapLevelCollection.hpp"
#include "GlobalNamespace/LevelSearchViewController.hpp"
#include "GlobalNamespace/LevelSearchViewController_BeatmapLevelPackCollection.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/BeatmapLevelPack.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollection.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollectionSO.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollectionContainerSO.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/HMCache_2.hpp"
#include "GlobalNamespace/FileHelpers.hpp"
#include "System/Action_2.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Linq/Enumerable.hpp"

#include "UnityEngine/AudioClip.hpp"

#include "NUnit/Framework/_Assert.hpp"

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace System::Collections::Generic;
using namespace System::Threading;
using namespace Tasks;

namespace RuntimeSongLoader::LoadingFixHooks {

    // TODO: Use a hook that works when fixed
    MAKE_HOOK_FIND_CLASS_UNSAFE_INSTANCE (BeatmapData_ctor, "", "BeatmapData", ".ctor", void, BeatmapData* self, int numberOfLines) {
        LOG_DEBUG("BeatmapData_ctor");
        BeatmapData_ctor(self, numberOfLines);
        self->prevAddedBeatmapEventDataTime = System::Single::MinValue;
    }

    MAKE_HOOK_FIND_CLASS_UNSAFE_INSTANCE(CustomBeatmapLevel_ctor, "", "CustomBeatmapLevel", ".ctor", void, CustomBeatmapLevel* self, CustomPreviewBeatmapLevel* customPreviewBeatmapLevel, AudioClip* previewAudioClip) {
        LOG_DEBUG("CustomBeatmapLevel_ctor");
        CustomBeatmapLevel_ctor(self, customPreviewBeatmapLevel, previewAudioClip);
        self->songDuration = customPreviewBeatmapLevel->songDuration;
    }

    MAKE_HOOK_FIND(Assert_IsTrue, classof(NUnit::Framework::_Assert*), "IsTrue", void, bool, Il2CppString* message, Array<Il2CppObject*>* args) {
        //LOG_DEBUG("Assert_IsTrue");
    }

    MAKE_HOOK_MATCH(LevelSearchViewController_UpdateBeatmapLevelPackCollectionAsync, &LevelSearchViewController::UpdateBeatmapLevelPackCollectionAsync, void, LevelSearchViewController* self) {
        LOG_DEBUG("LevelSearchViewController_UpdateBeatmapLevelPackCollectionAsync");
        static auto filterName = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>("allSongs");
        List_1<IPreviewBeatmapLevel*>* newLevels = List_1<IPreviewBeatmapLevel*>::New_ctor();
        auto levelPacks = self->beatmapLevelPacks;
        if(levelPacks->Length() != 1 || csstrtostr(levelPacks->values[0]->get_packID()) != csstrtostr(filterName)) {
            for(int i = 0; i < levelPacks->Length(); i++) {
                auto levels = reinterpret_cast<BeatmapLevelPack*>(levelPacks->values[i])->get_beatmapLevelCollection()->get_beatmapLevels();
                for(int j = 0; j < levels->Length(); j++) {
                    auto level = levels->values[j];
                    if(!newLevels->Contains(level))
                        newLevels->Add(level);
                }
            }
            BeatmapLevelCollection* beatmapLevelCollection = BeatmapLevelCollection::New_ctor(nullptr);
            BeatmapLevelPack* beatmapLevelPack = BeatmapLevelPack::New_ctor(filterName, filterName, filterName, nullptr, reinterpret_cast<IBeatmapLevelCollection*>(beatmapLevelCollection));
            self->beatmapLevelPacks = Array<IBeatmapLevelPack*>::NewLength(1);
            self->beatmapLevelPacks->values[0] = reinterpret_cast<IBeatmapLevelPack*>(beatmapLevelPack);
            beatmapLevelCollection->levels = newLevels->ToArray();
        }
        LevelSearchViewController_UpdateBeatmapLevelPackCollectionAsync(self);
    }

    MAKE_HOOK_MATCH(BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync, &BeatmapLevelsModel::ReloadCustomLevelPackCollectionAsync, Task_1<IBeatmapLevelPackCollection*>*, BeatmapLevelsModel* self, CancellationToken cancellationToken) {
        LOG_DEBUG("BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync");
        return Task_1<IBeatmapLevelPackCollection*>::New_ctor(self->customLevelPackCollection);
    }

    MAKE_HOOK_MATCH(BeatmapLevelsModel_UpdateAllLoadedBeatmapLevelPacks, &BeatmapLevelsModel::UpdateAllLoadedBeatmapLevelPacks, void, BeatmapLevelsModel* self) {
        LOG_DEBUG("BeatmapLevelsModel_UpdateAllLoadedBeatmapLevelPacks");
        List<IBeatmapLevelPack*>* list = List<IBeatmapLevelPack*>::New_ctor();
        if(self->ostAndExtrasPackCollection)
            list->AddRange(reinterpret_cast<IEnumerable_1<IBeatmapLevelPack*>*>(self->ostAndExtrasPackCollection->get_beatmapLevelPacks()));
        if(self->dlcLevelPackCollectionContainer && self->dlcLevelPackCollectionContainer->beatmapLevelPackCollection)
            list->AddRange(reinterpret_cast<IEnumerable_1<IBeatmapLevelPack*>*>(self->dlcLevelPackCollectionContainer->beatmapLevelPackCollection->get_beatmapLevelPacks()));
        self->allLoadedBeatmapLevelWithoutCustomLevelPackCollection = reinterpret_cast<IBeatmapLevelPackCollection*>(BeatmapLevelPackCollection::New_ctor(list->ToArray()));
        if(self->customLevelPackCollection)
            list->AddRange(reinterpret_cast<IEnumerable_1<IBeatmapLevelPack*>*>(self->customLevelPackCollection->get_beatmapLevelPacks()));
        self->allLoadedBeatmapLevelPackCollection = reinterpret_cast<IBeatmapLevelPackCollection*>(BeatmapLevelPackCollection::New_ctor(list->ToArray()));
    }

    MAKE_HOOK_MATCH(AdditionalContentModel_GetLevelEntitlementStatusAsync, &AdditionalContentModel::GetLevelEntitlementStatusAsync, Task_1<AdditionalContentModel::EntitlementStatus>*, AdditionalContentModel* self, Il2CppString* levelId, CancellationToken cancellationToken) {
        LOG_DEBUG("AdditionalContentModel_GetLevelEntitlementStatusAsync %s", to_utf8(csstrtostr(levelId)).c_str());
        if(to_utf8(csstrtostr(levelId)).starts_with(CustomLevelPrefixID)) {
            auto beatmapLevelsModel = FindComponentsUtils::GetBeatmapLevelsModel();
            bool loaded = beatmapLevelsModel->loadedPreviewBeatmapLevels->ContainsKey(levelId) || beatmapLevelsModel->loadedBeatmapLevels->IsInCache(levelId);
            return Task_1<AdditionalContentModel::EntitlementStatus>::New_ctor(loaded ? AdditionalContentModel::EntitlementStatus::Owned : AdditionalContentModel::EntitlementStatus::NotOwned);
        }
        return AdditionalContentModel_GetLevelEntitlementStatusAsync(self, levelId, cancellationToken);
    }

    MAKE_HOOK_MATCH(AdditionalContentModel_GetPackEntitlementStatusAsync, &AdditionalContentModel::GetPackEntitlementStatusAsync, Task_1<AdditionalContentModel::EntitlementStatus>*, AdditionalContentModel* self, Il2CppString* levelPackId, CancellationToken cancellationToken) {
        LOG_DEBUG("AdditionalContentModel_GetPackEntitlementStatusAsync %s", to_utf8(csstrtostr(levelPackId)).c_str());
        if(to_utf8(csstrtostr(levelPackId)).starts_with(CustomLevelPackPrefixID))
            return Task_1<AdditionalContentModel::EntitlementStatus>::New_ctor(AdditionalContentModel::EntitlementStatus::Owned);
        return AdditionalContentModel_GetPackEntitlementStatusAsync(self, levelPackId, cancellationToken);
    }

    MAKE_HOOK_MATCH(SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels, &SinglePlayerLevelSelectionFlowCoordinator::get_enableCustomLevels, bool, SinglePlayerLevelSelectionFlowCoordinator* self) {
        LOG_DEBUG("SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels");
        return true;
    }

    MAKE_HOOK_MATCH(FileHelpers_GetEscapedURLForFilePath, &FileHelpers::GetEscapedURLForFilePath, Il2CppString*, Il2CppString* filePath) {
        LOG_DEBUG("FileHelpers_GetEscapedURLForFilePath");
        return il2cpp_utils::newcsstr(std::u16string(u"file://") + std::u16string(csstrtostr(filePath)));
    }

    void InstallHooks() {
        INSTALL_HOOK_ORIG(getLogger(), BeatmapData_ctor);
        INSTALL_HOOK_ORIG(getLogger(), CustomBeatmapLevel_ctor);
        INSTALL_HOOK_ORIG(getLogger(), Assert_IsTrue);
        INSTALL_HOOK_ORIG(getLogger(), LevelSearchViewController_UpdateBeatmapLevelPackCollectionAsync);
        INSTALL_HOOK_ORIG(getLogger(), BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync);
        INSTALL_HOOK_ORIG(getLogger(), BeatmapLevelsModel_UpdateAllLoadedBeatmapLevelPacks);
        INSTALL_HOOK_ORIG(getLogger(), AdditionalContentModel_GetLevelEntitlementStatusAsync);
        INSTALL_HOOK_ORIG(getLogger(), AdditionalContentModel_GetPackEntitlementStatusAsync);
        INSTALL_HOOK_ORIG(getLogger(), SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels);
        INSTALL_HOOK_ORIG(getLogger(), FileHelpers_GetEscapedURLForFilePath);
    }
}