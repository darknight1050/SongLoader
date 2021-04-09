#include "CustomBeatmapLevelLoader.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "CustomLogger.hpp"

#include "Utils/FileUtils.hpp"
#include "Utils/FindComponentsUtils.hpp"

#include "GlobalNamespace/FileHelpers.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmap.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
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

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace UnityEngine::Networking;
using namespace System::IO;
using namespace System::Threading;
using namespace System::Threading::Tasks;

namespace RuntimeSongLoader::CustomBeatmapLevelLoader {

    using namespace FindComponentsUtils;
    
    AudioClip* GetPreviewAudioClip(CustomPreviewBeatmapLevel* customPreviewBeatmapLevel) {
        auto start = std::chrono::high_resolution_clock::now(); 
        LOG_DEBUG("GetPreviewAudioClipAsync Start %p", customPreviewBeatmapLevel->previewAudioClip);
        auto songFilename = customPreviewBeatmapLevel->standardLevelInfoSaveData->songFilename;
        if(!customPreviewBeatmapLevel->previewAudioClip && !System::String::IsNullOrEmpty(songFilename)) {
            Il2CppString* path = Path::Combine(customPreviewBeatmapLevel->customLevelPath, songFilename);
            /*AudioType audioType = (to_utf8(csstrtostr(Path::GetExtension(path)->ToLower())) == ".wav") ? AudioType::WAV : AudioType::OGGVORBIS;
            UnityWebRequest* www = UnityWebRequestMultimedia::GetAudioClip(FileHelpers::GetEscapedURLForFilePath(path), audioType);
            ((DownloadHandlerAudioClip*)www->m_DownloadHandler)->set_streamAudio(true);
            UnityWebRequestAsyncOperation* request = www->SendWebRequest();
            while(!request->get_isDone()) {
                LOG_DEBUG("GetPreviewAudioClipAsync Delay");
                usleep(10 * 1000);
            }
            LOG_DEBUG("GetPreviewAudioClipAsync ErrorStatus %d %d", www->get_isHttpError(), www->get_isNetworkError());
            if(!www->get_isHttpError() && !www->get_isNetworkError())
                customPreviewBeatmapLevel->previewAudioClip = DownloadHandlerAudioClip::GetContent(www);*/
            auto task = GetCachedMediaAsyncLoader()->LoadAudioClipAsync(path, CancellationToken::get_None());
            while(!task->get_IsCompleted()) {
                LOG_DEBUG("GetPreviewAudioClipAsync Delay");
                usleep(10 * 1000);
            }
            customPreviewBeatmapLevel->previewAudioClip = task->get_Result();
        }
        std::chrono::milliseconds duration = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start); 
        LOG_DEBUG("GetPreviewAudioClipAsync Stop %p Time %d", customPreviewBeatmapLevel->previewAudioClip, duration);
        return customPreviewBeatmapLevel->previewAudioClip;
    }

    BeatmapData* LoadBeatmapData(std::string customLevelPath, std::string difficultyFileName, StandardLevelInfoSaveData* standardLevelInfoSaveData) {
        LOG_DEBUG("LoadBeatmapData Start");
        BeatmapData* data = nullptr;
        std::string path = customLevelPath + "/" + difficultyFileName;
        if(fileexists(path)) {
            Il2CppString* json = il2cpp_utils::createcsstr(FileUtils::ReadAllText(path));
            data = BeatmapDataLoader::New_ctor()->GetBeatmapDataFromJson(json, standardLevelInfoSaveData->beatsPerMinute, standardLevelInfoSaveData->shuffle, standardLevelInfoSaveData->shufflePeriod);
        }
        LOG_DEBUG("LoadBeatmapData Stop");
        return data;
    }

    CustomDifficultyBeatmap* LoadDifficultyBeatmap(std::string customLevelPath, CustomBeatmapLevel* parentCustomBeatmapLevel, CustomDifficultyBeatmapSet* parentDifficultyBeatmapSet, StandardLevelInfoSaveData* standardLevelInfoSaveData, StandardLevelInfoSaveData::DifficultyBeatmap* difficultyBeatmapSaveData) {
        LOG_DEBUG("LoadDifficultyBeatmapAsync Start");
        BeatmapData* beatmapData = LoadBeatmapData(customLevelPath, to_utf8(csstrtostr(difficultyBeatmapSaveData->beatmapFilename)), standardLevelInfoSaveData);
        BeatmapDifficulty difficulty;
        BeatmapDifficultySerializedMethods::BeatmapDifficultyFromSerializedName(difficultyBeatmapSaveData->difficulty, difficulty);
        LOG_DEBUG("LoadDifficultyBeatmapAsync Stop");
        return CustomDifficultyBeatmap::New_ctor(reinterpret_cast<IBeatmapLevel*>(parentCustomBeatmapLevel), reinterpret_cast<IDifficultyBeatmapSet*>(parentDifficultyBeatmapSet), difficulty, difficultyBeatmapSaveData->difficultyRank, difficultyBeatmapSaveData->noteJumpMovementSpeed, difficultyBeatmapSaveData->noteJumpStartBeatOffset, beatmapData);
    }

    IDifficultyBeatmapSet* LoadDifficultyBeatmapSet(std::string customLevelPath, CustomBeatmapLevel* customBeatmapLevel, StandardLevelInfoSaveData* standardLevelInfoSaveData, StandardLevelInfoSaveData::DifficultyBeatmapSet* difficultyBeatmapSetSaveData) {
        LOG_DEBUG("LoadDifficultyBeatmapSetAsync Start");
        BeatmapCharacteristicSO* beatmapCharacteristicBySerializedName = GetCustomLevelLoader()->beatmapCharacteristicCollection->GetBeatmapCharacteristicBySerializedName(difficultyBeatmapSetSaveData->beatmapCharacteristicName);
        Array<CustomDifficultyBeatmap*>* difficultyBeatmaps = Array<CustomDifficultyBeatmap*>::NewLength(difficultyBeatmapSetSaveData->difficultyBeatmaps->Length());
        CustomDifficultyBeatmapSet* difficultyBeatmapSet = CustomDifficultyBeatmapSet::New_ctor(beatmapCharacteristicBySerializedName);
        for(int i = 0; i < difficultyBeatmapSetSaveData->difficultyBeatmaps->Length(); i++) {
            CustomDifficultyBeatmap* customDifficultyBeatmap = LoadDifficultyBeatmap(customLevelPath, customBeatmapLevel, difficultyBeatmapSet, standardLevelInfoSaveData, difficultyBeatmapSetSaveData->difficultyBeatmaps->values[i]);
            difficultyBeatmaps->values[i] = customDifficultyBeatmap;
        }
        difficultyBeatmapSet->SetCustomDifficultyBeatmaps(difficultyBeatmaps);
        LOG_DEBUG("LoadDifficultyBeatmapSetAsync Stop");
        return reinterpret_cast<IDifficultyBeatmapSet*>(difficultyBeatmapSet);
    }

    Array<IDifficultyBeatmapSet*>* LoadDifficultyBeatmapSets(std::string customLevelPath, CustomBeatmapLevel* customBeatmapLevel, StandardLevelInfoSaveData* standardLevelInfoSaveData) {
        LOG_DEBUG("LoadDifficultyBeatmapSetsAsync Start");
        Array<IDifficultyBeatmapSet*>* difficultyBeatmapSets = Array<IDifficultyBeatmapSet*>::NewLength(standardLevelInfoSaveData->difficultyBeatmapSets->Length());
        for(int i = 0; i < difficultyBeatmapSets->Length(); i++) {
            IDifficultyBeatmapSet* difficultyBeatmapSet = LoadDifficultyBeatmapSet(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData, standardLevelInfoSaveData->difficultyBeatmapSets->values[i]);
            difficultyBeatmapSets->values[i] = difficultyBeatmapSet;
        }
        LOG_DEBUG("LoadDifficultyBeatmapSetsAsync Stop");
        return difficultyBeatmapSets;
    }

    BeatmapLevelData* LoadBeatmapLevelData(std::string customLevelPath, CustomBeatmapLevel* customBeatmapLevel, StandardLevelInfoSaveData* standardLevelInfoSaveData) {
        LOG_DEBUG("LoadBeatmapLevelDataAsync Start");
        Array<IDifficultyBeatmapSet*>* difficultyBeatmapSets = LoadDifficultyBeatmapSets(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData);
        AudioClip* audioClip = GetPreviewAudioClip(reinterpret_cast<CustomPreviewBeatmapLevel*>(customBeatmapLevel));
        if(!audioClip)
            return nullptr;
        LOG_DEBUG("LoadBeatmapLevelDataAsync Stop");
        return BeatmapLevelData::New_ctor(audioClip, difficultyBeatmapSets);
    }

    CustomBeatmapLevel* LoadCustomBeatmapLevel(CustomPreviewBeatmapLevel* customPreviewBeatmapLevel) {
        LOG_DEBUG("LoadCustomBeatmapLevel Start");
        StandardLevelInfoSaveData* standardLevelInfoSaveData = customPreviewBeatmapLevel->standardLevelInfoSaveData;
        std::string customLevelPath = to_utf8(csstrtostr(customPreviewBeatmapLevel->customLevelPath));
        AudioClip* previewAudioClip = GetPreviewAudioClip(customPreviewBeatmapLevel);
        CustomBeatmapLevel* customBeatmapLevel = CustomBeatmapLevel::New_ctor(customPreviewBeatmapLevel, previewAudioClip);
        BeatmapLevelData* beatmapLevelData = LoadBeatmapLevelData(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData);
        customBeatmapLevel->SetBeatmapLevelData(beatmapLevelData);
        LOG_DEBUG("LoadCustomBeatmapLevel Stop");
        return customBeatmapLevel;
    }

    MAKE_HOOK_OFFSETLESS(BeatmapLevelsModel_GetBeatmapLevelAsync, Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>*, BeatmapLevelsModel* self, Il2CppString* levelID, CancellationToken cancellationToken) {
        LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync Start %s", to_utf8(csstrtostr(levelID)).c_str());
        Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>* result = BeatmapLevelsModel_GetBeatmapLevelAsync(self, levelID, cancellationToken);
        if(result->get_IsCompleted() && result->get_Result().isError) {
            IPreviewBeatmapLevel* previewBeatmapLevel = self->loadedPreviewBeatmapLevels->get_Item(levelID);
            if(previewBeatmapLevel) {
                LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync previewBeatmapLevel %p", previewBeatmapLevel);
                if(il2cpp_functions::class_is_assignable_from(classof(CustomPreviewBeatmapLevel*), il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(previewBeatmapLevel)))) {
                    auto task = Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>::New_ctor();
                    HMTask::New_ctor(il2cpp_utils::MakeDelegate<System::Action*>(classof(System::Action*),
                        (std::function<void()>)[=] { 
                            CustomBeatmapLevel* customBeatmapLevel = CustomBeatmapLevelLoader::LoadCustomBeatmapLevel(reinterpret_cast<CustomPreviewBeatmapLevel*>(previewBeatmapLevel));
                            LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync Thread Start");
                            if(customBeatmapLevel && customBeatmapLevel->beatmapLevelData) {
                                self->loadedBeatmapLevels->PutToCache(levelID, reinterpret_cast<IBeatmapLevel*>(customBeatmapLevel));
                                task->TrySetResult(BeatmapLevelsModel::GetBeatmapLevelResult(false, reinterpret_cast<IBeatmapLevel*>(customBeatmapLevel)));
                            } else {
                                task->TrySetResult(BeatmapLevelsModel::GetBeatmapLevelResult(true, nullptr));
                            }
                            LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync Thread Stop");
                    }), nullptr)->Run();
                    return task;
                }
            }
        }
        LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync Stop");
        return result;
    }

    void InstallHooks() {
        INSTALL_HOOK_OFFSETLESS(getLogger(), BeatmapLevelsModel_GetBeatmapLevelAsync, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "GetBeatmapLevelAsync", 2));
    }
    
}