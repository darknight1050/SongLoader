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
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/BeatmapDataTransformHelper.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/BeatmapData_-get_beatmapObjectsData-d__31.hpp"
#include "GlobalNamespace/BeatmapObjectData.hpp"
#include "GlobalNamespace/BeatmapLineData.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/BeatmapLevelPack.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollection.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollectionSO.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollectionContainerSO.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/NotesInTimeRowProcessor.hpp"
#include "GlobalNamespace/BeatmapDataMirrorTransform.hpp"
#include "GlobalNamespace/BeatmapDataZenModeTransform.hpp"
#include "GlobalNamespace/BeatmapDataObstaclesAndBombsTransform.hpp"
#include "GlobalNamespace/BeatmapDataNoArrowsTransform.hpp"
#include "GlobalNamespace/BeatmapDataObstaclesMergingTransform.hpp"
#include "GlobalNamespace/BeatmapDataNoEnvironmentEffectsTransform.hpp"
#include "GlobalNamespace/BeatmapDataStrobeFilterTransform.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/PracticeSettings.hpp"
#include "GlobalNamespace/EnvironmentEffectsFilterPreset.hpp"
#include "GlobalNamespace/EnvironmentIntensityReductionOptions.hpp"
#include "GlobalNamespace/HMCache_2.hpp"
#include "GlobalNamespace/FileHelpers.hpp"
#include "System/Action_2.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Linq/Enumerable.hpp"

#include "UnityEngine/AudioClip.hpp"

#include "NUnit/Framework/_Assert.hpp"

#include <map>

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace System::Collections::Generic;
using namespace System::Threading;
using namespace Tasks;

namespace RuntimeSongLoader::LoadingFixHooks {

    int addBeatmapObjectDataLineIndex;
    MAKE_HOOK_MATCH(BeatmapData_AddBeatmapObjectData, &BeatmapData::AddBeatmapObjectData, void, BeatmapData* self, BeatmapObjectData* item) {
        addBeatmapObjectDataLineIndex = item->lineIndex;
        // Preprocess the lineIndex to be 0-3 (the real method is hard-coded to 4
        // lines), recording the info needed to reverse it
        if (addBeatmapObjectDataLineIndex > 3) {
            item->lineIndex = 3;
        } else if (addBeatmapObjectDataLineIndex < 0) {
            item->lineIndex = 0;
        }
        BeatmapData_AddBeatmapObjectData(self, item);
    }

    MAKE_HOOK_MATCH(BeatmapLineData_AddBeatmapObjectData, &BeatmapLineData::AddBeatmapObjectData, void, BeatmapLineData* self, BeatmapObjectData* item) {
        item->lineIndex = addBeatmapObjectDataLineIndex;
        BeatmapLineData_AddBeatmapObjectData(self, item);
    }

    MAKE_HOOK_MATCH(NotesInTimeRowProcessor_ProcessAllNotesInTimeRow, &NotesInTimeRowProcessor::ProcessAllNotesInTimeRow, void, NotesInTimeRowProcessor* self, List<NoteData*>* notes) {
        std::map<int, int> extendedLanesMap;
        for (int i = 0; i < notes->size; ++i) {
            auto *item = notes->items->values[i];
            if (item->lineIndex > 3) {
                extendedLanesMap[i] = item->lineIndex;
                item->lineIndex = 3;
            } else if (item->lineIndex < 0) {
                extendedLanesMap[i] = item->lineIndex;
                item->lineIndex = 0;
            }
        }

        // NotesInTimeRowProcessor_ProcessAllNotesInTimeRow(self, notes);
        // Instead, we have a reimplementation of the hooked method to deal with precision
        // noteLineLayers:
        for (il2cpp_array_size_t i = 0; i < self->notesInColumns->Length(); i++) {
            self->notesInColumns->values[i]->Clear();
        }
        for (int j = 0; j < notes->size; j++) {
            auto *noteData = notes->items->values[j];
            auto *list = self->notesInColumns->values[noteData->lineIndex];

            bool flag = false;
            for (int k = 0; k < list->size; k++) {
                if (list->items->values[k]->noteLineLayer.value > noteData->noteLineLayer.value) {
                    list->Insert(k, noteData);
                    flag = true;
                    break;
                }
            }
            if (!flag) {
                list->Add(noteData);
            }
        }
        for (il2cpp_array_size_t l = 0; l < self->notesInColumns->Length(); l++) {
            auto *list2 = self->notesInColumns->values[l];
            for (int m = 0; m < list2->size; m++) {
                auto *note = list2->items->values[m];
                if (note->noteLineLayer.value >= 0 && note->noteLineLayer.value <= 2) {
                    note->SetNoteStartLineLayer(m);
                }
            }
        }

        for (int i = 0; i < notes->size; ++i) {
            if (extendedLanesMap.find(i) != extendedLanesMap.end()) {
                auto *item = notes->items->values[i];
                item->lineIndex = extendedLanesMap[i];
            }
        }
    }

    MAKE_HOOK_MATCH(BeatmapData_$get_beatmapObjectsData$d__31__MoveNext, &BeatmapData::$get_beatmapObjectsData$d__31::MoveNext, bool, BeatmapData::$get_beatmapObjectsData$d__31* self) {
        int num = self->$$1__state;
        BeatmapData *beatmapData = self->$$4__this;
        if (num != 0) {
            if (num != 1) {
                return false;
            }
            self->$$1__state = -1;
            // Increment index in idxs with clamped lineIndex
            int lineIndex = self->$minBeatmapObjectData$5__4->lineIndex;
            int clampedLineIndex = std::clamp(lineIndex, 0, 3);
            self->$idxs$5__3->values[clampedLineIndex]++;
            self->$minBeatmapObjectData$5__4 = nullptr;
        } else {
            self->$$1__state = -1;
            auto *arr =
                reinterpret_cast<Array<BeatmapLineData *> *>(beatmapData->get_beatmapLinesData());
            self->$beatmapLinesData$5__2 = arr;
            self->$idxs$5__3 = Array<int>::NewLength(self->$beatmapLinesData$5__2->Length());
        }
        self->$minBeatmapObjectData$5__4 = nullptr;
        float num2 = std::numeric_limits<float>::max();
        for (int i = 0; i < self->$beatmapLinesData$5__2->Length(); i++) {
            int idx = self->$idxs$5__3->values[i];
            BeatmapLineData *lineData = self->$beatmapLinesData$5__2->values[i];
            if (idx < lineData->beatmapObjectsData->get_Count()) {
                BeatmapObjectData *beatmapObjectData = lineData->beatmapObjectsData->get_Item(idx);
                float time = beatmapObjectData->time;
                if (time < num2) {
                    num2 = time;
                    self->$minBeatmapObjectData$5__4 = beatmapObjectData;
                }
            }
        }
        if (self->$minBeatmapObjectData$5__4 == nullptr) {
            return false;
        }
        self->$$2__current = self->$minBeatmapObjectData$5__4;
        self->$$1__state = 1;
        return true;
    }

    MAKE_HOOK_MATCH(BeatmapDataTransformHelper_CreateTransformedBeatmapData, &BeatmapDataTransformHelper::CreateTransformedBeatmapData, IReadonlyBeatmapData*, IReadonlyBeatmapData* beatmapData, IPreviewBeatmapLevel* beatmapLevel, GameplayModifiers* gameplayModifiers, PracticeSettings* practiceSettings, bool leftHanded, EnvironmentEffectsFilterPreset environmentEffectsFilterPreset, EnvironmentIntensityReductionOptions* environmentIntensityReductionOptions, bool screenDisplacementEffectsEnabled) {
        IReadonlyBeatmapData* readonlyBeatmapData = beatmapData;
        if (leftHanded)
		{
			readonlyBeatmapData = BeatmapDataMirrorTransform::CreateTransformedData(readonlyBeatmapData);
		}
		if (gameplayModifiers->zenMode)
		{
			readonlyBeatmapData = BeatmapDataZenModeTransform::CreateTransformedData(readonlyBeatmapData);
		}
		else
		{
			GameplayModifiers::EnabledObstacleType enabledObstacleType = gameplayModifiers->enabledObstacleType;
			if (gameplayModifiers->demoNoObstacles)
			{
				enabledObstacleType = GameplayModifiers::EnabledObstacleType::NoObstacles;
			}
			if (enabledObstacleType != GameplayModifiers::EnabledObstacleType::All || gameplayModifiers->noBombs)
			{
				readonlyBeatmapData = BeatmapDataObstaclesAndBombsTransform::CreateTransformedData(readonlyBeatmapData, enabledObstacleType, gameplayModifiers->noBombs);
			}
			if (gameplayModifiers->noArrows)
			{
				readonlyBeatmapData = BeatmapDataNoArrowsTransform::CreateTransformedData(readonlyBeatmapData);
			}
			if (!screenDisplacementEffectsEnabled && !to_utf8(csstrtostr(beatmapLevel->get_levelID())).starts_with(CustomLevelPrefixID))
			{
				readonlyBeatmapData = BeatmapDataObstaclesMergingTransform::CreateTransformedData(readonlyBeatmapData);
			}
		}
		if (environmentEffectsFilterPreset >= EnvironmentEffectsFilterPreset::NoEffects)
		{
			readonlyBeatmapData = reinterpret_cast<IReadonlyBeatmapData*>(BeatmapDataNoEnvironmentEffectsTransform::CreateTransformedData(readonlyBeatmapData));
		}
		else if (environmentEffectsFilterPreset == EnvironmentEffectsFilterPreset::StrobeFilter)
		{
			readonlyBeatmapData = BeatmapDataStrobeFilterTransform::CreateTransformedData(readonlyBeatmapData, environmentIntensityReductionOptions);
		}
		if (readonlyBeatmapData == beatmapData)
		{
			readonlyBeatmapData = reinterpret_cast<IReadonlyBeatmapData*>(beatmapData->GetCopy());
		}
        return readonlyBeatmapData;
    }

    // TODO: Use a hook that works when fixed
    MAKE_HOOK_FIND_CLASS_UNSAFE_INSTANCE(BeatmapData_ctor, "", "BeatmapData", ".ctor", void, BeatmapData* self, int numberOfLines) {
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
        INSTALL_HOOK_ORIG(getLogger(), BeatmapData_AddBeatmapObjectData);
        INSTALL_HOOK_ORIG(getLogger(), BeatmapLineData_AddBeatmapObjectData);
        INSTALL_HOOK_ORIG(getLogger(), NotesInTimeRowProcessor_ProcessAllNotesInTimeRow);
        INSTALL_HOOK_ORIG(getLogger(), BeatmapData_$get_beatmapObjectsData$d__31__MoveNext);
        INSTALL_HOOK_ORIG(getLogger(), BeatmapDataTransformHelper_CreateTransformedBeatmapData);
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