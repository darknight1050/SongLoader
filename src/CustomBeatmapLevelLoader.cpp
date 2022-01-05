#include "CustomBeatmapLevelLoader.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "CustomLogger.hpp"

#include "Utils/FileUtils.hpp"
#include "Utils/FindComponentsUtils.hpp"

#include "GlobalNamespace/FileHelpers.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmap.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapDataLoader.hpp"
#include "GlobalNamespace/BeatmapLevelData.hpp"
#include "GlobalNamespace/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmap.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/BeatmapDifficultySerializedMethods.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollectionSO.hpp"
#include "GlobalNamespace/AsyncCachedLoader_2.hpp"
#include "GlobalNamespace/HMCache_2.hpp"
#include "GlobalNamespace/HMTask.hpp"
#include "GlobalNamespace/AudioClipAsyncLoader.hpp"
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
#include "System/Threading/Tasks/Task_1.hpp"

#include <vector>
#include <mutex>

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace UnityEngine::Networking;
using namespace System::IO;
using namespace System::Threading;
using namespace System::Threading::Tasks;

namespace RuntimeSongLoader::CustomBeatmapLevelLoader {

    using namespace FindComponentsUtils;

    std::vector<std::function<void(CustomJSONData::CustomLevelInfoSaveData*, const std::string&, BeatmapData*)>> BeatmapDataLoadedEvents;
    std::mutex BeatmapDataLoadedEventsMutex;

    void AddBeatmapDataLoadedEvent(const std::function<void(CustomJSONData::CustomLevelInfoSaveData*, const std::string&, BeatmapData*)>& event) {
        std::lock_guard<std::mutex> lock(BeatmapDataLoadedEventsMutex);
        BeatmapDataLoadedEvents.push_back(event);
    }
    
    BeatmapData* LoadBeatmapData(const std::string& customLevelPath, const std::string& difficultyFileName, CustomJSONData::CustomLevelInfoSaveData* standardLevelInfoSaveData) {
        LOG_DEBUG("LoadBeatmapData Start");
        std::string path = customLevelPath + "/" + difficultyFileName;
        BeatmapData* beatmapData = nullptr;
        if(fileexists(path)) {
            Il2CppString* json = il2cpp_utils::newcsstr(FileUtils::ReadAllText16(path));

            //Temporary fix because exceptions don't work
            auto optional = il2cpp_utils::RunMethod<BeatmapData*>(BeatmapDataLoader::New_ctor(), "GetBeatmapDataFromJson", json, standardLevelInfoSaveData->beatsPerMinute, standardLevelInfoSaveData->shuffle, standardLevelInfoSaveData->shufflePeriod);
            if(optional.has_value()) {
                beatmapData = *optional;
            } else {
                LOG_ERROR("LoadBeatmapData File %s is corrupted!", (path).c_str());
            }
            //return BeatmapDataLoader::New_ctor()->GetBeatmapDataFromJson(json, standardLevelInfoSaveData->beatsPerMinute, standardLevelInfoSaveData->shuffle, standardLevelInfoSaveData->shufflePeriod);
        
        } else {
            LOG_ERROR("LoadBeatmapData File %s doesn't exist!", (path).c_str());
        }
        std::lock_guard<std::mutex> lock(BeatmapDataLoadedEventsMutex);
        for (auto& event : BeatmapDataLoadedEvents) {
            event(standardLevelInfoSaveData, difficultyFileName, beatmapData);
        }
        return beatmapData;
    }

    CustomDifficultyBeatmap* LoadDifficultyBeatmap(const std::string& customLevelPath, CustomBeatmapLevel* parentCustomBeatmapLevel, CustomDifficultyBeatmapSet* parentDifficultyBeatmapSet, CustomJSONData::CustomLevelInfoSaveData* standardLevelInfoSaveData, CustomJSONData::CustomDifficultyBeatmap* difficultyBeatmapSaveData) {
        LOG_DEBUG("LoadDifficultyBeatmapAsync Start");
        BeatmapData* beatmapData = LoadBeatmapData(customLevelPath, to_utf8(csstrtostr(difficultyBeatmapSaveData->beatmapFilename)), standardLevelInfoSaveData);
        if(!beatmapData)
            return nullptr;
        BeatmapDifficulty difficulty;
        BeatmapDifficultySerializedMethods::BeatmapDifficultyFromSerializedName(difficultyBeatmapSaveData->difficulty, byref(difficulty));
        LOG_DEBUG("LoadDifficultyBeatmapAsync Stop");
        return CustomDifficultyBeatmap::New_ctor(reinterpret_cast<IBeatmapLevel*>(parentCustomBeatmapLevel), reinterpret_cast<IDifficultyBeatmapSet*>(parentDifficultyBeatmapSet), difficulty, difficultyBeatmapSaveData->difficultyRank, difficultyBeatmapSaveData->noteJumpMovementSpeed, difficultyBeatmapSaveData->noteJumpStartBeatOffset, beatmapData);
    }

    IDifficultyBeatmapSet* LoadDifficultyBeatmapSet(std::string const& customLevelPath, CustomBeatmapLevel* customBeatmapLevel, CustomJSONData::CustomLevelInfoSaveData* standardLevelInfoSaveData, StandardLevelInfoSaveData::DifficultyBeatmapSet* difficultyBeatmapSetSaveData) {
        LOG_DEBUG("LoadDifficultyBeatmapSetAsync Start");
        if(!GetCustomLevelLoader()->beatmapCharacteristicCollection || !difficultyBeatmapSetSaveData || !difficultyBeatmapSetSaveData->beatmapCharacteristicName || !difficultyBeatmapSetSaveData->difficultyBeatmaps) return nullptr;
        BeatmapCharacteristicSO* beatmapCharacteristicBySerializedName = GetCustomLevelLoader()->beatmapCharacteristicCollection->GetBeatmapCharacteristicBySerializedName(difficultyBeatmapSetSaveData->beatmapCharacteristicName);
        ArrayW<CustomDifficultyBeatmap*> difficultyBeatmaps = ArrayW<CustomDifficultyBeatmap*>(difficultyBeatmapSetSaveData->difficultyBeatmaps.Length());
        CustomDifficultyBeatmapSet* difficultyBeatmapSet = CustomDifficultyBeatmapSet::New_ctor(beatmapCharacteristicBySerializedName);
        for(int i = 0; i < difficultyBeatmapSetSaveData->difficultyBeatmaps.Length(); i++) {
            auto beatmap = il2cpp_utils::cast<CustomJSONData::CustomDifficultyBeatmap>(difficultyBeatmapSetSaveData->difficultyBeatmaps[i]);
            if (!beatmap) continue;
            CustomDifficultyBeatmap* customDifficultyBeatmap = LoadDifficultyBeatmap(customLevelPath, customBeatmapLevel, difficultyBeatmapSet, standardLevelInfoSaveData, beatmap);
            if(!customDifficultyBeatmap)
                return nullptr;
            difficultyBeatmaps[i] = customDifficultyBeatmap;
        }
        difficultyBeatmapSet->SetCustomDifficultyBeatmaps(difficultyBeatmaps);
        LOG_DEBUG("LoadDifficultyBeatmapSetAsync Stop");
        return reinterpret_cast<IDifficultyBeatmapSet*>(difficultyBeatmapSet);
    }

    Array<IDifficultyBeatmapSet*>* LoadDifficultyBeatmapSets(std::string const& customLevelPath, CustomBeatmapLevel* customBeatmapLevel, CustomJSONData::CustomLevelInfoSaveData* standardLevelInfoSaveData) {
        LOG_DEBUG("LoadDifficultyBeatmapSetsAsync Start");
        ArrayW<IDifficultyBeatmapSet*> difficultyBeatmapSets = ArrayW<IDifficultyBeatmapSet*>(standardLevelInfoSaveData->difficultyBeatmapSets.Length());
        for(int i = 0; i < difficultyBeatmapSets.Length(); i++) {
            IDifficultyBeatmapSet* difficultyBeatmapSet = LoadDifficultyBeatmapSet(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData, standardLevelInfoSaveData->difficultyBeatmapSets[i]);
            if(!difficultyBeatmapSet)
                return nullptr;
            difficultyBeatmapSets[i] = difficultyBeatmapSet;
        }
        LOG_DEBUG("LoadDifficultyBeatmapSetsAsync Stop");
        return (Array<IDifficultyBeatmapSet*>*) difficultyBeatmapSets;
    }

    BeatmapLevelData* LoadBeatmapLevelData(std::string const& customLevelPath, CustomBeatmapLevel* customBeatmapLevel, CustomJSONData::CustomLevelInfoSaveData* standardLevelInfoSaveData) {
        LOG_DEBUG("LoadBeatmapLevelDataAsync Start");
        ArrayW<IDifficultyBeatmapSet*> difficultyBeatmapSets = LoadDifficultyBeatmapSets(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData);
        if(!difficultyBeatmapSets)
            return nullptr;
        auto task = GetBeatmapLevelsModel()->audioClipAsyncLoader->LoadSong(reinterpret_cast<IBeatmapLevel*>(customBeatmapLevel));
        while(!task->get_IsCompleted()) {
            usleep(10 * 1000);
        }
        AudioClip* audioClip = task->get_Result();
        if(!audioClip)
            return nullptr;
        LOG_DEBUG("LoadBeatmapLevelDataAsync Stop");
        return BeatmapLevelData::New_ctor(audioClip, difficultyBeatmapSets);
    }

    CustomBeatmapLevel* LoadCustomBeatmapLevel(CustomPreviewBeatmapLevel* customPreviewBeatmapLevel) {
        LOG_DEBUG("LoadCustomBeatmapLevel Start");
        auto* standardLevelInfoSaveData = il2cpp_utils::cast<CustomJSONData::CustomLevelInfoSaveData>(customPreviewBeatmapLevel->standardLevelInfoSaveData);
        std::string customLevelPath = to_utf8(csstrtostr(customPreviewBeatmapLevel->customLevelPath));
        CustomBeatmapLevel* customBeatmapLevel = CustomBeatmapLevel::New_ctor(customPreviewBeatmapLevel);
        BeatmapLevelData* beatmapLevelData = LoadBeatmapLevelData(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData);
        if(!beatmapLevelData)
            return nullptr;
        customBeatmapLevel->SetBeatmapLevelData(beatmapLevelData);
        LOG_DEBUG("LoadCustomBeatmapLevel Stop");
        return customBeatmapLevel;
    }

    MAKE_HOOK_MATCH(BeatmapLevelsModel_GetBeatmapLevelAsync, &BeatmapLevelsModel::GetBeatmapLevelAsync, Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>*, BeatmapLevelsModel* self, Il2CppString* levelID, CancellationToken cancellationToken) {
        LOG_INFO("BeatmapLevelsModel_GetBeatmapLevelAsync Start %s", to_utf8(csstrtostr(levelID)).c_str());
        Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>* result = BeatmapLevelsModel_GetBeatmapLevelAsync(self, levelID, cancellationToken);
        if(result->get_IsCompleted() && result->get_Result().isError) {
            if(self->loadedPreviewBeatmapLevels->ContainsKey(levelID)) {
                IPreviewBeatmapLevel* previewBeatmapLevel = self->loadedPreviewBeatmapLevels->get_Item(levelID);
                LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync previewBeatmapLevel %p", previewBeatmapLevel);
                if(il2cpp_functions::class_is_assignable_from(classof(CustomPreviewBeatmapLevel*), il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(previewBeatmapLevel)))) {
                    auto task = Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>::New_ctor();
                    HMTask::New_ctor(il2cpp_utils::MakeDelegate<System::Action*>(classof(System::Action*),
                        (std::function<void()>)[=] { 
                            LOG_INFO("BeatmapLevelsModel_GetBeatmapLevelAsync Thread Start");
                            CustomBeatmapLevel* customBeatmapLevel = CustomBeatmapLevelLoader::LoadCustomBeatmapLevel(reinterpret_cast<CustomPreviewBeatmapLevel*>(previewBeatmapLevel));
                            if(customBeatmapLevel && customBeatmapLevel->beatmapLevelData) {
                                self->loadedBeatmapLevels->PutToCache(levelID, reinterpret_cast<IBeatmapLevel*>(customBeatmapLevel));
                                task->TrySetResult(BeatmapLevelsModel::GetBeatmapLevelResult(false, reinterpret_cast<IBeatmapLevel*>(customBeatmapLevel)));
                            } else {
                                task->TrySetResult(BeatmapLevelsModel::GetBeatmapLevelResult(true, nullptr));
                            }
                            LOG_INFO("BeatmapLevelsModel_GetBeatmapLevelAsync Thread Stop");
                        }
                    ), nullptr)->Run();
                    return task;
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