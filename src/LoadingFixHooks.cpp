#include "LoadingFixHooks.hpp"

#include "beatsaber-hook/shared/utils/typedefs.h"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "CustomLogger.hpp"

#include "Paths.hpp"

#include "LevelData.hpp"

#include "Utils/FindComponentsUtils.hpp"

#include "CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"
#include "CustomTypes/CustomLevelInfoSaveData.hpp"

#include "GlobalNamespace/AdditionalContentModel.hpp"
#include "GlobalNamespace/SinglePlayerLevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapLevelCollection.hpp"
#include "GlobalNamespace/LevelSearchViewController.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/BeatmapDataTransformHelper.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/BeatmapObjectData.hpp"
#include "GlobalNamespace/BeatmapLineData.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/BeatmapLevelPack.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollection.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollectionSO.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollectionContainerSO.hpp"
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
#include "GlobalNamespace/HMCache_2.hpp"
#include "GlobalNamespace/FileHelpers.hpp"
#include "GlobalNamespace/MainSettingsModelSO.hpp"
#include "GlobalNamespace/BeatmapObjectSpawnMovementData.hpp"
#include "GlobalNamespace/IJumpOffsetYProvider.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IAnnotatedBeatmapLevelCollection.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/StandardLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/MultiplayerLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/MissionLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/BoolSO.hpp"
#include "GlobalNamespace/BeatmapSaveDataHelpers.hpp"
#include "GlobalNamespace/PlayerSensitivityFlag.hpp"
#include "GlobalNamespace/EntitlementStatus.hpp"
#include "GlobalNamespace/IBeatmapLevelPack.hpp"
#include "GlobalNamespace/IBeatmapLevelCollection.hpp"
#include "GlobalNamespace/IBeatmapLevelPackCollection.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"

#include "System/Version.hpp"
#include "System/Action_2.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Linq/Enumerable.hpp"

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

    MAKE_HOOK_MATCH(BeatmapSaveDataHelpers_GetVersion, &BeatmapSaveDataHelpers::GetVersion, System::Version*, StringW data) {
        LOG_DEBUG("BeatmapSaveDataHelpers_GetVersion");
        auto truncatedText = data.operator std::string().substr(0, 50);
        static const std::regex versionRegex (R"("_?version"\s*:\s*"[0-9]+\.[0-9]+\.?[0-9]?")", std::regex_constants::optimize);
        std::smatch matches;
        if(std::regex_search(truncatedText, matches, versionRegex)) {
            if(!matches.empty()) {
                auto version = matches[0].str();
                version = version.substr(0, version.length()-1);
                version = version.substr(version.find_last_of('\"')+1, version.length());
                try {
                    return System::Version::New_ctor(version);
                } catch(const std::runtime_error& e) {
                    LOG_INFO("BeatmapSaveDataHelpers_GetVersion Invalid version: \"%s\"!", version.c_str());
                }
            }
        }
        return System::Version::New_ctor("2.0.0");
    }

    MAKE_HOOK_MATCH(BeatmapDataTransformHelper_CreateTransformedBeatmapData, &BeatmapDataTransformHelper::CreateTransformedBeatmapData, IReadonlyBeatmapData*, IReadonlyBeatmapData* beatmapData, IPreviewBeatmapLevel* beatmapLevel, GameplayModifiers* gameplayModifiers, bool leftHanded, EnvironmentEffectsFilterPreset environmentEffectsFilterPreset, EnvironmentIntensityReductionOptions* environmentIntensityReductionOptions, MainSettingsModelSO* mainSettingsModel) {
        LOG_DEBUG("BeatmapDataTransformHelper_CreateTransformedBeatmapData");
        // BeatGames, why did you put the effort into making this not work on Quest IF CUSTOM LEVELS ARE NOT EVEN LOADED IN THE FIRST PLACE BASEGAME.
        // THERE WAS NO POINT IN CHANGING THE IF STATEMENT SPECIFICALLY FOR QUEST
        // Sincerely, a quest developer
        bool& screenDisplacementEffectsEnabled = mainSettingsModel->screenDisplacementEffectsEnabled->_value;
        bool oldScreenDisplacementEffectsEnabled = screenDisplacementEffectsEnabled;

        if(beatmapLevel->get_levelID().starts_with(CustomLevelPrefixID))
            screenDisplacementEffectsEnabled = screenDisplacementEffectsEnabled || beatmapLevel->get_levelID().starts_with(CustomLevelPrefixID);

        auto result = BeatmapDataTransformHelper_CreateTransformedBeatmapData(
            beatmapData, beatmapLevel, gameplayModifiers, leftHanded,
            environmentEffectsFilterPreset, environmentIntensityReductionOptions,
            mainSettingsModel);

        screenDisplacementEffectsEnabled = oldScreenDisplacementEffectsEnabled;

        return result;
    }

    // TODO: Use a hook that works when fixed
    MAKE_HOOK_MATCH(CustomBeatmapLevel_ctor, &CustomBeatmapLevel::_ctor, void, CustomBeatmapLevel* self, CustomPreviewBeatmapLevel* customPreviewBeatmapLevel) {
        LOG_DEBUG("CustomBeatmapLevel_ctor");
        CustomBeatmapLevel_ctor(self, customPreviewBeatmapLevel);
        self->_songDuration_k__BackingField = customPreviewBeatmapLevel->songDuration;
    }

    MAKE_HOOK_MATCH(StandardLevelScenesTransitionSetupDataSO_Init, &StandardLevelScenesTransitionSetupDataSO::Init, void, StandardLevelScenesTransitionSetupDataSO* self, ::StringW  gameMode, ::GlobalNamespace::IDifficultyBeatmap*  difficultyBeatmap, ::GlobalNamespace::IPreviewBeatmapLevel*  previewBeatmapLevel, ::GlobalNamespace::OverrideEnvironmentSettings*  overrideEnvironmentSettings, ::GlobalNamespace::ColorScheme*  overrideColorScheme, ::GlobalNamespace::ColorScheme*  beatmapOverrideColorScheme, ::GlobalNamespace::GameplayModifiers*  gameplayModifiers, ::GlobalNamespace::PlayerSpecificSettings*  playerSpecificSettings, ::GlobalNamespace::PracticeSettings*  practiceSettings, ::StringW  backButtonText, bool  useTestNoteCutSoundEffects, bool  startPaused, ::GlobalNamespace::BeatmapDataCache*  beatmapDataCache, ::System::Nullable_1<::GlobalNamespace::RecordingToolManager::SetupData>  recordingToolData) {
        LOG_DEBUG("StandardLevelScenesTransitionSetupDataSO_Init");
        StandardLevelScenesTransitionSetupDataSO_Init(self, gameMode, difficultyBeatmap, previewBeatmapLevel, overrideEnvironmentSettings, overrideColorScheme, beatmapOverrideColorScheme, gameplayModifiers, playerSpecificSettings, practiceSettings, backButtonText, useTestNoteCutSoundEffects, startPaused, beatmapDataCache, recordingToolData);
        LevelData::difficultyBeatmap = difficultyBeatmap;
    }

    MAKE_HOOK_MATCH(MultiplayerLevelScenesTransitionSetupDataSO_Init, &MultiplayerLevelScenesTransitionSetupDataSO::Init, void, MultiplayerLevelScenesTransitionSetupDataSO* self, StringW gameMode, IPreviewBeatmapLevel* previewBeatmapLevel, BeatmapDifficulty beatmapDifficulty, BeatmapCharacteristicSO* beatmapCharacteristic, IDifficultyBeatmap* difficultyBeatmap, ColorScheme* overrideColorScheme, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, PracticeSettings* practiceSettings, bool useTestNoteCutSoundEffects) {
        LOG_DEBUG("MultiplayerLevelScenesTransitionSetupDataSO_Init");
        MultiplayerLevelScenesTransitionSetupDataSO_Init(self, gameMode, previewBeatmapLevel, beatmapDifficulty, beatmapCharacteristic, difficultyBeatmap, overrideColorScheme, gameplayModifiers, playerSpecificSettings, practiceSettings, useTestNoteCutSoundEffects);
        LevelData::difficultyBeatmap = difficultyBeatmap;
    }

    MAKE_HOOK_MATCH(MissionLevelScenesTransitionSetupDataSO_Init, &MissionLevelScenesTransitionSetupDataSO::Init, void, MissionLevelScenesTransitionSetupDataSO* self, StringW missionId, IDifficultyBeatmap* difficultyBeatmap, IPreviewBeatmapLevel* previewBeatmapLevel, ArrayW<MissionObjective*> missionObjectives, ColorScheme* overrideColorScheme, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, StringW backButtonText) {
        LOG_DEBUG("MissionLevelScenesTransitionSetupDataSO_Init");
        MissionLevelScenesTransitionSetupDataSO_Init(self, missionId, difficultyBeatmap, previewBeatmapLevel, missionObjectives, overrideColorScheme, gameplayModifiers, playerSpecificSettings, backButtonText);
        LevelData::difficultyBeatmap = difficultyBeatmap;
    }

    // https://github.com/Kylemc1413/SongCore/blob/master/source/SongCore/HarmonyPatches/NegativeNjsPatch.cs
    // patch to allow negative NJS
    MAKE_HOOK_MATCH(BeatmapObjectSpawnMovementData_Init, &BeatmapObjectSpawnMovementData::Init, void, BeatmapObjectSpawnMovementData* self, int noteLinesCount, float startNoteJumpMovementSpeed, float startBpm, BeatmapObjectSpawnMovementData::NoteJumpValueType noteJumpValueType, float noteJumpValue, IJumpOffsetYProvider* jumpOffsetYProvider, UnityEngine::Vector3 rightVec, UnityEngine::Vector3 forwardVec) {
        LOG_DEBUG("BeatmapObjectSpawnMovementData_Init");
        if(LevelData::difficultyBeatmap) {
            auto noteJumpMovementSpeed = LevelData::difficultyBeatmap->get_noteJumpMovementSpeed();
            if(noteJumpMovementSpeed < 0)
                startNoteJumpMovementSpeed = noteJumpMovementSpeed;
        }
        BeatmapObjectSpawnMovementData_Init(self, noteLinesCount, startNoteJumpMovementSpeed, startBpm, noteJumpValueType, noteJumpValue, jumpOffsetYProvider, rightVec, forwardVec);
    }

    MAKE_HOOK_MATCH(LevelSearchViewController_UpdateBeatmapLevelPackCollectionAsync, &LevelSearchViewController::UpdateBeatmapLevelPackCollectionAsync, void, LevelSearchViewController* self) {
        LOG_DEBUG("LevelSearchViewController_UpdateBeatmapLevelPackCollectionAsync");
        static ConstString filterName("allSongs");
        auto newLevels = ListW<IPreviewBeatmapLevel*>::New();
        auto levelPacks = self->_beatmapLevelPacks;
        if(levelPacks.size() != 1 || levelPacks[0]->packID != filterName) {
            for(auto levelPack : levelPacks) {
                GlobalNamespace::IAnnotatedBeatmapLevelCollection* levelCollection = *levelPack;
                ArrayW<GlobalNamespace::IPreviewBeatmapLevel*> levels{levelCollection->beatmapLevelCollection->beatmapLevels};
                for(auto level : levels) {
                    if(!newLevels->Contains(level))
                        newLevels->Add(level);
                }
            }

            BeatmapLevelCollection* beatmapLevelCollection = BeatmapLevelCollection::New_ctor({});
            BeatmapLevelPack* beatmapLevelPack = BeatmapLevelPack::New_ctor(filterName, filterName, filterName, nullptr, nullptr, reinterpret_cast<IBeatmapLevelCollection*>(beatmapLevelCollection), ::GlobalNamespace::PlayerSensitivityFlag::Unknown);
            self->_beatmapLevelPacks = ArrayW<IBeatmapLevelPack*>(1);
            self->_beatmapLevelPacks[0] = *beatmapLevelPack;
            beatmapLevelCollection->_levels = reinterpret_cast<IReadOnlyList_1<IPreviewBeatmapLevel*>*>(newLevels.convert());
        }
        LevelSearchViewController_UpdateBeatmapLevelPackCollectionAsync(self);
    }

    MAKE_HOOK_MATCH(BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync, &BeatmapLevelsModel::ReloadCustomLevelPackCollectionAsync, Task_1<IBeatmapLevelPackCollection*>*, BeatmapLevelsModel* self, CancellationToken cancellationToken) {
        LOG_DEBUG("BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync");
        return Task_1<IBeatmapLevelPackCollection*>::New_ctor(self->customLevelPackCollection);
    }

    MAKE_HOOK_MATCH(BeatmapLevelsModel_UpdateAllLoadedBeatmapLevelPacks, &BeatmapLevelsModel::UpdateAllLoadedBeatmapLevelPacks, void, BeatmapLevelsModel* self) {
        LOG_DEBUG("BeatmapLevelsModel_UpdateAllLoadedBeatmapLevelPacks");
        auto list = ListW<IBeatmapLevelPack*>::New();
        if(self->ostAndExtrasPackCollection)
            list->AddRange(reinterpret_cast<IEnumerable_1<IBeatmapLevelPack*>*>(self->ostAndExtrasPackCollection->get_beatmapLevelPacks().convert()));
        if(self->_dlcLevelPackCollectionContainer && self->_dlcLevelPackCollectionContainer->_beatmapLevelPackCollection)
            list->AddRange(reinterpret_cast<IEnumerable_1<IBeatmapLevelPack*>*>(self->_dlcLevelPackCollectionContainer->_beatmapLevelPackCollection->get_beatmapLevelPacks().convert()));
        self->_allLoadedBeatmapLevelWithoutCustomLevelPackCollection = reinterpret_cast<IBeatmapLevelPackCollection*>(BeatmapLevelPackCollection::New_ctor(list->ToArray()));
        if(self->customLevelPackCollection)
            list->AddRange(reinterpret_cast<IEnumerable_1<IBeatmapLevelPack*>*>(self->customLevelPackCollection->get_beatmapLevelPacks().convert()));
        self->_allLoadedBeatmapLevelPackCollection = reinterpret_cast<IBeatmapLevelPackCollection*>(BeatmapLevelPackCollection::New_ctor(list->ToArray()));
    }

    MAKE_HOOK_MATCH(AdditionalContentModel_GetLevelEntitlementStatusAsync, &AdditionalContentModel::GetLevelEntitlementStatusAsync, Task_1<EntitlementStatus>*, AdditionalContentModel* self, StringW levelId, CancellationToken cancellationToken) {
        std::string levelIdCpp = levelId;
        LOG_DEBUG("AdditionalContentModel_GetLevelEntitlementStatusAsync %s", levelIdCpp.c_str());
        if(levelId.starts_with(CustomLevelPrefixID)) {
            auto beatmapLevelsModel = FindComponentsUtils::GetBeatmapLevelsModel();
            bool loaded = beatmapLevelsModel->_loadedPreviewBeatmapLevels->ContainsKey(levelId) || beatmapLevelsModel->_loadedBeatmapLevels->IsInCache(levelId);
            return Task_1<EntitlementStatus>::New_ctor(loaded ? EntitlementStatus::Owned : EntitlementStatus::NotOwned);
        }
        return AdditionalContentModel_GetLevelEntitlementStatusAsync(self, levelId, cancellationToken);
    }

    MAKE_HOOK_MATCH(AdditionalContentModel_GetPackEntitlementStatusAsync, &AdditionalContentModel::GetPackEntitlementStatusAsync, Task_1<EntitlementStatus>*, AdditionalContentModel* self, StringW levelPackId, CancellationToken cancellationToken) {
        std::string levelPackIdCpp = levelPackId;
        LOG_DEBUG("AdditionalContentModel_GetPackEntitlementStatusAsync %s", levelPackIdCpp.c_str());

        if(levelPackId.starts_with(CustomLevelPackPrefixID))
            return Task_1<EntitlementStatus>::New_ctor(EntitlementStatus::Owned);
        return AdditionalContentModel_GetPackEntitlementStatusAsync(self, levelPackId, cancellationToken);
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

        SafePtr<GlobalNamespace::StandardLevelInfoSaveData> original;

        // replacing the 2.x.x version with 2.0.0 is assumed safe because
        // minor/patch versions should NOT break the schema and therefore deemed
        // readable by RSL even if the new fields are not parsed
        // short circuit
        // 1.0.0 and 2.0.0 are supported by basegame through string equality
        // checks if (!stringData->Contains("1.0.0") &&
        // !stringData->Contains("2.0.0")) { https://regex101.com/r/jJAvvE/3
        // Verified result: https://godbolt.org/z/MvfW3eh7q
        // Checks if version is 2.0.0 range and then replaces it with 2.0.0
        // for compatibility
        static const std::regex versionRegex(
            R"(\"_version\"\s*:\s*\"(2\.\d\.\d)\")",
            std::regex_constants::ECMAScript | std::regex_constants::optimize);

        std::smatch matches;
        std::string cppStr(stringData);

        std::string sub(cppStr.substr(0, 100));

        LOG_DEBUG("First 100 chars from json: %s", sub.c_str());

        if (std::regex_search(cppStr, matches, versionRegex)) {
            // Does not match supported version
            if (matches.size() >= 1) {

                // match group is index 1 because we're matching for (2.x.x)
                auto badVersion = matches[1].str();
                LOG_DEBUG("Performing fixup for version %s", badVersion.c_str());

                // mutates the string does not copy
                cppStr.replace(matches[1].first, matches[1].second, "2.0.0");

                original = StandardLevelInfoSaveData_DeserializeFromJSONString(
                    StringW(cppStr));
            }
        }
        // }

        if (!original || !original.ptr()) {
            LOG_DEBUG("No fixup performed for map version");
            original = StandardLevelInfoSaveData_DeserializeFromJSONString(
                stringData);
        }

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

    void InstallHooks() {
        INSTALL_HOOK_ORIG(getLogger(), BeatmapSaveDataHelpers_GetVersion);
        INSTALL_HOOK(getLogger(), BeatmapDataTransformHelper_CreateTransformedBeatmapData);
        INSTALL_HOOK_ORIG(getLogger(), CustomBeatmapLevel_ctor);
        INSTALL_HOOK_ORIG(getLogger(), StandardLevelScenesTransitionSetupDataSO_Init);
        INSTALL_HOOK_ORIG(getLogger(), MultiplayerLevelScenesTransitionSetupDataSO_Init);
        INSTALL_HOOK_ORIG(getLogger(), MissionLevelScenesTransitionSetupDataSO_Init);
        INSTALL_HOOK_ORIG(getLogger(), BeatmapObjectSpawnMovementData_Init);
        INSTALL_HOOK_ORIG(getLogger(), LevelSearchViewController_UpdateBeatmapLevelPackCollectionAsync);
        INSTALL_HOOK_ORIG(getLogger(), BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync);
        INSTALL_HOOK_ORIG(getLogger(), BeatmapLevelsModel_UpdateAllLoadedBeatmapLevelPacks);
        INSTALL_HOOK_ORIG(getLogger(), AdditionalContentModel_GetLevelEntitlementStatusAsync);
        INSTALL_HOOK_ORIG(getLogger(), AdditionalContentModel_GetPackEntitlementStatusAsync);
        INSTALL_HOOK_ORIG(getLogger(), SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels);
        INSTALL_HOOK_ORIG(getLogger(), FileHelpers_GetEscapedURLForFilePath);
        INSTALL_HOOK_ORIG(getLogger(), StandardLevelInfoSaveData_DeserializeFromJSONString);
    }

}
