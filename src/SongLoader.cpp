#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "custom-types/shared/register.hpp"

#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
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

#include "customlogger.hpp"
#include "CustomCharacteristics.hpp"
#include "Sprites.hpp"
#include "Utils/ArrayUtil.hpp"
#include "Utils/HashUtils.hpp"
#include "Utils/FileUtils.hpp"
#include "Utils/FindComponentsUtils.hpp"

#include "CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp"

#include <unistd.h>
#include <chrono>

const std::string CustomSongsFolder = "/sdcard/BeatSaberSongs";
const std::string CustomLevelPackPrefixID = "custom_level_";

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
class SongLoader {
    private:
        CustomBeatmapLevelCollection* CustomLevelsCollection = nullptr;
        CustomBeatmapLevelCollection* WIPLevelsCollection = nullptr;

        CustomBeatmapLevelPack* CustomLevelsPack = nullptr;
        CustomBeatmapLevelPack* WIPLevelsPack = nullptr;

        RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO* CustomBeatmapLevelPackCollectionSO = nullptr;

    public:
        void Setup();
        void RefreshLevelPacks();
        StandardLevelInfoSaveData* GetStandardLevelInfoSaveData(std::string customLevelPath);
        EnvironmentInfoSO* LoadEnvironmentInfo(Il2CppString* environmentName, bool allDirections);
        CustomPreviewBeatmapLevel* LoadCustomPreviewBeatmapLevel(std::string customLevelPath, StandardLevelInfoSaveData* standardLevelInfoSaveData, std::string& outHash);
};

void SongLoader::Setup() {
    if(CustomLevelsCollection)
        return;
    CustomLevelsCollection = CustomBeatmapLevelCollection::New_ctor(Array<CustomPreviewBeatmapLevel*>::NewLength(0));
    static auto customLevelsPackID = il2cpp_utils::createcsstr(CustomLevelPackPrefixID + "CustomLevels", il2cpp_utils::StringType::Manual);
    GetAlwaysOwnedContentContainerSO()->alwaysOwnedPacksIds->Add_NEW(customLevelsPackID);
    static auto customLevelsPackName = il2cpp_utils::createcsstr("Custom Levels", il2cpp_utils::StringType::Manual);
    CustomLevelsPack = CustomBeatmapLevelPack::New_ctor(customLevelsPackID, customLevelsPackName, customLevelsPackName, GetCustomLevelLoader()->defaultPackCover, CustomLevelsCollection);
    
    WIPLevelsCollection = CustomBeatmapLevelCollection::New_ctor(Array<CustomPreviewBeatmapLevel*>::NewLength(0));
    static auto WIPLevelsPackID = il2cpp_utils::createcsstr(CustomLevelPackPrefixID + "CustomWIPLevels", il2cpp_utils::StringType::Manual);
    GetAlwaysOwnedContentContainerSO()->alwaysOwnedPacksIds->Add_NEW(WIPLevelsPackID);
    static auto WIPLevelsPackName = il2cpp_utils::createcsstr("WIP Levels", il2cpp_utils::StringType::Manual);
    WIPLevelsPack = CustomBeatmapLevelPack::New_ctor(WIPLevelsPackID, WIPLevelsPackName, WIPLevelsPackName, GetCustomLevelLoader()->defaultPackCover, WIPLevelsCollection);
    
    CustomBeatmapLevelPackCollectionSO = RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO::CreateNew();
    CustomBeatmapLevelPackCollectionSO->AddLevelPack(CustomLevelsPack);
    CustomBeatmapLevelPackCollectionSO->AddLevelPack(WIPLevelsPack);

    HMTask::New_ctor(il2cpp_utils::MakeDelegate<System::Action*>(classof(System::Action*),
        (std::function<void()>)[=] { 
            auto list = List<CustomPreviewBeatmapLevel*>::New_ctor();
            Array<Il2CppString*>* directories = Directory::GetDirectories(il2cpp_utils::createcsstr(CustomSongsFolder));
            for (int i = 0; i < directories->Length(); i++)
            {
                std::string songPath = to_utf8(csstrtostr(Path::GetFullPath(directories->values[i])));
                StandardLevelInfoSaveData* saveData = GetStandardLevelInfoSaveData(songPath);
                std::string hash;
                auto level = LoadCustomPreviewBeatmapLevel(songPath, saveData, hash);
                if(level)
                    list->Add_NEW(level);
            }
            CustomLevelsCollection->customPreviewBeatmapLevels = list->ToArray();
            RefreshLevelPacks();
            Thread::Sleep(4000);
            directories = Directory::GetDirectories(il2cpp_utils::createcsstr("/sdcard/Songs2"));
            for (int i = 0; i < directories->Length(); i++)
            {
                std::string songPath = to_utf8(csstrtostr(Path::GetFullPath(directories->values[i])));
                StandardLevelInfoSaveData* saveData = GetStandardLevelInfoSaveData(songPath);
                std::string hash;
                auto level = LoadCustomPreviewBeatmapLevel(songPath, saveData, hash);
                if(level)
                    list->Add_NEW(level);
            }
            CustomLevelsCollection->customPreviewBeatmapLevels = list->ToArray();
            RefreshLevelPacks();
        }), nullptr)->Run();
    
}

void SongLoader::RefreshLevelPacks() {
    auto beatmapLevelsModel = GetBeatmapLevelsModel();
    beatmapLevelsModel->customLevelPackCollection = reinterpret_cast<IBeatmapLevelPackCollection*>(CustomBeatmapLevelPackCollectionSO);
    beatmapLevelsModel->UpdateLoadedPreviewLevels();
    auto levelFilteringNavigationController = ArrayUtil::First(Resources::FindObjectsOfTypeAll<LevelFilteringNavigationController*>());
    if(levelFilteringNavigationController && levelFilteringNavigationController->get_isActiveAndEnabled())
        levelFilteringNavigationController->UpdateCustomSongs();
    }

StandardLevelInfoSaveData* SongLoader::GetStandardLevelInfoSaveData(std::string customLevelPath) {
    std::string path = customLevelPath + "/info.dat";
    if(!fileexists(path))
        path = customLevelPath + "/Info.dat";
    if(fileexists(path))
        return StandardLevelInfoSaveData::DeserializeFromJSONString(il2cpp_utils::createcsstr(FileUtils::ReadAllText(path)));
    return nullptr;
}

EnvironmentInfoSO* SongLoader::LoadEnvironmentInfo(Il2CppString* environmentName, bool allDirections) {
    EnvironmentInfoSO* environmentInfoSO = GetCustomLevelLoader()->environmentSceneInfoCollection->GetEnviromentInfoBySerializedName(environmentName);
    if (!environmentInfoSO)
        environmentInfoSO = (allDirections ? GetCustomLevelLoader()->defaultAllDirectionsEnvironmentInfo : GetCustomLevelLoader()->defaultEnvironmentInfo);
    LOG_DEBUG("LoadEnvironmentInfo: %p", environmentInfoSO);
    return environmentInfoSO;
}

CustomPreviewBeatmapLevel* SongLoader::LoadCustomPreviewBeatmapLevel(std::string customLevelPath, StandardLevelInfoSaveData* standardLevelInfoSaveData, std::string& outHash) {
    if(!standardLevelInfoSaveData) 
        return nullptr;
    LOG_DEBUG("LoadCustomPreviewBeatmapLevel StandardLevelInfoSaveData: ");
    bool wip = customLevelPath.find("CustomWIPLevels") != std::string::npos;

    outHash = HashUtils::GetCustomLevelHash(standardLevelInfoSaveData, customLevelPath);
    std::string stringLevelID = CustomLevelPackPrefixID + outHash;
    if(wip)
        stringLevelID += " WIP";
    Il2CppString* levelID = il2cpp_utils::createcsstr(stringLevelID);
    Il2CppString* songName = standardLevelInfoSaveData->songName;
    Il2CppString* songSubName = standardLevelInfoSaveData->songSubName;
    Il2CppString* songAuthorName = standardLevelInfoSaveData->songAuthorName;
    Il2CppString* levelAuthorName = standardLevelInfoSaveData->levelAuthorName;
    float beatsPerMinute = standardLevelInfoSaveData->beatsPerMinute;
    float songTimeOffset = standardLevelInfoSaveData->songTimeOffset;
    float shuffle = standardLevelInfoSaveData->shuffle;
    float shufflePeriod = standardLevelInfoSaveData->shufflePeriod;
    float previewStartTime = standardLevelInfoSaveData->previewStartTime;
    float previewDuration = standardLevelInfoSaveData->previewDuration;
    LOG_DEBUG("levelID: %s", stringLevelID.c_str());
    LOG_DEBUG("songName: %s", to_utf8(csstrtostr(songName)).c_str());
    LOG_DEBUG("songSubName: %s", to_utf8(csstrtostr(songSubName)).c_str());
    LOG_DEBUG("songAuthorName: %s", to_utf8(csstrtostr(songAuthorName)).c_str());
    LOG_DEBUG("levelAuthorName: %s", to_utf8(csstrtostr(levelAuthorName)).c_str());
    LOG_DEBUG("beatsPerMinute: %f", beatsPerMinute);
    LOG_DEBUG("songTimeOffset: %f", songTimeOffset);
    LOG_DEBUG("shuffle: %f", shuffle);
    LOG_DEBUG("shufflePeriod: %f", shufflePeriod);
    LOG_DEBUG("previewStartTime: %f", previewStartTime);
    LOG_DEBUG("previewDuration: %f", previewDuration);

    EnvironmentInfoSO* environmentInfo = LoadEnvironmentInfo(standardLevelInfoSaveData->environmentName, false);
    EnvironmentInfoSO* allDirectionsEnvironmentInfo = LoadEnvironmentInfo(standardLevelInfoSaveData->allDirectionsEnvironmentName, true);
    List<PreviewDifficultyBeatmapSet*>* list = List<PreviewDifficultyBeatmapSet*>::New_ctor();
    Array<StandardLevelInfoSaveData::DifficultyBeatmapSet*>* difficultyBeatmapSets = standardLevelInfoSaveData->difficultyBeatmapSets;
    for (int i = 0; i < difficultyBeatmapSets->Length(); i++) {
        StandardLevelInfoSaveData::DifficultyBeatmapSet* difficultyBeatmapSet = difficultyBeatmapSets->values[i];
        BeatmapCharacteristicSO* beatmapCharacteristicBySerializedName = GetCustomLevelLoader()->beatmapCharacteristicCollection->GetBeatmapCharacteristicBySerializedName(difficultyBeatmapSet->beatmapCharacteristicName);
        LOG_DEBUG("beatmapCharacteristicBySerializedName: %p", beatmapCharacteristicBySerializedName);
        LOG_DEBUG("beatmapCharacteristicBySerializedName: %s", to_utf8(csstrtostr(difficultyBeatmapSet->beatmapCharacteristicName)).c_str());
        if (beatmapCharacteristicBySerializedName) {
            Array<BeatmapDifficulty>* array = Array<BeatmapDifficulty>::NewLength(difficultyBeatmapSet->difficultyBeatmaps->Length());
            for (int j = 0; j < difficultyBeatmapSet->difficultyBeatmaps->Length(); j++) {
                BeatmapDifficulty beatmapDifficulty;
                BeatmapDifficultySerializedMethods::BeatmapDifficultyFromSerializedName(difficultyBeatmapSet->difficultyBeatmaps->values[j]->difficulty, beatmapDifficulty);
                array->values[j] = beatmapDifficulty;
            }
            list->Add_NEW(PreviewDifficultyBeatmapSet::New_ctor(beatmapCharacteristicBySerializedName, array));
        }
    }
    GetAlwaysOwnedContentContainerSO()->alwaysOwnedBeatmapLevelIds->Add_NEW(levelID);
    LOG_DEBUG("LoadCustomPreviewBeatmapLevel Stop");
    return CustomPreviewBeatmapLevel::New_ctor(GetCustomLevelLoader()->defaultPackCover, standardLevelInfoSaveData, il2cpp_utils::createcsstr(customLevelPath), reinterpret_cast<IAudioClipAsyncLoader*>(GetCachedMediaAsyncLoader()), reinterpret_cast<ISpriteAsyncLoader*>(GetCachedMediaAsyncLoader()), levelID, songName, songSubName, songAuthorName, levelAuthorName, beatsPerMinute, songTimeOffset, shuffle, shufflePeriod, previewStartTime, previewDuration, environmentInfo, allDirectionsEnvironmentInfo, list->ToArray());
}

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
                CustomBeatmapLevel* customBeatmapLevel = LoadCustomBeatmapLevelAsync(reinterpret_cast<CustomPreviewBeatmapLevel*>(previewBeatmapLevel), cancellationToken);
                LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync");
                if (!customBeatmapLevel || !customBeatmapLevel->get_beatmapLevelData_NEW()) {
                    LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync Stop");
                    return Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>::New_ctor(BeatmapLevelsModel::GetBeatmapLevelResult(true, nullptr));
                }
                self->loadedBeatmapLevels->PutToCache(levelID, reinterpret_cast<IBeatmapLevel*>(customBeatmapLevel));
                LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync Stop %p", customBeatmapLevel);
                return Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>::New_ctor(BeatmapLevelsModel::GetBeatmapLevelResult(false, reinterpret_cast<IBeatmapLevel*>(customBeatmapLevel)));
            }
        }
    }
    LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync Stop");
    return result;
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

MAKE_HOOK_OFFSETLESS(BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync, Task_1<IBeatmapLevelPackCollection*>*, BeatmapLevelsModel* self, CancellationToken cancellationToken) {
    LOG_DEBUG("BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync");
    ArrayUtil::First(Resources::FindObjectsOfTypeAll<LevelFilteringNavigationController*>())->cancellationTokenSource->Cancel();
    return Task_1<IBeatmapLevelPackCollection*>::New_ctor(self->customLevelPackCollection);
}

MAKE_HOOK_OFFSETLESS(LevelFilteringNavigationController_UpdateCustomSongs, void, LevelFilteringNavigationController* self) {
    LOG_DEBUG("LevelFilteringNavigationController_UpdateCustomSongs");
    self->customLevelPacks = self->beatmapLevelsModel->customLevelPackCollection->get_beatmapLevelPacks_NEW();
    List<IBeatmapLevelPack*>* list = List<IBeatmapLevelPack*>::New_ctor();
    if(self->ostBeatmapLevelPacks)
        list->AddRange(reinterpret_cast<IEnumerable_1<IBeatmapLevelPack*>*>(self->ostBeatmapLevelPacks));
    if(self->musicPacksBeatmapLevelPacks)
        list->AddRange(reinterpret_cast<IEnumerable_1<IBeatmapLevelPack*>*>(self->musicPacksBeatmapLevelPacks));
    if(self->customLevelPacks)
        list->AddRange(reinterpret_cast<IEnumerable_1<IBeatmapLevelPack*>*>(self->customLevelPacks));
    self->allBeatmapLevelPacks = list->ToArray();
    self->levelSearchViewController->Setup(self->allBeatmapLevelPacks);
    self->UpdateSecondChildControllerContent(self->selectLevelCategoryViewController->get_selectedLevelCategory());
}

MAKE_HOOK_OFFSETLESS(SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels, bool, SinglePlayerLevelSelectionFlowCoordinator* self) {
    LOG_DEBUG("SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels");
    SetupCustomCharacteristics();
    return true;
}

MAKE_HOOK_OFFSETLESS(FileHelpers_GetEscapedURLForFilePath, Il2CppString*, Il2CppString* filePath) {
    LOG_DEBUG("FileHelpers_GetEscapedURLForFilePath");
    return il2cpp_utils::createcsstr("file:///" + to_utf8(csstrtostr(filePath)));
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

            static SongLoader* loader = new SongLoader();
            loader->Setup();
            loader->RefreshLevelPacks();
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
    custom_types::Register::RegisterTypes<RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO>();
    INSTALL_HOOK_OFFSETLESS(getLogger(), BeatmapLevelsModel_ClearLoadedBeatmapLevelsCaches, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "ClearLoadedBeatmapLevelsCaches", 0));
    INSTALL_HOOK_OFFSETLESS(getLogger(), BeatmapLevelsModel_GetBeatmapLevelAsync, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "GetBeatmapLevelAsync", 2));
    INSTALL_HOOK_OFFSETLESS(getLogger(), BeatmapLevelsModel_UpdateAllLoadedBeatmapLevelPacks, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "UpdateAllLoadedBeatmapLevelPacks", 0));
    INSTALL_HOOK_OFFSETLESS(getLogger(), BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "ReloadCustomLevelPackCollectionAsync", 1));
    INSTALL_HOOK_OFFSETLESS(getLogger(), LevelFilteringNavigationController_UpdateCustomSongs, il2cpp_utils::FindMethodUnsafe("", "LevelFilteringNavigationController", "UpdateCustomSongs", 0));
    INSTALL_HOOK_OFFSETLESS(getLogger(), SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels, il2cpp_utils::FindMethodUnsafe("", "SinglePlayerLevelSelectionFlowCoordinator", "get_enableCustomLevels", 0)); 
    INSTALL_HOOK_OFFSETLESS(getLogger(), FileHelpers_GetEscapedURLForFilePath, il2cpp_utils::FindMethodUnsafe("", "FileHelpers", "GetEscapedURLForFilePath", 1));
    INSTALL_HOOK_OFFSETLESS(getLogger(), SceneManager_Internal_ActiveSceneChanged, il2cpp_utils::FindMethodUnsafe("UnityEngine.SceneManagement", "SceneManager", "Internal_ActiveSceneChanged", 2));
    CustomCharacteristics::InstallHook();
    LOG_INFO("Successfully installed SongLoader!");
}