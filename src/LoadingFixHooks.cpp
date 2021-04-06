#include "LoadingFixHooks.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "CustomLogger.hpp"

#include "Paths.hpp"

#include "Utils/FindComponentsUtils.hpp"

#include "GlobalNamespace/AdditionalContentModel.hpp"
#include "GlobalNamespace/SinglePlayerLevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/NotesInTimeRowProcessor.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/NoteCutDirection.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/BeatmapLineData.hpp"
#include "GlobalNamespace/BeatmapObjectType.hpp"
#include "GlobalNamespace/BeatmapObjectData.hpp"
#include "GlobalNamespace/BeatmapEventData.hpp"
#include "GlobalNamespace/BeatmapEventTypeExtensions.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/IBeatmapLevelPack.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollection.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollectionSO.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollectionContainerSO.hpp"
#include "GlobalNamespace/HMCache_2.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Linq/Enumerable.hpp"

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace System::Collections::Generic;
using namespace System::Threading;
using namespace Tasks;

namespace RuntimeSongLoader::LoadingFixHooks {

    MAKE_HOOK_OFFSETLESS(BeatmapData_ctor, void, BeatmapData* self, int numberOfLines) {
        LOG_DEBUG("BeatmapData_ctor");
        BeatmapData_ctor(self, numberOfLines);
        self->prevAddedBeatmapEventDataTime = System::Single::MinValue;
    }

    MAKE_HOOK_OFFSETLESS(CustomBeatmapLevel_ctor, void, CustomBeatmapLevel* self, CustomPreviewBeatmapLevel* customPreviewBeatmapLevel, AudioClip* previewAudioClip) {
        LOG_DEBUG("CustomBeatmapLevel_ctor");
        CustomBeatmapLevel_ctor(self, customPreviewBeatmapLevel, previewAudioClip);
        self->songDuration = customPreviewBeatmapLevel->songDuration;
    }

    MAKE_HOOK_OFFSETLESS(BeatmapData_AddBeatmapObjectData, void, BeatmapData* self, BeatmapObjectData* beatmapObjectData) {
        //LOG_DEBUG("BeatmapData_AddBeatmapObjectData");
        self->prevAddedBeatmapObjectDataTime = beatmapObjectData->time;
        self->beatmapLinesData->values[beatmapObjectData->lineIndex]->AddBeatmapObjectData(beatmapObjectData);

        auto type = beatmapObjectData->get_beatmapObjectType();
		if (type == BeatmapObjectType::Obstacle) {
			self->obstaclesCount++;
			return;
		}
		if (type == BeatmapObjectType::Note) {
			NoteData* noteData = reinterpret_cast<NoteData*>(beatmapObjectData);
			self->notesInTimeRowProcessor->ProcessNote(noteData);
			if (noteData->cutDirection != NoteCutDirection::None) {
				self->cuttableNotesType++;
				return;
			}
			self->bombsCount++;
		}
    }

    MAKE_HOOK_OFFSETLESS(BeatmapData_AddBeatmapEventData, void, BeatmapData* self, BeatmapEventData* beatmapEventData) {
        //LOG_DEBUG("BeatmapData_AddBeatmapEventData");
        self->prevAddedBeatmapEventDataTime = beatmapEventData->time;
        self->beatmapEventsData->Add(beatmapEventData);
		if(BeatmapEventTypeExtensions::IsRotationEvent(beatmapEventData->type))
			self->spawnRotationEventsCount++;
    }

    MAKE_HOOK_OFFSETLESS(BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync, Task_1<IBeatmapLevelPackCollection*>*, BeatmapLevelsModel* self, CancellationToken cancellationToken) {
        LOG_DEBUG("BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync");
        return Task_1<IBeatmapLevelPackCollection*>::New_ctor(self->customLevelPackCollection);
    }

    MAKE_HOOK_OFFSETLESS(BeatmapLevelsModel_UpdateAllLoadedBeatmapLevelPacks, void, BeatmapLevelsModel* self) {
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

    MAKE_HOOK_OFFSETLESS(AdditionalContentModel_GetLevelEntitlementStatusAsync, Task_1<AdditionalContentModel::EntitlementStatus>*, AdditionalContentModel* self, Il2CppString* levelId, CancellationToken cancellationToken) {
        LOG_DEBUG("AdditionalContentModel_GetLevelEntitlementStatusAsync %s", to_utf8(csstrtostr(levelId)).c_str());
        if(to_utf8(csstrtostr(levelId)).starts_with(CustomLevelPrefixID)) {
            auto beatmapLevelsModel = FindComponentsUtils::GetBeatmapLevelsModel();
            bool loaded = beatmapLevelsModel->loadedPreviewBeatmapLevels->ContainsKey(levelId) || beatmapLevelsModel->loadedBeatmapLevels->IsInCache(levelId);
            return Task_1<AdditionalContentModel::EntitlementStatus>::New_ctor(loaded ? AdditionalContentModel::EntitlementStatus::Owned : AdditionalContentModel::EntitlementStatus::NotOwned);
        }
        return AdditionalContentModel_GetLevelEntitlementStatusAsync(self, levelId, cancellationToken);
    }

    MAKE_HOOK_OFFSETLESS(AdditionalContentModel_GetPackEntitlementStatusAsync, Task_1<AdditionalContentModel::EntitlementStatus>*, AdditionalContentModel* self, Il2CppString* levelPackId, CancellationToken cancellationToken) {
        LOG_DEBUG("AdditionalContentModel_GetPackEntitlementStatusAsync %s", to_utf8(csstrtostr(levelPackId)).c_str());
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
        return il2cpp_utils::createcsstr("file://" + to_utf8(csstrtostr(filePath)));
    }

    void InstallHooks() {
        INSTALL_HOOK_OFFSETLESS(getLogger(), BeatmapData_ctor, il2cpp_utils::FindMethodUnsafe("", "BeatmapData", ".ctor", 1));
        INSTALL_HOOK_OFFSETLESS(getLogger(), CustomBeatmapLevel_ctor, il2cpp_utils::FindMethodUnsafe("", "CustomBeatmapLevel", ".ctor", 2));
        INSTALL_HOOK_OFFSETLESS(getLogger(), BeatmapData_AddBeatmapObjectData, il2cpp_utils::FindMethodUnsafe("", "BeatmapData", "AddBeatmapObjectData", 1));
        INSTALL_HOOK_OFFSETLESS(getLogger(), BeatmapData_AddBeatmapEventData, il2cpp_utils::FindMethodUnsafe("", "BeatmapData", "AddBeatmapEventData", 1));
        INSTALL_HOOK_OFFSETLESS(getLogger(), BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "ReloadCustomLevelPackCollectionAsync", 1));
        INSTALL_HOOK_OFFSETLESS(getLogger(), BeatmapLevelsModel_UpdateAllLoadedBeatmapLevelPacks, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "UpdateAllLoadedBeatmapLevelPacks", 0));
        INSTALL_HOOK_OFFSETLESS(getLogger(), AdditionalContentModel_GetLevelEntitlementStatusAsync, il2cpp_utils::FindMethodUnsafe("", "AdditionalContentModel", "GetLevelEntitlementStatusAsync", 2));
        INSTALL_HOOK_OFFSETLESS(getLogger(), AdditionalContentModel_GetPackEntitlementStatusAsync, il2cpp_utils::FindMethodUnsafe("", "AdditionalContentModel", "GetPackEntitlementStatusAsync", 2));
        INSTALL_HOOK_OFFSETLESS(getLogger(), SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels, il2cpp_utils::FindMethodUnsafe("", "SinglePlayerLevelSelectionFlowCoordinator", "get_enableCustomLevels", 0)); 
        INSTALL_HOOK_OFFSETLESS(getLogger(), FileHelpers_GetEscapedURLForFilePath, il2cpp_utils::FindMethodUnsafe("", "FileHelpers", "GetEscapedURLForFilePath", 1));
    }
}