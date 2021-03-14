#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "custom-types/shared/register.hpp"

#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/AdditionalContentModel.hpp"
#include "GlobalNamespace/SongPackMask.hpp"
#include "GlobalNamespace/SinglePlayerLevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
#include "GlobalNamespace/LevelSearchViewController.hpp"
#include "GlobalNamespace/CustomLevelLoader.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/FileHelpers.hpp"
#include "GlobalNamespace/AlwaysOwnedContentContainerSO.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmap.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmapSet.hpp"
#include "GlobalNamespace/PreviewDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/BeatmapLevelData.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/BeatmapDifficultySerializedMethods.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollectionSO.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollectionContainerSO.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmap.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp"
#include "GlobalNamespace/CustomBeatmapLevelCollection.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollection.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollectionSO.hpp"
#include "GlobalNamespace/BeatmapDataLoader.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/EnvironmentsListSO.hpp"
#include "GlobalNamespace/CachedMediaAsyncLoader.hpp"
#include "GlobalNamespace/IAudioClipAsyncLoader.hpp"
#include "GlobalNamespace/ISpriteAsyncLoader.hpp"
#include "GlobalNamespace/HMCache_2.hpp"
#include "GlobalNamespace/HMTask.hpp"
#include "UnityEngine/SceneManagement/Scene.hpp"
#include "UnityEngine/Networking/UnityWebRequestAsyncOperation.hpp"
#include "UnityEngine/Networking/UnityWebRequest.hpp"
#include "UnityEngine/Networking/UnityWebRequestMultimedia.hpp"
#include "UnityEngine/Networking/DownloadHandlerAudioClip.hpp"
#include "UnityEngine/AudioType.hpp"
#include "UnityEngine/AudioClip.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Vector4.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/SpriteMeshType.hpp"
#include "System/Action.hpp"
#include "System/String.hpp"
#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/CancellationTokenSource.hpp"
#include "System/Threading/Tasks/Task.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Threading/Tasks/TaskStatus.hpp"
#include "System/IO/Path.hpp"
#include "System/IO/File.hpp"
#include "System/IO/Directory.hpp"
#include "System/IO/DirectoryInfo.hpp"
#include "System/Collections/Generic/HashSet_1.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include "System/Security/Cryptography/SHA1.hpp"
#include "System/BitConverter.hpp"
#include "System/Convert.hpp"
#include "System/Collections/Generic/List_1.hpp"
#include "System/Linq/Enumerable.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/TextureFormat.hpp"
#include "UnityEngine/ImageConversion.hpp"

#include "CustomLogger.hpp"
#include "LoadingFixHooks.hpp"
#include "CustomCharacteristics.hpp"
#include "Sprites.hpp"
#include "Paths.hpp"
#include "Utils/ArrayUtil.hpp"
#include "Utils/HashUtils.hpp"
#include "Utils/FileUtils.hpp"
#include "Utils/FindComponentsUtils.hpp"

#include "CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp"

#include <unistd.h>
#include <chrono>

static ModInfo modInfo;

Logger& getLogger() {
    static auto logger = new Logger(modInfo, LoggerOptions(false, true)); 
    return *logger; 
}

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace UnityEngine::Networking;
using namespace System::Collections::Generic;
using namespace System::Collections;
using namespace System::IO;
using namespace System::Threading;
using namespace Tasks;
using namespace FindComponentsUtils;

//From questui: https://github.com/darknight1050/questui
inline Sprite* Base64ToSprite(std::string base64, int width, int height) {
    Array<uint8_t>* bytes = System::Convert::FromBase64String(il2cpp_utils::createcsstr(base64));
    Texture2D* texture = Texture2D::New_ctor(width, height, TextureFormat::RGBA32, false, false);
    if(ImageConversion::LoadImage(texture, bytes, false))
        return Sprite::Create(texture, UnityEngine::Rect(0.0f, 0.0f, (float)width, (float)height), UnityEngine::Vector2(0.5f,0.5f), 1024.0f, 1u, SpriteMeshType::FullRect, UnityEngine::Vector4(0.0f, 0.0f, 0.0f, 0.0f), false);
    return nullptr;
}

void SetupCustomCharacteristics() {
    static bool created = false;
    if(!created) {
        created = true;
        CustomCharacteristics::RegisterCustomCharacteristic(Base64ToSprite(Sprites::MissingBase64, 128, 128), "Missing Characteristic", "Missing Characteristic", "MissingCharacteristic", "MissingCharacteristic", false, false, 1000);
        CustomCharacteristics::RegisterCustomCharacteristic(Base64ToSprite(Sprites::LightshowBase64, 128, 128), "Lightshow", "Lightshow", "Lightshow", "Lightshow", false, false, 100);
        CustomCharacteristics::RegisterCustomCharacteristic(Base64ToSprite(Sprites::LawlessBase64, 128, 128), "Lawless", "Lawless - Anything Goes", "Lawless", "Lawless", false, false, 101);
    }
}

using namespace RuntimeSongLoader;
#include "CustomTypes/SongLoader.hpp"

BeatmapData* LoadBeatmapDataBeatmapData(std::string customLevelPath, std::string difficultyFileName, StandardLevelInfoSaveData* standardLevelInfoSaveData) {
    LOG_DEBUG("LoadBeatmapDataBeatmapData Start");
    BeatmapData* data = nullptr;
    std::string path = customLevelPath + "/" + difficultyFileName;
    if(fileexists(path)) {
        Il2CppString* json = il2cpp_utils::createcsstr(FileUtils::ReadAllText(path));
        data = BeatmapDataLoader::New_ctor()->GetBeatmapDataFromJson(json, standardLevelInfoSaveData->beatsPerMinute, standardLevelInfoSaveData->shuffle, standardLevelInfoSaveData->shufflePeriod);
    }
    LOG_DEBUG("LoadBeatmapDataBeatmapData Stop");
    return data;
}

CustomDifficultyBeatmap* LoadDifficultyBeatmapAsync(std::string customLevelPath, CustomBeatmapLevel* parentCustomBeatmapLevel, CustomDifficultyBeatmapSet* parentDifficultyBeatmapSet, StandardLevelInfoSaveData* standardLevelInfoSaveData, StandardLevelInfoSaveData::DifficultyBeatmap* difficultyBeatmapSaveData) {
    LOG_DEBUG("LoadDifficultyBeatmapAsync Start");
    BeatmapData* beatmapData = LoadBeatmapDataBeatmapData(customLevelPath, to_utf8(csstrtostr(difficultyBeatmapSaveData->beatmapFilename)), standardLevelInfoSaveData);
    BeatmapDifficulty difficulty;
    BeatmapDifficultySerializedMethods::BeatmapDifficultyFromSerializedName(difficultyBeatmapSaveData->difficulty, difficulty);
    LOG_DEBUG("LoadDifficultyBeatmapAsync Stop");
    return CustomDifficultyBeatmap::New_ctor(reinterpret_cast<IBeatmapLevel*>(parentCustomBeatmapLevel), reinterpret_cast<IDifficultyBeatmapSet*>(parentDifficultyBeatmapSet), difficulty, difficultyBeatmapSaveData->difficultyRank, difficultyBeatmapSaveData->noteJumpMovementSpeed, difficultyBeatmapSaveData->noteJumpStartBeatOffset, beatmapData);
}

IDifficultyBeatmapSet* LoadDifficultyBeatmapSetAsync(std::string customLevelPath, CustomBeatmapLevel* customBeatmapLevel, StandardLevelInfoSaveData* standardLevelInfoSaveData, StandardLevelInfoSaveData::DifficultyBeatmapSet* difficultyBeatmapSetSaveData) {
    LOG_DEBUG("LoadDifficultyBeatmapSetAsync Start");
    BeatmapCharacteristicSO* beatmapCharacteristicBySerializedName = GetCustomLevelLoader()->beatmapCharacteristicCollection->GetBeatmapCharacteristicBySerializedName(difficultyBeatmapSetSaveData->beatmapCharacteristicName);
    Array<CustomDifficultyBeatmap*>* difficultyBeatmaps = Array<CustomDifficultyBeatmap*>::NewLength(difficultyBeatmapSetSaveData->difficultyBeatmaps->Length());
    CustomDifficultyBeatmapSet* difficultyBeatmapSet = CustomDifficultyBeatmapSet::New_ctor(beatmapCharacteristicBySerializedName);
    for (int i = 0; i < difficultyBeatmapSetSaveData->difficultyBeatmaps->Length(); i++) {
        CustomDifficultyBeatmap* customDifficultyBeatmap = LoadDifficultyBeatmapAsync(customLevelPath, customBeatmapLevel, difficultyBeatmapSet, standardLevelInfoSaveData, difficultyBeatmapSetSaveData->difficultyBeatmaps->values[i]);
        difficultyBeatmaps->values[i] = customDifficultyBeatmap;
    }
    difficultyBeatmapSet->SetCustomDifficultyBeatmaps(difficultyBeatmaps);
    LOG_DEBUG("LoadDifficultyBeatmapSetAsync Stop");
    return reinterpret_cast<IDifficultyBeatmapSet*>(difficultyBeatmapSet);
}

Array<IDifficultyBeatmapSet*>* LoadDifficultyBeatmapSetsAsync(std::string customLevelPath, CustomBeatmapLevel* customBeatmapLevel, StandardLevelInfoSaveData* standardLevelInfoSaveData) {
    LOG_DEBUG("LoadDifficultyBeatmapSetsAsync Start");
    Array<IDifficultyBeatmapSet*>* difficultyBeatmapSets = Array<IDifficultyBeatmapSet*>::NewLength(standardLevelInfoSaveData->difficultyBeatmapSets->Length());
    for (int i = 0; i < difficultyBeatmapSets->Length(); i++) {
        IDifficultyBeatmapSet* difficultyBeatmapSet = LoadDifficultyBeatmapSetAsync(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData, standardLevelInfoSaveData->difficultyBeatmapSets->values[i]);
        difficultyBeatmapSets->values[i] = difficultyBeatmapSet;
    }
    LOG_DEBUG("LoadDifficultyBeatmapSetsAsync Stop");
    return difficultyBeatmapSets;
}

AudioClip* GetPreviewAudioClipAsync(CustomPreviewBeatmapLevel* customPreviewBeatmapLevel) {
    LOG_DEBUG("GetPreviewAudioClipAsync Start %p", customPreviewBeatmapLevel->previewAudioClip);
    if (!customPreviewBeatmapLevel->previewAudioClip && !System::String::IsNullOrEmpty(customPreviewBeatmapLevel->standardLevelInfoSaveData->songFilename)) {
        Il2CppString* path = Path::Combine(customPreviewBeatmapLevel->customLevelPath, customPreviewBeatmapLevel->standardLevelInfoSaveData->songFilename);
        AudioType audioType = (to_utf8(csstrtostr(Path::GetExtension(path)->ToLower())) == ".wav") ? AudioType::WAV : AudioType::OGGVORBIS;
        UnityWebRequest* www = UnityWebRequestMultimedia::GetAudioClip(FileHelpers::GetEscapedURLForFilePath(path), audioType);
        ((DownloadHandlerAudioClip*)www->m_DownloadHandler)->set_streamAudio(true);
        UnityWebRequestAsyncOperation* request = www->SendWebRequest();
        while (!request->get_isDone()) {
            LOG_DEBUG("GetPreviewAudioClipAsync Delay");
            usleep(100 * 1000);
        }
        LOG_DEBUG("GetPreviewAudioClipAsync ErrorStatus %d %d", www->get_isHttpError(), www->get_isNetworkError());
        if(!www->get_isHttpError() && !www->get_isNetworkError())
            customPreviewBeatmapLevel->previewAudioClip = DownloadHandlerAudioClip::GetContent(www);
    }
    LOG_DEBUG("GetPreviewAudioClipAsync Stop %p", customPreviewBeatmapLevel->previewAudioClip);
    return customPreviewBeatmapLevel->previewAudioClip;
}

BeatmapLevelData* LoadBeatmapLevelDataAsync(std::string customLevelPath, CustomBeatmapLevel* customBeatmapLevel, StandardLevelInfoSaveData* standardLevelInfoSaveData, CancellationToken cancellationToken) {
    LOG_DEBUG("LoadBeatmapLevelDataAsync Start");
    Array<IDifficultyBeatmapSet*>* difficultyBeatmapSets = LoadDifficultyBeatmapSetsAsync(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData);
    AudioClip* audioClip = GetPreviewAudioClipAsync(reinterpret_cast<CustomPreviewBeatmapLevel*>(customBeatmapLevel));
    if (!audioClip)
        return nullptr;
    LOG_DEBUG("LoadBeatmapLevelDataAsync Stop");
    return BeatmapLevelData::New_ctor(audioClip, difficultyBeatmapSets);
}

CustomBeatmapLevel* LoadCustomBeatmapLevelAsync(CustomPreviewBeatmapLevel* customPreviewBeatmapLevel, CancellationToken cancellationToken) {
    LOG_DEBUG("LoadCustomBeatmapLevelAsync Start");
    StandardLevelInfoSaveData* standardLevelInfoSaveData = customPreviewBeatmapLevel->standardLevelInfoSaveData;
    std::string customLevelPath = to_utf8(csstrtostr(customPreviewBeatmapLevel->customLevelPath));
    AudioClip* previewAudioClip = GetPreviewAudioClipAsync(customPreviewBeatmapLevel);
    CustomBeatmapLevel* customBeatmapLevel = CustomBeatmapLevel::New_ctor(customPreviewBeatmapLevel, previewAudioClip);
    BeatmapLevelData* beatmapLevelData = LoadBeatmapLevelDataAsync(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData, cancellationToken);
    customBeatmapLevel->SetBeatmapLevelData(beatmapLevelData);
    LOG_DEBUG("LoadCustomBeatmapLevelAsync Stop");
    return customBeatmapLevel;
}

MAKE_HOOK_OFFSETLESS(BeatmapLevelsModel_ClearLoadedBeatmapLevelsCaches, void, BeatmapLevelsModel* self) {
    LOG_DEBUG("BeatmapLevelsModel_ClearLoadedBeatmapLevelsCaches");
    if(GetCachedMediaAsyncLoader())
        GetCachedMediaAsyncLoader()->ClearCache();
    BeatmapLevelsModel_ClearLoadedBeatmapLevelsCaches(self);
}

MAKE_HOOK_OFFSETLESS(BeatmapLevelsModel_GetBeatmapLevelAsync, Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>*, BeatmapLevelsModel* self, Il2CppString* levelID, CancellationToken cancellationToken) {
    LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync Start %s", to_utf8(csstrtostr(levelID)).c_str());
    Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>* result = BeatmapLevelsModel_GetBeatmapLevelAsync(self, levelID, cancellationToken);
    if(result->get_Result().isError) {
        IPreviewBeatmapLevel* previewBeatmapLevel = self->loadedPreviewBeatmapLevels->get_Item_NEW(levelID);
        if(previewBeatmapLevel) {
            LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync previewBeatmapLevel %p", previewBeatmapLevel);
            if (il2cpp_functions::class_is_subclass_of(classof(CustomPreviewBeatmapLevel*), il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(previewBeatmapLevel)), true)) {
                auto task = Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>::New_ctor();
                HMTask::New_ctor(il2cpp_utils::MakeDelegate<System::Action*>(classof(System::Action*),
                    (std::function<void()>)[=] { 
                        CustomBeatmapLevel* customBeatmapLevel = LoadCustomBeatmapLevelAsync(reinterpret_cast<CustomPreviewBeatmapLevel*>(previewBeatmapLevel), cancellationToken);
                        LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync");
                        if (customBeatmapLevel && customBeatmapLevel->get_beatmapLevelData_NEW()) {
                            self->loadedBeatmapLevels->PutToCache(levelID, reinterpret_cast<IBeatmapLevel*>(customBeatmapLevel));
                            task->TrySetResult(BeatmapLevelsModel::GetBeatmapLevelResult(false, reinterpret_cast<IBeatmapLevel*>(customBeatmapLevel)));
                        } else {
                            LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync Stop");
                            task->TrySetResult(BeatmapLevelsModel::GetBeatmapLevelResult(true, nullptr));
                        }
                }), nullptr)->Run();
                return task;
            }
        }
    }
    LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync Stop");
    return result;
}

MAKE_HOOK_OFFSETLESS(SceneManager_Internal_ActiveSceneChanged, void, UnityEngine::SceneManagement::Scene prevScene, UnityEngine::SceneManagement::Scene nextScene) {
    SceneManager_Internal_ActiveSceneChanged(prevScene, nextScene);
    if(prevScene.IsValid() && nextScene.IsValid()) {
        std::string prevSceneName = to_utf8(csstrtostr(prevScene.get_name()));
        std::string nextSceneName = to_utf8(csstrtostr(nextScene.get_name()));
        static bool hasInited = false;
        if(prevSceneName == "QuestInit"){
            hasInited = true;
        }
        if(hasInited && prevSceneName == "EmptyTransition" && nextSceneName.find("Menu") != std::string::npos) {
            hasInited = false;

            SetupCustomCharacteristics();
            SongLoader* loader = SongLoader::GetInstance();
            HMTask::New_ctor(il2cpp_utils::MakeDelegate<System::Action*>(classof(System::Action*),
                (std::function<void()>)[=] { 
                auto start = std::chrono::high_resolution_clock::now(); 
                auto list = List<CustomPreviewBeatmapLevel*>::New_ctor();
                Array<Il2CppString*>* directories = Directory::GetDirectories(il2cpp_utils::createcsstr(CustomSongsFolder));
                for (int i = 0; i < directories->Length(); i++)
                {
                    std::string songPath = to_utf8(csstrtostr(Path::GetFullPath(directories->values[i])));
                    StandardLevelInfoSaveData* saveData = loader->GetStandardLevelInfoSaveData(songPath);
                    std::string hash;
                    auto startLevel = std::chrono::high_resolution_clock::now(); 
                    auto level = loader->LoadCustomPreviewBeatmapLevel(songPath, saveData, hash);
                    std::chrono::milliseconds durationLevel = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startLevel); 
                    LOG_INFO("Song loaded in %d milliseconds!", durationLevel);
                    if(level)
                        list->Add_NEW(level);
                }
                loader->CustomLevelsCollection->customPreviewBeatmapLevels = list->ToArray();
                loader->RefreshLevelPacks();
                std::chrono::milliseconds duration = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start); 
                LOG_INFO("RSL Loaded %d songs in %d milliseconds!", loader->CustomLevelsCollection->customPreviewBeatmapLevels->Length(), duration);
            }), nullptr)->Run();
        }
    }
}

extern "C" void setup(ModInfo& info) {
    modInfo.id = "SongLoader";
    modInfo.version = VERSION;
    info = modInfo;
}

extern "C" void load() {
    LOG_INFO("Starting SongLoader installation...");
    il2cpp_functions::Init();
    custom_types::Register::RegisterTypes<SongLoaderBeatmapLevelPackCollectionSO, SongLoader>();
    INSTALL_HOOK_OFFSETLESS(getLogger(), BeatmapLevelsModel_ClearLoadedBeatmapLevelsCaches, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "ClearLoadedBeatmapLevelsCaches", 0));
    INSTALL_HOOK_OFFSETLESS(getLogger(), BeatmapLevelsModel_GetBeatmapLevelAsync, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "GetBeatmapLevelAsync", 2));
    INSTALL_HOOK_OFFSETLESS(getLogger(), SceneManager_Internal_ActiveSceneChanged, il2cpp_utils::FindMethodUnsafe("UnityEngine.SceneManagement", "SceneManager", "Internal_ActiveSceneChanged", 2));
    LoadingFixHooks::InstallHooks();
    CustomCharacteristics::InstallHooks();
    LOG_INFO("Successfully installed SongLoader!");
}