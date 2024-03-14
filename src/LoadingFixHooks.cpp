#include "LoadingFixHooks.hpp"

#include "beatsaber-hook/shared/utils/typedefs.h"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "CustomLogger.hpp"

#include "Paths.hpp"
#include "assets.hpp"

#include "LevelData.hpp"

#include "Utils/FindComponentsUtils.hpp"

#include "CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"
#include "CustomTypes/CustomLevelInfoSaveData.hpp"
#include "CustomTypes/SongLoader.hpp"

#include "bsml/shared/BSML-Lite.hpp"

#include "GlobalNamespace/AdditionalContentModel.hpp"
#include "GlobalNamespace/SinglePlayerLevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/BeatmapLevel.hpp"
#include "GlobalNamespace/LevelSearchViewController.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/BeatmapDataTransformHelper.hpp"
#include "GlobalNamespace/BeatmapBasicData.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/BeatmapObjectData.hpp"
#include "GlobalNamespace/BeatmapLineData.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/BeatmapLevelPack.hpp"
#include "GlobalNamespace/BeatmapLevelsRepository.hpp"
#include "GlobalNamespace/BeatmapLevelLoader.hpp"
#include "GlobalNamespace/BeatmapLevelDataLoader.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapDataMirrorTransform.hpp"
#include "GlobalNamespace/BeatmapDataZenModeTransform.hpp"
#include "GlobalNamespace/BeatmapDataObstaclesAndBombsTransform.hpp"
#include "GlobalNamespace/BeatmapDataNoArrowsTransform.hpp"
#include "GlobalNamespace/BeatmapDataObstaclesMergingTransform.hpp"
#include "GlobalNamespace/BeatmapDataStrobeFilterTransform.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/PracticeSettings.hpp"
#include "GlobalNamespace/EnvironmentEffectsFilterPreset.hpp"
#include "GlobalNamespace/EnvironmentIntensityReductionOptions.hpp"
#include "GlobalNamespace/FileHelpers.hpp"
#include "GlobalNamespace/MainSettingsModelSO.hpp"
#include "GlobalNamespace/BeatmapObjectSpawnMovementData.hpp"
#include "GlobalNamespace/IJumpOffsetYProvider.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/StandardLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/MultiplayerLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/MissionLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/BoolSO.hpp"
#include "GlobalNamespace/BeatmapSaveDataHelpers.hpp"
#include "GlobalNamespace/PlayerSensitivityFlag.hpp"
#include "GlobalNamespace/EntitlementStatus.hpp"
#include "GlobalNamespace/OculusPlatformAdditionalContentModel.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsViewController.hpp"
#include "GlobalNamespace/LoadingControl.hpp"
#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsGridView.hpp"
#include "GlobalNamespace/PackDefinitionSO.hpp"
#include "GlobalNamespace/BeatmapLevelPackSO.hpp"
#include "GlobalNamespace/BeatmapLevelSO.hpp"
#include "GlobalNamespace/BeatmapLevelCollectionSO.hpp"
#include "GlobalNamespace/LoadBeatmapLevelDataResult.hpp"
#include "GlobalNamespace/BeatmapSaveDataHelpers.hpp"

#include "Environments/Definitions/EnvironmentsAsyncInstaller.hpp"

#include "System/Version.hpp"
#include "System/Action_2.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include "System/Collections/Generic/IEnumerator_1.hpp"
#include "System/Collections/Generic/IEnumerable_1.hpp"
#include "System/Collections/Generic/IReadOnlyList_1.hpp"
#include "System/Collections/Generic/IReadOnlyCollection_1.hpp"
#include "System/Collections/Generic/IReadOnlyDictionary_2.hpp"
#include "System/Collections/IEnumerator.hpp"
#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Linq/Enumerable.hpp"

#include "BGLib/UnityExtension/JsonSettings.hpp"

#include "Zenject/DiContainer.hpp"

#include "Newtonsoft/Json/Formatting.hpp"
#include "Newtonsoft/Json/DefaultValueHandling.hpp"
#include "Newtonsoft/Json/JsonConvert.hpp"
#include "Newtonsoft/Json/JsonSerializerSettings.hpp"
#include "Newtonsoft/Json/Serialization/DefaultContractResolver.hpp"

#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/AudioClip.hpp"
#include "UnityEngine/Networking/UnityWebRequest.hpp"

#include <map>
#include <regex>

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace System::Collections::Generic;
using namespace System::Threading;
using namespace Tasks;

namespace RuntimeSongLoader::LoadingFixHooks {

    MAKE_HOOK_MATCH(BeatmapDataTransformHelper_CreateTransformedBeatmapData, &BeatmapDataTransformHelper::CreateTransformedBeatmapData, IReadonlyBeatmapData*, IReadonlyBeatmapData* beatmapData, BeatmapLevel* beatmapLevel, GameplayModifiers* gameplayModifiers, bool leftHanded, EnvironmentEffectsFilterPreset environmentEffectsFilterPreset, EnvironmentIntensityReductionOptions* environmentIntensityReductionOptions, MainSettingsModelSO* mainSettingsModel) {
        LOG_DEBUG("BeatmapDataTransformHelper_CreateTransformedBeatmapData");
        // BeatGames, why did you put the effort into making this not work on Quest IF CUSTOM LEVELS ARE NOT EVEN LOADED IN THE FIRST PLACE BASEGAME.
        // THERE WAS NO POINT IN CHANGING THE IF STATEMENT SPECIFICALLY FOR QUEST
        // Sincerely, a quest developer
        bool& screenDisplacementEffectsEnabled = mainSettingsModel->screenDisplacementEffectsEnabled->_value;
        bool oldScreenDisplacementEffectsEnabled = screenDisplacementEffectsEnabled;

        if(beatmapLevel->levelID.starts_with(CustomLevelPrefixID))
            screenDisplacementEffectsEnabled = screenDisplacementEffectsEnabled || beatmapLevel->levelID.starts_with(CustomLevelPrefixID);

        auto result = BeatmapDataTransformHelper_CreateTransformedBeatmapData(
            beatmapData, beatmapLevel, gameplayModifiers, leftHanded,
            environmentEffectsFilterPreset, environmentIntensityReductionOptions,
            mainSettingsModel);

        screenDisplacementEffectsEnabled = oldScreenDisplacementEffectsEnabled;

        return result;
    }

    // TODO: Use a hook that works when fixed
//    MAKE_HOOK_MATCH(CustomBeatmapLevel_ctor, &CustomBeatmapLevel::_ctor, void, CustomBeatmapLevel* self, CustomPreviewBeatmapLevel* customPreviewBeatmapLevel) {
//        LOG_DEBUG("CustomBeatmapLevel_ctor");
//        CustomBeatmapLevel_ctor(self, customPreviewBeatmapLevel);
//        self->_songDuration_k__BackingField = customPreviewBeatmapLevel->songDuration;
//    }

    MAKE_HOOK_MATCH(StandardLevelScenesTransitionSetupDataSO_Init, static_cast<void(StandardLevelScenesTransitionSetupDataSO::*)(StringW, ByRef<::GlobalNamespace::BeatmapKey>, ::GlobalNamespace::BeatmapLevel*, GlobalNamespace::OverrideEnvironmentSettings*, GlobalNamespace::ColorScheme*, ::GlobalNamespace::ColorScheme*, GlobalNamespace::GameplayModifiers*, ::GlobalNamespace::PlayerSpecificSettings*, GlobalNamespace::PracticeSettings*, ::GlobalNamespace::EnvironmentsListModel*, GlobalNamespace::AudioClipAsyncLoader*, ::GlobalNamespace::BeatmapDataLoader*, StringW, ::GlobalNamespace::BeatmapLevelsModel*, bool, bool, ::System::Nullable_1<::GlobalNamespace::RecordingToolManager::SetupData>)>(&StandardLevelScenesTransitionSetupDataSO::Init),
                    void, StandardLevelScenesTransitionSetupDataSO* self, ::StringW gameMode, ByRef<::GlobalNamespace::BeatmapKey> beatmapKey, ::GlobalNamespace::BeatmapLevel* beatmapLevel, GlobalNamespace::OverrideEnvironmentSettings* overrideEnvironmentSettings, GlobalNamespace::ColorScheme* overrideColorScheme, ::GlobalNamespace::ColorScheme* beatmapOverrideColorScheme, GlobalNamespace::GameplayModifiers* gameplayModifiers, ::GlobalNamespace::PlayerSpecificSettings* playerSpecificSettings, GlobalNamespace::PracticeSettings* practiceSettings, ::GlobalNamespace::EnvironmentsListModel* environmentsListModel, GlobalNamespace::AudioClipAsyncLoader* audioClipAsyncLoader, ::GlobalNamespace::BeatmapDataLoader* beatmapDataLoader, StringW backButtonText, ::GlobalNamespace::BeatmapLevelsModel* beatmapLevelsModel, bool useTestNoteCutSoundEffects, bool startPaused, ::System::Nullable_1<::GlobalNamespace::RecordingToolManager::SetupData> recordingToolData) {
        LOG_DEBUG("StandardLevelScenesTransitionSetupDataSO_Init");
        StandardLevelScenesTransitionSetupDataSO_Init(self, gameMode, beatmapKey, beatmapLevel, overrideEnvironmentSettings, overrideColorScheme, beatmapOverrideColorScheme, gameplayModifiers, playerSpecificSettings, practiceSettings, environmentsListModel, audioClipAsyncLoader, beatmapDataLoader, backButtonText, beatmapLevelsModel, useTestNoteCutSoundEffects, startPaused, recordingToolData);
        LevelData::gameplayCoreSceneSetupData = self->gameplayCoreSceneSetupData;
    }

    MAKE_HOOK_MATCH(MultiplayerLevelScenesTransitionSetupDataSO_Init, &MultiplayerLevelScenesTransitionSetupDataSO::Init, void, MultiplayerLevelScenesTransitionSetupDataSO* self, StringW gameMode, ByRef<::GlobalNamespace::BeatmapKey> beatmapKey, ::GlobalNamespace::BeatmapLevel* beatmapLevel, GlobalNamespace::IBeatmapLevelData* beatmapLevelData, ::GlobalNamespace::ColorScheme* overrideColorScheme, ::GlobalNamespace::GameplayModifiers* gameplayModifiers, ::GlobalNamespace::PlayerSpecificSettings* playerSpecificSettings, ::GlobalNamespace::PracticeSettings* practiceSettings, ::GlobalNamespace::AudioClipAsyncLoader* audioClipAsyncLoader, GlobalNamespace::BeatmapDataLoader* beatmapDataLoader, bool useTestNoteCutSoundEffects) {
        LOG_DEBUG("MultiplayerLevelScenesTransitionSetupDataSO_Init");
        MultiplayerLevelScenesTransitionSetupDataSO_Init(self, gameMode, beatmapKey, beatmapLevel, beatmapLevelData, overrideColorScheme, gameplayModifiers, playerSpecificSettings, practiceSettings, audioClipAsyncLoader, beatmapDataLoader, useTestNoteCutSoundEffects);
        LevelData::gameplayCoreSceneSetupData = self->gameplayCoreSceneSetupData;
    }

    MAKE_HOOK_MATCH(MissionLevelScenesTransitionSetupDataSO_Init, static_cast<void(MissionLevelScenesTransitionSetupDataSO::*)(StringW, ::GlobalNamespace::IBeatmapLevelData*, ByRef<::GlobalNamespace::BeatmapKey>, ::GlobalNamespace::BeatmapLevel*, ArrayW<::GlobalNamespace::MissionObjective*>, ::GlobalNamespace::ColorScheme*, GlobalNamespace::GameplayModifiers*, ::GlobalNamespace::PlayerSpecificSettings*, ::GlobalNamespace::EnvironmentsListModel*, GlobalNamespace::AudioClipAsyncLoader*, ::GlobalNamespace::BeatmapDataLoader*, ::StringW)>(&MissionLevelScenesTransitionSetupDataSO::Init),
                    void, MissionLevelScenesTransitionSetupDataSO* self, StringW missionId, ::GlobalNamespace::IBeatmapLevelData* beatmapLevelData, ByRef<::GlobalNamespace::BeatmapKey> beatmapKey, ::GlobalNamespace::BeatmapLevel* beatmapLevel, ArrayW<::GlobalNamespace::MissionObjective*> missionObjectives, ::GlobalNamespace::ColorScheme* overrideColorScheme, GlobalNamespace::GameplayModifiers* gameplayModifiers, ::GlobalNamespace::PlayerSpecificSettings* playerSpecificSettings, ::GlobalNamespace::EnvironmentsListModel* environmentsListModel, GlobalNamespace::AudioClipAsyncLoader* audioClipAsyncLoader, ::GlobalNamespace::BeatmapDataLoader* beatmapDataLoader, ::StringW backButtonText) {
        LOG_DEBUG("MissionLevelScenesTransitionSetupDataSO_Init");
        MissionLevelScenesTransitionSetupDataSO_Init(self, missionId, beatmapLevelData, beatmapKey, beatmapLevel, missionObjectives, overrideColorScheme, gameplayModifiers, playerSpecificSettings, environmentsListModel, audioClipAsyncLoader, beatmapDataLoader, backButtonText);
        LevelData::gameplayCoreSceneSetupData = self->gameplayCoreSceneSetupData;
    }

    // https://github.com/Kylemc1413/SongCore/blob/master/source/SongCore/HarmonyPatches/NegativeNjsPatch.cs
    // patch to allow negative NJS
    MAKE_HOOK_MATCH(BeatmapObjectSpawnMovementData_Init, &BeatmapObjectSpawnMovementData::Init, void, BeatmapObjectSpawnMovementData* self, int noteLinesCount, float startNoteJumpMovementSpeed, float startBpm, BeatmapObjectSpawnMovementData::NoteJumpValueType noteJumpValueType, float noteJumpValue, IJumpOffsetYProvider* jumpOffsetYProvider, UnityEngine::Vector3 rightVec, UnityEngine::Vector3 forwardVec) {
        LOG_DEBUG("BeatmapObjectSpawnMovementData_Init");
        if(LevelData::gameplayCoreSceneSetupData) {
            auto gameplayCoreSceneSetupData = LevelData::gameplayCoreSceneSetupData;
            auto dict = gameplayCoreSceneSetupData->beatmapLevel->beatmapBasicData;
            auto collection = dict->i___System__Collections__Generic__IReadOnlyCollection_1___System__Collections__Generic__KeyValuePair_2_TKey_TValue__();
            auto enumerable = collection->i___System__Collections__Generic__IEnumerable_1_T_();
            auto enumerator_1 = enumerable->GetEnumerator();
            auto enumerator = enumerator_1->i___System__Collections__IEnumerator();

            BeatmapBasicData* beatmapData = nullptr;
            while(enumerator->MoveNext())
            {
                auto current = enumerator_1->get_Current();
                if(current.key.Equals(System::ValueTuple_2(gameplayCoreSceneSetupData->beatmapKey.beatmapCharacteristic, gameplayCoreSceneSetupData->beatmapKey.difficulty)))
                {
                    beatmapData = current.value;
                }
            }

            if(beatmapData)
            {
                float noteJumpMovementSpeed = beatmapData->noteJumpMovementSpeed;
                if (noteJumpMovementSpeed < 0)
                    startNoteJumpMovementSpeed = noteJumpMovementSpeed;
            }
        }
        BeatmapObjectSpawnMovementData_Init(self, noteLinesCount, startNoteJumpMovementSpeed, startBpm, noteJumpValueType, noteJumpValue, jumpOffsetYProvider, rightVec, forwardVec);
    }

    MAKE_HOOK_MATCH(LevelSearchViewController_UpdateBeatmapLevelPackCollectionAsync, &LevelSearchViewController::RefreshAsync, void, LevelSearchViewController* self) {
        LOG_DEBUG("LevelSearchViewController_UpdateBeatmapLevelPackCollectionAsync");
        static ConstString filterName("allSongs");
        auto newLevels = ListW<BeatmapLevel*>::New();
        auto levelPacks = self->_beatmapLevelPacks;
        if(levelPacks.size() != 1 || levelPacks[0]->packID != filterName) {
            for(auto levelPack : levelPacks) {
                GlobalNamespace::BeatmapLevelPack* levelCollection = levelPack;
                ArrayW<GlobalNamespace::BeatmapLevel*> levels{levelCollection->beatmapLevels};
                for(auto level : levels) {
                    if(!newLevels->Contains(level))
                        newLevels->Add(level);
                }
            }

            BeatmapLevelPack* beatmapLevelPack = BeatmapLevelPack::New_ctor(filterName, filterName, filterName, nullptr, nullptr, newLevels.to_array(), ::GlobalNamespace::PlayerSensitivityFlag::Unknown);
            self->_beatmapLevelPacks = ArrayW<BeatmapLevelPack*>(1);
            self->_beatmapLevelPacks[0] = beatmapLevelPack;
        }
        LevelSearchViewController_UpdateBeatmapLevelPackCollectionAsync(self);
    }

    MAKE_HOOK_MATCH(BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync, &BeatmapLevelsModel::ReloadCustomLevelPackCollectionAsync, Task_1<BeatmapLevelsRepository*>*, BeatmapLevelsModel* self, CancellationToken cancellationToken) {
        LOG_DEBUG("BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync");
        // Not sure if this works
        return Task_1<BeatmapLevelsRepository*>::New_ctor(SongLoader::GetInstance()->CustomBeatmapLevelPackCollectionSO);
    }

    MAKE_HOOK_MATCH(BeatmapLevelsModel_CreateAllLoadedBeatmapLevelPacks, &BeatmapLevelsModel::CreateAllLoadedBeatmapLevelPacks, BeatmapLevelsRepository*, BeatmapLevelsModel* self) {
        LOG_DEBUG("BeatmapLevelsModel_CreateAllLoadedBeatmapLevelPacks");
        ListW<BeatmapLevelPack*> list = ListW<BeatmapLevelPack*>::New();
        for (PackDefinitionSO* packDefinitionSO : self->_packDefinitions)
        {
            list->Add(BeatmapLevelsModel::CreateBeatmapLevelPack(packDefinitionSO));
        }
        list->AddRange(SongLoader::GetInstance()->CustomBeatmapLevelPackCollectionSO->beatmapLevelPacks->i___System__Collections__Generic__IEnumerable_1_T_());
        return BeatmapLevelsRepository::New_ctor(list->i___System__Collections__Generic__IReadOnlyList_1_T_());
    }

    MAKE_HOOK_MATCH(OculusPlatformAdditionalContentModel_GetLevelEntitlementStatusAsync, &OculusPlatformAdditionalContentModel::GetLevelEntitlementStatusInternalAsync, Task_1<EntitlementStatus>*, OculusPlatformAdditionalContentModel* self, StringW levelId, CancellationToken cancellationToken) {
        std::string levelIdCpp = levelId;
        LOG_DEBUG("OculusPlatformAdditionalContentModel_GetLevelEntitlementStatusAsync %s", levelIdCpp.c_str());
        if(levelId.starts_with(CustomLevelPrefixID))
            return Task_1<EntitlementStatus>::New_ctor(EntitlementStatus::Owned);
        return OculusPlatformAdditionalContentModel_GetLevelEntitlementStatusAsync(self, levelId, cancellationToken);
    }

    MAKE_HOOK_MATCH(OculusPlatformAdditionalContentModel_GetPackEntitlementStatusAsync, &OculusPlatformAdditionalContentModel::GetPackEntitlementStatusInternalAsync, Task_1<EntitlementStatus>*, OculusPlatformAdditionalContentModel* self, StringW levelPackId, CancellationToken cancellationToken) {
         std::string levelPackIdCpp = levelPackId;
        LOG_DEBUG("OculusPlatformAdditionalContentModel_GetPackEntitlementStatusAsync %s", levelPackIdCpp.c_str());

        if(levelPackId.starts_with(CustomLevelPackPrefixID))
            return Task_1<EntitlementStatus>::New_ctor(EntitlementStatus::Owned);

        return OculusPlatformAdditionalContentModel_GetPackEntitlementStatusAsync(self, levelPackId, cancellationToken);
    }

    MAKE_HOOK_MATCH(SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels, &SinglePlayerLevelSelectionFlowCoordinator::get_enableCustomLevels, bool, SinglePlayerLevelSelectionFlowCoordinator* self) {
        LOG_DEBUG("SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels");
        return true;
    }

    MAKE_HOOK_MATCH(FileHelpers_GetEscapedURLForFilePath, &FileHelpers::GetEscapedURLForFilePath, StringW, StringW filePath) {
        LOG_DEBUG("FileHelpers_GetEscapedURLForFilePath");
        std::u16string str = static_cast<std::u16string>(filePath);
        int index = str.find_last_of('/') + 1;
        StringW dir = str.substr(0, index);
        StringW fileName = Networking::UnityWebRequest::EscapeURL(str.substr(index, str.size()));
        std::replace(fileName.begin(), fileName.end(), u'+', u' '); // '+' breaks stuff even though it's supposed to be valid encoding ¯\_(ツ)_/¯
        return u"file://" + dir + fileName;
    }


// Implementation by https://github.com/StackDoubleFlow
    MAKE_HOOK_MATCH(StandardLevelInfoSaveData_DeserializeFromJSONString, &GlobalNamespace::StandardLevelInfoSaveData::DeserializeFromJSONString, GlobalNamespace::StandardLevelInfoSaveData *, StringW stringData) {
        LOG_DEBUG("StandardLevelInfoSaveData_DeserializeFromJSONString");

        SafePtr<GlobalNamespace::StandardLevelInfoSaveData> original = StandardLevelInfoSaveData_DeserializeFromJSONString(stringData);

        if (!original || !original.ptr()) {
            LOG_DEBUG("Orig call did not produce valid savedata!");
            return nullptr;
        }

        auto customBeatmapSets = ArrayW<GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmapSet*>(il2cpp_array_size_t(original->difficultyBeatmapSets.size()));

        CustomJSONData::CustomLevelInfoSaveData *customSaveData =
                CustomJSONData::CustomLevelInfoSaveData::New_ctor(
                    original->songName,
                    original->songSubName,
                    original->songAuthorName,
                    original->levelAuthorName,
                    original->beatsPerMinute,
                    original->songTimeOffset,
                    original->shuffle,
                    original->shufflePeriod,
                    original->previewStartTime,
                    original->previewDuration,
                    original->songFilename,
                    original->coverImageFilename,
                    original->environmentName,
                    original->allDirectionsEnvironmentName,
                    original->environmentNames,
                    original->colorSchemes,
                    customBeatmapSets
                );

        std::u16string str(stringData ? stringData : u"{}");

        auto sharedDoc = std::make_shared<CustomJSONData::DocumentUTF16>();
        customSaveData->doc = sharedDoc;

        rapidjson::GenericDocument<rapidjson::UTF16<char16_t>> &doc = *sharedDoc;
        doc.Parse(str.c_str());

        auto dataItr = doc.FindMember(u"_customData");
        if (dataItr != doc.MemberEnd()) {
            customSaveData->customData = dataItr->value;
        }

        CustomJSONData::ValueUTF16 const& beatmapSetsArr = doc.FindMember(u"_difficultyBeatmapSets")->value;

        for (rapidjson::SizeType i = 0; i < beatmapSetsArr.Size(); i++) {
            CustomJSONData::ValueUTF16 const& beatmapSetJson = beatmapSetsArr[i];

            LOG_DEBUG("Handling set %d", i);
            auto originalBeatmapSet = original->difficultyBeatmapSets[i];
            auto customBeatmaps = ArrayW<GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmap *>(originalBeatmapSet->difficultyBeatmaps.size());

            auto const& difficultyBeatmaps = beatmapSetJson.FindMember(u"_difficultyBeatmaps")->value;

            for (rapidjson::SizeType j = 0; j < originalBeatmapSet->difficultyBeatmaps.size(); j++) {
                LOG_DEBUG("Handling map %d", j);

                CustomJSONData::ValueUTF16 const& difficultyBeatmapJson = difficultyBeatmaps[j];
                auto originalBeatmap = originalBeatmapSet->difficultyBeatmaps[j];

                auto customBeatmap =
                    CustomJSONData::CustomDifficultyBeatmap::New_ctor(
                        originalBeatmap->difficulty,
                        originalBeatmap->difficultyRank,
                        originalBeatmap->beatmapFilename,
                        originalBeatmap->noteJumpMovementSpeed,
                        originalBeatmap->noteJumpStartBeatOffset,
                        originalBeatmap->beatmapColorSchemeIdx,
                        originalBeatmap->environmentNameIdx
                    );

                auto customDataItr = difficultyBeatmapJson.FindMember(u"_customData");
                if (customDataItr != difficultyBeatmapJson.MemberEnd()) {
                    customBeatmap->customData = customDataItr->value;
                }

                customBeatmaps[j] = customBeatmap;
            }

            customBeatmapSets[i] = GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmapSet::New_ctor(
                originalBeatmapSet->beatmapCharacteristicName,
                customBeatmaps
            );
        }

        return customSaveData;
    }

    MAKE_HOOK_MATCH(BeatmapLevelLoader_HandleItemWillBeRemovedFromCache, &BeatmapLevelLoader::HandleItemWillBeRemovedFromCache, void, BeatmapLevelLoader* self, StringW beatmapLevelId, IBeatmapLevelData* beatmapLevel) {
        LOG_DEBUG("BeatmapLevelLoader_HandleItemWillBeRemovedFromCache");
        self->_beatmapLevelDataLoader->TryUnload(beatmapLevelId);
    }

    MAKE_HOOK_MATCH(LevelFilteringNavigationController_UpdateCustomSongs, &LevelFilteringNavigationController::UpdateCustomSongs, void, LevelFilteringNavigationController* self) {
        LOG_DEBUG("LevelFilteringNavigationController_UpdateCustomSongs");

        ListW<BeatmapLevelPack*> list = ListW<BeatmapLevelPack*>::New();
        if(self->_ostBeatmapLevelPacks) {
            list->AddRange(static_cast<System::Collections::Generic::IEnumerable_1<BeatmapLevelPack*>*>(self->_ostBeatmapLevelPacks.convert()));
        }
        else {
            return;
        }
        if(self->_musicPacksBeatmapLevelPacks) {
            list->AddRange(static_cast<System::Collections::Generic::IEnumerable_1<BeatmapLevelPack*>*>(self->_musicPacksBeatmapLevelPacks.convert()));
        }
        else {
            return;
        }
        if(self->_customLevelPacks) {
            list->AddRange(self->_customLevelPacks->i___System__Collections__Generic__IEnumerable_1_T_());
        }
        else {
            return;
        }
        self->_allBeatmapLevelPacks = list->ToArray();
        self->_levelSearchViewController->Setup(self->_allBeatmapLevelPacks);
        self->UpdateSecondChildControllerContent(self->_selectLevelCategoryViewController->selectedLevelCategory);
    }

    MAKE_HOOK_MATCH(LevelFilteringNavigationController_UpdateSecondChildControllerContent, &LevelFilteringNavigationController::UpdateSecondChildControllerContent, void, LevelFilteringNavigationController* self, SelectLevelCategoryViewController::LevelCategory levelCategory) {
        LOG_DEBUG("LevelFilteringNavigationController_UpdateSecondChildControllerContent");
        self->_customLevelPacks = static_cast<System::Collections::Generic::IReadOnlyList_1<BeatmapLevelPack*>*>(SongLoader::GetInstance()->CustomBeatmapLevelPackCollectionSO->customBeatmapLevelPacks.convert());
        
        LevelFilteringNavigationController_UpdateSecondChildControllerContent(self, levelCategory);
    }

    MAKE_HOOK_MATCH(EnvironmentsAsyncInstaller_InstallBindings, &Environments::Definitions::EnvironmentsAsyncInstaller::InstallBindings, void, Environments::Definitions::EnvironmentsAsyncInstaller* self) {
        LOG_DEBUG("EnvironmentsAsyncInstaller_InstallBindings");
        EnvironmentsAsyncInstaller_InstallBindings(self);
        RuntimeSongLoader::SongLoader::GetInstance()->environmentsListModel = self->Container->Resolve<EnvironmentsListModel*>();
        LOG_DEBUG("env list %p", RuntimeSongLoader::SongLoader::GetInstance()->environmentsListModel);
    }

    MAKE_HOOK_MATCH(BeatmapLevelsModel_LoadBeatmapLevelDataAsync, &BeatmapLevelsModel::LoadBeatmapLevelDataAsync, Task_1<LoadBeatmapLevelDataResult>*, BeatmapLevelsModel* self, StringW levelID, CancellationToken token) {
        LOG_DEBUG("BeatmapLevelsModel_LoadBeatmapLevelDataAsync");
        auto id = levelID;
        if (!id.starts_with("custom_level_")) {
            return BeatmapLevelsModel_LoadBeatmapLevelDataAsync(self, levelID, token);
        }

        auto levelDatas = RuntimeSongLoader::SongLoader::GetInstance()->LevelDatas;
        IBeatmapLevelData* out;
        if(levelDatas->TryGetValue(id, byref(out))) {
            return Task_1<LoadBeatmapLevelDataResult>::New_ctor(LoadBeatmapLevelDataResult::Success(out));
        }

        return BeatmapLevelsModel_LoadBeatmapLevelDataAsync(self, levelID, token);
    }

    MAKE_HOOK_MATCH(BeatmapLevelsModel_CheckBeatmapLevelDataExistsAsync, &BeatmapLevelsModel::CheckBeatmapLevelDataExistsAsync, Task_1<bool>*, BeatmapLevelsModel* self, StringW levelID, CancellationToken token) {
        LOG_DEBUG("BeatmapLevelsModel_CheckBeatmapLevelDataExistsAsync");
        auto id = levelID;
        if (!id.starts_with("custom_level_")) {
            return BeatmapLevelsModel_CheckBeatmapLevelDataExistsAsync(self, levelID, token);
        }

        auto levelDatas = RuntimeSongLoader::SongLoader::GetInstance()->LevelDatas;
        IBeatmapLevelData* out;
        if(levelDatas->TryGetValue(id, byref(out))) {
            return Task_1<bool>::New_ctor(true);
        }

        return BeatmapLevelsModel_CheckBeatmapLevelDataExistsAsync(self, levelID, token);
    }

    void InstallHooks() {
        INSTALL_HOOK(getLogger(), BeatmapDataTransformHelper_CreateTransformedBeatmapData);
        INSTALL_HOOK_ORIG(getLogger(), StandardLevelScenesTransitionSetupDataSO_Init);
        INSTALL_HOOK_ORIG(getLogger(), MultiplayerLevelScenesTransitionSetupDataSO_Init);
        INSTALL_HOOK_ORIG(getLogger(), MissionLevelScenesTransitionSetupDataSO_Init);
        INSTALL_HOOK_ORIG(getLogger(), BeatmapObjectSpawnMovementData_Init);
        INSTALL_HOOK_ORIG(getLogger(), LevelSearchViewController_UpdateBeatmapLevelPackCollectionAsync);
        INSTALL_HOOK_ORIG(getLogger(), BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync);
        INSTALL_HOOK_ORIG(getLogger(), BeatmapLevelsModel_CreateAllLoadedBeatmapLevelPacks);
        INSTALL_HOOK_ORIG(getLogger(), OculusPlatformAdditionalContentModel_GetLevelEntitlementStatusAsync);
        INSTALL_HOOK_ORIG(getLogger(), OculusPlatformAdditionalContentModel_GetPackEntitlementStatusAsync);
        INSTALL_HOOK_ORIG(getLogger(), SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels);
        INSTALL_HOOK_ORIG(getLogger(), FileHelpers_GetEscapedURLForFilePath);
        INSTALL_HOOK_ORIG(getLogger(), StandardLevelInfoSaveData_DeserializeFromJSONString);
        INSTALL_HOOK_ORIG(getLogger(), BeatmapLevelLoader_HandleItemWillBeRemovedFromCache);
        INSTALL_HOOK_ORIG(getLogger(), LevelFilteringNavigationController_UpdateCustomSongs);
        INSTALL_HOOK_ORIG(getLogger(), LevelFilteringNavigationController_UpdateSecondChildControllerContent);
        INSTALL_HOOK_ORIG(getLogger(), EnvironmentsAsyncInstaller_InstallBindings);
        INSTALL_HOOK_ORIG(getLogger(), BeatmapLevelsModel_LoadBeatmapLevelDataAsync);
        INSTALL_HOOK_ORIG(getLogger(), BeatmapLevelsModel_CheckBeatmapLevelDataExistsAsync);
    }
}