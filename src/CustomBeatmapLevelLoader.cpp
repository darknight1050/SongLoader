#include "CustomBeatmapLevelLoader.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "custom-types/shared/delegate.hpp"

#include "CustomLogger.hpp"

#include "Utils/FileUtils.hpp"
#include "Utils/FindComponentsUtils.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "bsml/shared/Helpers/getters.hpp"

#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/FileHelpers.hpp"
#include "GlobalNamespace/BeatmapDataLoader.hpp"
#include "GlobalNamespace/BeatmapLevelData.hpp"
#include "GlobalNamespace/BeatmapDataBasicInfo.hpp"
#include "GlobalNamespace/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmap.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/BeatmapDifficultySerializedMethods.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollectionSO.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollection.hpp"
#include "GlobalNamespace/AsyncCachedLoader_2.hpp"
#include "GlobalNamespace/HMCache_2.hpp"
#include "GlobalNamespace/AudioClipAsyncLoader.hpp"
#include "BeatmapSaveDataVersion3/BeatmapSaveData.hpp"
#include "UnityEngine/Networking/UnityWebRequestAsyncOperation.hpp"
#include "UnityEngine/Networking/UnityWebRequest.hpp"
#include "UnityEngine/Networking/UnityWebRequestMultimedia.hpp"
#include "UnityEngine/Networking/DownloadHandlerAudioClip.hpp"
#include "UnityEngine/AudioType.hpp"
#include "UnityEngine/AudioClip.hpp"
#include "System/Action.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include "System/IO/Path.hpp"
#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/CancellationTokenSource.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Threading/Tasks/Task.hpp"

#include <vector>
#include <mutex>

using namespace GlobalNamespace;
using namespace BeatmapSaveDataVersion3;
using namespace UnityEngine;
using namespace UnityEngine::Networking;
using namespace System::IO;
using namespace System::Threading;
using namespace System::Threading::Tasks;

namespace RuntimeSongLoader::CustomBeatmapLevelLoader {

    using namespace FindComponentsUtils;

    std::vector<std::function<void(CustomJSONData::CustomLevelInfoSaveData*, std::string const&, BeatmapSaveData*, BeatmapDataBasicInfo*)>> BeatmapDataBasicInfoLoadedEvents;
    std::mutex BeatmapDataBasicInfoLoadedEventsMutex;

    void AddBeatmapDataBasicInfoLoadedEvent(std::function<void(CustomJSONData::CustomLevelInfoSaveData*, std::string const&, BeatmapSaveData*, BeatmapDataBasicInfo*)> const& event) {
        std::lock_guard<std::mutex> lock(BeatmapDataBasicInfoLoadedEventsMutex);
        BeatmapDataBasicInfoLoadedEvents.push_back(event);
    }

    bool LoadBeatmapDataBasicInfo(std::string const& customLevelPath, std::string const& difficultyFileName, CustomJSONData::CustomLevelInfoSaveData* standardLevelInfoSaveData, BeatmapSaveData*& beatmapSaveData, BeatmapDataBasicInfo*& beatmapDataBasicInfo) {
        static auto logger = getLogger().WithContext("LoadBeatmapDataBasicInfo");

        LOG_DEBUG("LoadBeatmapDataBasicInfo Start");
        std::string path = customLevelPath + "/" + difficultyFileName;
        if(fileexists(path)) {
            try {
                beatmapSaveData = BeatmapSaveData::DeserializeFromJSONString(FileUtils::ReadAllText16(path));
                beatmapDataBasicInfo = BeatmapDataLoader::GetBeatmapDataBasicInfoFromSaveData(beatmapSaveData);
                std::lock_guard<std::mutex> lock(BeatmapDataBasicInfoLoadedEventsMutex);
                for (auto& event : BeatmapDataBasicInfoLoadedEvents) {
                    event(standardLevelInfoSaveData, difficultyFileName, beatmapSaveData, beatmapDataBasicInfo);
                }
                return true;
            } catch(const std::runtime_error& e) {
                LOG_ERROR("LoadBeatmapDataBasicInfo Can't Load File %s: %s!", (path).c_str(), e.what());
            }
        } else {
            LOG_ERROR("LoadBeatmapDataBasicInfo File %s doesn't exist!", (path).c_str());
        }

        return false;
    }

    CustomDifficultyBeatmap* LoadDifficultyBeatmap(std::string const& customLevelPath, CustomBeatmapLevel* parentCustomBeatmapLevel, CustomDifficultyBeatmapSet* parentDifficultyBeatmapSet, CustomJSONData::CustomLevelInfoSaveData* standardLevelInfoSaveData, CustomJSONData::CustomDifficultyBeatmap* difficultyBeatmapSaveData) {
        static auto logger = getLogger().WithContext("LoadDifficultyBeatmap");
        LOG_DEBUG("LoadDifficultyBeatmapAsync Start");
        BeatmapSaveData* beatmapSaveData = nullptr;
        BeatmapDataBasicInfo* beatmapDataBasicInfo = nullptr;
        RET_0_UNLESS(logger, LoadBeatmapDataBasicInfo(customLevelPath, difficultyBeatmapSaveData->beatmapFilename, standardLevelInfoSaveData, beatmapSaveData, beatmapDataBasicInfo));

        RET_0_UNLESS(logger, beatmapSaveData);
        RET_0_UNLESS(logger, beatmapDataBasicInfo);

        BeatmapDifficulty difficulty;
        BeatmapDifficultySerializedMethods::BeatmapDifficultyFromSerializedName(difficultyBeatmapSaveData->difficulty, byref(difficulty));

        LOG_DEBUG("LoadDifficultyBeatmapAsync Stop");

        return CustomDifficultyBeatmap::New_ctor(
            reinterpret_cast<IBeatmapLevel*>(parentCustomBeatmapLevel),
            reinterpret_cast<IDifficultyBeatmapSet*>(parentDifficultyBeatmapSet),
            difficulty,
            difficultyBeatmapSaveData->difficultyRank,
            difficultyBeatmapSaveData->noteJumpMovementSpeed,
            difficultyBeatmapSaveData->noteJumpStartBeatOffset,
            standardLevelInfoSaveData->beatsPerMinute,
            difficultyBeatmapSaveData->beatmapColorSchemeIdx,
            difficultyBeatmapSaveData->environmentNameIdx,
            beatmapSaveData,
            reinterpret_cast<IBeatmapDataBasicInfo*>(beatmapDataBasicInfo)
        );
    }

    IDifficultyBeatmapSet* LoadDifficultyBeatmapSet(std::string const& customLevelPath, CustomBeatmapLevel* customBeatmapLevel, CustomJSONData::CustomLevelInfoSaveData* standardLevelInfoSaveData, StandardLevelInfoSaveData::DifficultyBeatmapSet* difficultyBeatmapSetSaveData) {
        static auto logger = getLogger().WithContext("LoadDifficultyBeatmapSet");
        LOG_DEBUG("LoadDifficultyBeatmapSetAsync Start");
        // TODO: check whether this works, since CustomLevelLoader doesn't have any meaningful fields anymore
        auto beatmapCharacteristicCollection = BSML::Helpers::GetDiContainer()->TryResolve<BeatmapCharacteristicCollection*>();

        RET_0_UNLESS(logger, beatmapCharacteristicCollection);
        RET_0_UNLESS(logger, difficultyBeatmapSetSaveData);
        RET_0_UNLESS(logger, difficultyBeatmapSetSaveData->beatmapCharacteristicName);
        RET_0_UNLESS(logger, difficultyBeatmapSetSaveData->difficultyBeatmaps);

        BeatmapCharacteristicSO* beatmapCharacteristicBySerializedName = beatmapCharacteristicCollection->GetBeatmapCharacteristicBySerializedName(difficultyBeatmapSetSaveData->beatmapCharacteristicName);
        ArrayW<CustomDifficultyBeatmap*> difficultyBeatmaps = ArrayW<CustomDifficultyBeatmap*>(difficultyBeatmapSetSaveData->difficultyBeatmaps.size());
        CustomDifficultyBeatmapSet* difficultyBeatmapSet = CustomDifficultyBeatmapSet::New_ctor(beatmapCharacteristicBySerializedName);
        for(int i = 0; i < difficultyBeatmapSetSaveData->difficultyBeatmaps.size(); i++) {
            auto beatmap = il2cpp_utils::try_cast<CustomJSONData::CustomDifficultyBeatmap>(difficultyBeatmapSetSaveData->difficultyBeatmaps[i]).value_or(nullptr);
            if (!beatmap) continue;

            CustomDifficultyBeatmap* customDifficultyBeatmap = LoadDifficultyBeatmap(customLevelPath, customBeatmapLevel, difficultyBeatmapSet, standardLevelInfoSaveData, beatmap);
            RET_0_UNLESS(logger, customDifficultyBeatmap);

            difficultyBeatmaps[i] = customDifficultyBeatmap;
        }
        difficultyBeatmapSet->SetCustomDifficultyBeatmaps(difficultyBeatmaps);
        LOG_DEBUG("LoadDifficultyBeatmapSetAsync Stop");
        return reinterpret_cast<IDifficultyBeatmapSet*>(difficultyBeatmapSet);
    }

    ArrayW<IDifficultyBeatmapSet*> LoadDifficultyBeatmapSets(std::string const& customLevelPath, CustomBeatmapLevel* customBeatmapLevel, CustomJSONData::CustomLevelInfoSaveData* standardLevelInfoSaveData) {
        static auto logger = getLogger().WithContext("LoadDifficultyBeatmapSets");
        LOG_DEBUG("LoadDifficultyBeatmapSetsAsync Start");

        auto difficultyBeatmapSets = ArrayW<IDifficultyBeatmapSet*>(standardLevelInfoSaveData->difficultyBeatmapSets.size());
        for(int i = 0; i < difficultyBeatmapSets.size(); i++) {
            IDifficultyBeatmapSet* difficultyBeatmapSet = LoadDifficultyBeatmapSet(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData, standardLevelInfoSaveData->difficultyBeatmapSets[i]);
            RET_0_UNLESS(logger, difficultyBeatmapSet);
            difficultyBeatmapSets[i] = difficultyBeatmapSet;
        }

        LOG_DEBUG("LoadDifficultyBeatmapSetsAsync Stop");
        return difficultyBeatmapSets;
    }

    BeatmapLevelData* LoadBeatmapLevelData(std::string const& customLevelPath, CustomBeatmapLevel* customBeatmapLevel, CustomJSONData::CustomLevelInfoSaveData* standardLevelInfoSaveData) {
        static auto logger = getLogger().WithContext("LoadBeatmapLevelData");
        auto difficultyBeatmapSets = LoadDifficultyBeatmapSets(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData);
        RET_0_UNLESS(logger, difficultyBeatmapSets);

        auto task = GetBeatmapLevelsModel()->_audioClipAsyncLoader->LoadSong(customBeatmapLevel->i___GlobalNamespace__IBeatmapLevel());
        while(!task->IsCompleted) { std::this_thread::yield(); }

        AudioClip* audioClip = task->get_Result();
        RET_0_UNLESS(logger, audioClip);

        LOG_DEBUG("LoadBeatmapLevelDataAsync Stop");
        return BeatmapLevelData::New_ctor(audioClip, reinterpret_cast<::System::Collections::Generic::IReadOnlyList_1<IDifficultyBeatmapSet*>*>(difficultyBeatmapSets.convert()));
    }

    CustomBeatmapLevel* LoadCustomBeatmapLevel(CustomPreviewBeatmapLevel* customPreviewBeatmapLevel) {
        static auto logger = getLogger().WithContext("LoadCustomBeatmapLevel");
        auto* standardLevelInfoSaveData = il2cpp_utils::try_cast<CustomJSONData::CustomLevelInfoSaveData>(customPreviewBeatmapLevel->standardLevelInfoSaveData).value_or(nullptr);
        RET_0_UNLESS(logger, standardLevelInfoSaveData);

        CustomBeatmapLevel* customBeatmapLevel = CustomBeatmapLevel::New_ctor(customPreviewBeatmapLevel);
        BeatmapLevelData* beatmapLevelData = LoadBeatmapLevelData(customPreviewBeatmapLevel->customLevelPath, customBeatmapLevel, standardLevelInfoSaveData);
        RET_0_UNLESS(logger, beatmapLevelData);

        customBeatmapLevel->SetBeatmapLevelData(beatmapLevelData);
        return customBeatmapLevel;
    }

    template<typename Ret, typename T>
    requires(std::is_invocable_r_v<Ret, T>)
    void task_func(System::Threading::Tasks::Task_1<Ret>* task, T func) {
        task->TrySetResult(std::invoke(func));
    }

    template<typename Ret, typename T>
    requires(std::is_invocable_r_v<Ret, T>)
    void task_cancel_func(System::Threading::Tasks::Task_1<Ret>* task, T func, System::Threading::CancellationToken cancelToken) {
        auto value = std::invoke(func);
        if (!cancelToken.IsCancellationRequested) {
            task->TrySetResult(std::invoke(func));
        } else {
            task->TrySetCanceled(cancelToken);
        }
    }

    template<typename Ret, typename T>
    requires(!std::is_same_v<Ret, void> && std::is_invocable_r_v<Ret, T>)
    System::Threading::Tasks::Task_1<Ret>* StartTask(T func) {
        auto t = System::Threading::Tasks::Task_1<Ret>::New_ctor();
        il2cpp_utils::il2cpp_aware_thread(&task_func<Ret, T>, t, func).detach();
        return t;
    }

    template<typename Ret, typename T>
    requires(!std::is_same_v<Ret, void> && std::is_invocable_r_v<Ret, T>)
    System::Threading::Tasks::Task_1<Ret>* StartTask(T func, System::Threading::CancellationToken cancelToken) {
        auto t = System::Threading::Tasks::Task_1<Ret>::New_ctor();
        il2cpp_utils::il2cpp_aware_thread(&task_cancel_func<Ret, T>, t, func, cancelToken).detach();
        return t;
    }

    MAKE_HOOK_MATCH(BeatmapLevelsModel_GetBeatmapLevelAsync, &BeatmapLevelsModel::GetBeatmapLevelAsync, Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>*, BeatmapLevelsModel* self, StringW levelID, CancellationToken cancellationToken) {
        auto cppLevelId = levelID ? static_cast<std::string>(levelID) : "";

        LOG_INFO("BeatmapLevelsModel_GetBeatmapLevelAsync Start %s", cppLevelId.c_str());

        Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>* result = BeatmapLevelsModel_GetBeatmapLevelAsync(self, levelID, cancellationToken);
        if(result->get_IsCompleted() && result->get_Result().isError) {
            if(self->_loadedPreviewBeatmapLevels->ContainsKey(levelID)) {
                IPreviewBeatmapLevel* previewBeatmapLevel = self->_loadedPreviewBeatmapLevels->get_Item(levelID);
                auto customPreviewBeatmapLevel = il2cpp_utils::try_cast<CustomPreviewBeatmapLevel>(previewBeatmapLevel).value_or(nullptr);

                LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync previewBeatmapLevel %p", previewBeatmapLevel);
                if (customPreviewBeatmapLevel) {
                    return StartTask<BeatmapLevelsModel::GetBeatmapLevelResult>([=]{
                        LOG_INFO("BeatmapLevelsModel_GetBeatmapLevelAsync Thread Start");
                        CustomBeatmapLevel* customBeatmapLevel = CustomBeatmapLevelLoader::LoadCustomBeatmapLevel(customPreviewBeatmapLevel);
                        auto result = BeatmapLevelsModel::GetBeatmapLevelResult(true, nullptr);
                        LOG_DEBUG("level: %p", customBeatmapLevel);
                        LOG_DEBUG("levelData: %p", customBeatmapLevel ? customBeatmapLevel->beatmapLevelData : nullptr);

                        if(customBeatmapLevel && customBeatmapLevel->beatmapLevelData) {
                            BSML::MainThreadScheduler::Schedule(
                                [=] {
                                    try {
                                        self->_loadedBeatmapLevels->PutToCache(levelID, reinterpret_cast<IBeatmapLevel*>(customBeatmapLevel));
                                    } catch (std::runtime_error const& e) {
                                        getLogger().Backtrace(20);
                                        LOG_ERROR("CustomBeatmapLevelLoader_GetBeatmapLevelAsync Failed to put (%s) to cache: %s!", static_cast<std::string>(levelID).c_str(), e.what());
                                    }
                                }
                            );

                        }

                        return BeatmapLevelsModel::GetBeatmapLevelResult(false, customBeatmapLevel->i___GlobalNamespace__IBeatmapLevel());
                    }, cancellationToken);
                }
            }
        }
        LOG_INFO("BeatmapLevelsModel_GetBeatmapLevelAsync Stop");
        return result;
    }

    void InstallHooks() {
        INSTALL_HOOK(getLogger(), BeatmapLevelsModel_GetBeatmapLevelAsync);
    }

}
