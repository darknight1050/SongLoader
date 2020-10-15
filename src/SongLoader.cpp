#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

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
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollectionSO.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmap.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp"
#include "GlobalNamespace/CustomBeatmapLevelCollection.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollection.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollectionSO.hpp"
#include "GlobalNamespace/BeatmapDataLoader.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/EnvironmentsListSO.hpp"
#include "GlobalNamespace/CachedMediaAsyncLoader.hpp"
#include "GlobalNamespace/HMCache_2.hpp"
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
#include "UnityEngine/Texture2D.hpp"
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
#include "System/Collections/Generic/List_1.hpp"
#include "System/Collections/Generic/HashSet_1.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"

#include "customlogger.hpp"
#include "vorbisLib/stb_vorbis.h"

//TODO: REMOVE TABS

static ModInfo modInfo;

const Logger& getLogger() {
    static const Logger logger(modInfo, LoggerOptions(false, false));
    return logger;
}

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace UnityEngine::Networking;
using namespace System::Collections::Generic;
using namespace System::IO;
using namespace System::Threading;
using namespace Tasks;

std::string baseProjectPath = "/sdcard";
Texture2D* _defaultPackCoverTexture2D = nullptr;
BeatmapCharacteristicCollectionSO* _beatmapCharacteristicCollection = nullptr;
EnvironmentsListSO* _environmentSceneInfoCollection = nullptr;
CachedMediaAsyncLoader* _cachedMediaAsyncLoader = nullptr;
AlwaysOwnedContentContainerSO* _alwaysOwnedContentContainer = nullptr;

StandardLevelInfoSaveData* LoadCustomLevelInfoSaveData(Il2CppString* customLevelPath)
{
    Il2CppString* path = Path::Combine(customLevelPath, il2cpp_utils::createcsstr("Info.dat"));
    if (File::Exists(path))
    {
        return StandardLevelInfoSaveData::DeserializeFromJSONString(File::ReadAllText(path));
    }
    return nullptr;
}

EnvironmentInfoSO* LoadEnvironmentInfo(Il2CppString* environmentName, bool allDirections)
{
    EnvironmentInfoSO* environmentInfoSO = _environmentSceneInfoCollection->GetEnviromentInfoBySerializedName(environmentName);
    //if (environmentInfoSO == nullptr)
    //    environmentInfoSO = (allDirections ? _defaultAllDirectionsEnvironmentInfo : _defaultEnvironmentInfo);
    return environmentInfoSO;
}

CustomPreviewBeatmapLevel* LoadCustomPreviewBeatmapLevelAsync(Il2CppString* customLevelPath, StandardLevelInfoSaveData* standardLevelInfoSaveData)
{
    Il2CppString* levelID = il2cpp_utils::createcsstr("custom_level_" + to_utf8(csstrtostr(DirectoryInfo::New_ctor(customLevelPath)->get_Name()->Substring(0, 4))));
    _alwaysOwnedContentContainer->alwaysOwnedBeatmapLevelIds->Add(levelID);
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
    EnvironmentInfoSO* environmentInfo = LoadEnvironmentInfo(standardLevelInfoSaveData->environmentName, false);
    EnvironmentInfoSO* allDirectionsEnvironmentInfo = LoadEnvironmentInfo(standardLevelInfoSaveData->allDirectionsEnvironmentName, true);
    List_1<PreviewDifficultyBeatmapSet*>* list = List<PreviewDifficultyBeatmapSet*>::New_ctor();
    Array<StandardLevelInfoSaveData::DifficultyBeatmapSet*>* difficultyBeatmapSets = standardLevelInfoSaveData->difficultyBeatmapSets;
    for (int i = 0;i<difficultyBeatmapSets->Length();i++)
    {
        StandardLevelInfoSaveData::DifficultyBeatmapSet* difficultyBeatmapSet = difficultyBeatmapSets->values[i];
        BeatmapCharacteristicSO* beatmapCharacteristicBySerializedName = _beatmapCharacteristicCollection->GetBeatmapCharacteristicBySerializedName(difficultyBeatmapSet->beatmapCharacteristicName);
        if (beatmapCharacteristicBySerializedName)
        {
                
            Array<BeatmapDifficulty>* array = Array<BeatmapDifficulty>::NewLength(difficultyBeatmapSet->difficultyBeatmaps->Length());
            for (int j = 0; j < difficultyBeatmapSet->difficultyBeatmaps->Length(); j++)
            {
                BeatmapDifficulty beatmapDifficulty;
                BeatmapDifficultySerializedMethods::BeatmapDifficultyFromSerializedName(difficultyBeatmapSet->difficultyBeatmaps->values[j]->difficulty, beatmapDifficulty);
                array->values[j] = beatmapDifficulty;
            }
            list->Add(PreviewDifficultyBeatmapSet::New_ctor(beatmapCharacteristicBySerializedName, array));
        }
    }
    return CustomPreviewBeatmapLevel::New_ctor(_defaultPackCoverTexture2D, standardLevelInfoSaveData, customLevelPath, _cachedMediaAsyncLoader, _cachedMediaAsyncLoader, levelID, songName, songSubName, songAuthorName, levelAuthorName, beatsPerMinute, songTimeOffset, shuffle, shufflePeriod, previewStartTime, previewDuration, environmentInfo, allDirectionsEnvironmentInfo, list->ToArray());
}

Array<CustomPreviewBeatmapLevel*>* LoadCustomPreviewBeatmapLevelsAsync(Il2CppString* customLevelPackPath) {
    List_1<CustomPreviewBeatmapLevel*>* customPreviewBeatmapLevels = List_1<CustomPreviewBeatmapLevel*>::New_ctor();
    Array<Il2CppString*>* directories = Directory::GetDirectories(customLevelPackPath);
    for (int i = 0;i<directories->Length(); i++)
    {
        Il2CppString* customLevelPath = directories->values[i];
        CustomPreviewBeatmapLevel* customPreviewBeatmapLevel = LoadCustomPreviewBeatmapLevelAsync(customLevelPath, LoadCustomLevelInfoSaveData(customLevelPath));
        if (customPreviewBeatmapLevel && customPreviewBeatmapLevel->get_previewDifficultyBeatmapSets()->Length() != 0)
        {
            customPreviewBeatmapLevels->Add(customPreviewBeatmapLevel);
        }
    }
    return customPreviewBeatmapLevels->ToArray();
}

CustomBeatmapLevelCollection* LoadCustomBeatmapLevelCollectionAsync(Il2CppString* customLevelPackPath) {
    return CustomBeatmapLevelCollection::New_ctor(LoadCustomPreviewBeatmapLevelsAsync(customLevelPackPath));
}

CustomBeatmapLevelPack* LoadCustomBeatmapLevelPackAsync(Il2CppString* customLevelPackPath, Il2CppString* packName) {
    CustomBeatmapLevelCollection* customBeatmapLevelCollection = LoadCustomBeatmapLevelCollectionAsync(customLevelPackPath);
    if (customBeatmapLevelCollection->get_beatmapLevels()->Length() == 0)
        return nullptr;
    Sprite* coverImage = nullptr;//Sprite::Create(_defaultPackCoverTexture2D, UnityEngine::Rect(0.0f, 0.0f, (float)_defaultPackCoverTexture2D->get_width(), (float)_defaultPackCoverTexture2D->get_height()), UnityEngine::Vector2(0.5f, 0.5f), 1024.0f, 1u, SpriteMeshType::FullRect, UnityEngine::Vector4(0.0f, 0.0f, 0.0f, 0.0f), false);
    return CustomBeatmapLevelPack::New_ctor(il2cpp_utils::createcsstr("custom_levelpack_" + to_utf8(csstrtostr(customLevelPackPath))), packName, packName, coverImage, customBeatmapLevelCollection);
}

bool CheckPathExists(Il2CppString* path) {
    return Directory::Exists(path);
}

struct CustomPackFolderInfo
{
    Il2CppString* folderName;
    Il2CppString* packName;
};

Array<CustomBeatmapLevelPack*>* LoadCustomPreviewBeatmapLevelPacksAsync(std::vector<CustomPackFolderInfo> customPackFolderInfos) {
    int numberOfPacks = customPackFolderInfos.size();
    List_1<CustomBeatmapLevelPack*>* customBeatmapLevelPacks = List_1<CustomBeatmapLevelPack*>::New_ctor(numberOfPacks);
    for (int i = 0; i < numberOfPacks; i++)
    {
        Il2CppString* customLevelPackPath = Path::Combine(il2cpp_utils::createcsstr(baseProjectPath), customPackFolderInfos[i].folderName);
        if (CheckPathExists(customLevelPackPath))
        {
            CustomBeatmapLevelPack* customBeatmapLevelPack = LoadCustomBeatmapLevelPackAsync(customLevelPackPath, customPackFolderInfos[i].packName);
            if (customBeatmapLevelPack && customBeatmapLevelPack->get_beatmapLevelCollection()->get_beatmapLevels()->Length() != 0)
            {
                customBeatmapLevelPacks->Add(customBeatmapLevelPack);
            }
        }
    }
    return customBeatmapLevelPacks->ToArray();
}

std::vector<CustomPackFolderInfo> GetSubFoldersInfosAsync(std::string rootPath)
{
    std::vector<CustomPackFolderInfo> subDirFolderInfo;
    Il2CppString* rootPathCS = Path::Combine(il2cpp_utils::createcsstr(baseProjectPath), il2cpp_utils::createcsstr(rootPath));
    Array<Il2CppString*>* directories = Directory::GetDirectories(rootPathCS);
    for (int i = 0;i<directories->Length(); i++)
    {
        Il2CppString* path = directories->values[i];
        subDirFolderInfo.push_back(CustomPackFolderInfo{Path::GetFullPath(path), DirectoryInfo::New_ctor(path)->get_Name()});
    }
    return subDirFolderInfo;
}

BeatmapData* LoadBeatmapDataBeatmapData(Il2CppString* customLevelPath, Il2CppString* difficultyFileName, StandardLevelInfoSaveData* standardLevelInfoSaveData)
{
    getLogger().info("LoadBeatmapDataBeatmapData Start");
    BeatmapData* data = nullptr;
    Il2CppString* path = Path::Combine(customLevelPath, difficultyFileName);
    if (File::Exists(path))
    {
        Il2CppString* json = File::ReadAllText(path);
        data = BeatmapDataLoader::New_ctor()->GetBeatmapDataFromJson(json, standardLevelInfoSaveData->beatsPerMinute, standardLevelInfoSaveData->shuffle, standardLevelInfoSaveData->shufflePeriod);
    }
    getLogger().info("LoadBeatmapDataBeatmapData Stop");
    return data;
}

CustomDifficultyBeatmap* LoadDifficultyBeatmapAsync(Il2CppString* customLevelPath, CustomBeatmapLevel* parentCustomBeatmapLevel, CustomDifficultyBeatmapSet* parentDifficultyBeatmapSet, StandardLevelInfoSaveData* standardLevelInfoSaveData, StandardLevelInfoSaveData::DifficultyBeatmap* difficultyBeatmapSaveData)
{
    getLogger().info("LoadDifficultyBeatmapAsync Start");
    BeatmapData* beatmapData = LoadBeatmapDataBeatmapData(customLevelPath, difficultyBeatmapSaveData->beatmapFilename, standardLevelInfoSaveData);
    BeatmapDifficulty difficulty;
    BeatmapDifficultySerializedMethods::BeatmapDifficultyFromSerializedName(difficultyBeatmapSaveData->difficulty, difficulty);
    getLogger().info("LoadDifficultyBeatmapAsync Stop");
    return CustomDifficultyBeatmap::New_ctor(parentCustomBeatmapLevel, parentDifficultyBeatmapSet, difficulty, difficultyBeatmapSaveData->difficultyRank, difficultyBeatmapSaveData->noteJumpMovementSpeed, difficultyBeatmapSaveData->noteJumpStartBeatOffset, beatmapData);
}

IDifficultyBeatmapSet* LoadDifficultyBeatmapSetAsync(Il2CppString* customLevelPath, CustomBeatmapLevel* customBeatmapLevel, StandardLevelInfoSaveData* standardLevelInfoSaveData, StandardLevelInfoSaveData::DifficultyBeatmapSet* difficultyBeatmapSetSaveData)
{
    getLogger().info("LoadDifficultyBeatmapSetAsync Start");
    BeatmapCharacteristicSO* beatmapCharacteristicBySerializedName = _beatmapCharacteristicCollection->GetBeatmapCharacteristicBySerializedName(difficultyBeatmapSetSaveData->beatmapCharacteristicName);
    
    Array<CustomDifficultyBeatmap*>* difficultyBeatmaps = Array<CustomDifficultyBeatmap*>::NewLength(difficultyBeatmapSetSaveData->difficultyBeatmaps->Length());
    CustomDifficultyBeatmapSet* difficultyBeatmapSet = CustomDifficultyBeatmapSet::New_ctor(beatmapCharacteristicBySerializedName);
    for (int i = 0; i < difficultyBeatmapSetSaveData->difficultyBeatmaps->Length(); i++)
    {
        CustomDifficultyBeatmap* customDifficultyBeatmap = LoadDifficultyBeatmapAsync(customLevelPath, customBeatmapLevel, difficultyBeatmapSet, standardLevelInfoSaveData, difficultyBeatmapSetSaveData->difficultyBeatmaps->values[i]);
        difficultyBeatmaps->values[i] = customDifficultyBeatmap;
    }
    difficultyBeatmapSet->SetCustomDifficultyBeatmaps(difficultyBeatmaps);
    getLogger().info("LoadDifficultyBeatmapSetAsync Stop");
    return difficultyBeatmapSet;
}

Array<IDifficultyBeatmapSet*>* LoadDifficultyBeatmapSetsAsync(Il2CppString* customLevelPath, CustomBeatmapLevel* customBeatmapLevel, StandardLevelInfoSaveData* standardLevelInfoSaveData)
{
    getLogger().info("LoadDifficultyBeatmapSetsAsync Start");
    Array<IDifficultyBeatmapSet*>* difficultyBeatmapSets = Array<IDifficultyBeatmapSet*>::NewLength(standardLevelInfoSaveData->difficultyBeatmapSets->Length());
    for (int i = 0; i < difficultyBeatmapSets->Length(); i++)
    {
        IDifficultyBeatmapSet* difficultyBeatmapSet = LoadDifficultyBeatmapSetAsync(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData, standardLevelInfoSaveData->difficultyBeatmapSets->values[i]);
        difficultyBeatmapSets->values[i] = difficultyBeatmapSet;
    }
    getLogger().info("LoadDifficultyBeatmapSetsAsync Stop");
    return difficultyBeatmapSets;
}

/*AudioClip* GetPreviewAudioClipAsync(CustomPreviewBeatmapLevel* customPreviewBeatmapLevel) {
    getLogger().info("GetPreviewAudioClipAsync Start %p", customPreviewBeatmapLevel->previewAudioClip);
    if (!customPreviewBeatmapLevel->previewAudioClip && !System::String::IsNullOrEmpty(customPreviewBeatmapLevel->standardLevelInfoSaveData->songFilename))
    {
        Il2CppString* path = Path::Combine(customPreviewBeatmapLevel->customLevelPath, customPreviewBeatmapLevel->standardLevelInfoSaveData->songFilename);
        std::string filePath = to_utf8(csstrtostr(path));
        getLogger().info("Sound file: %s", filePath.c_str());
        short* data;
        int sampleRate;
        int channels;
        int dataLength = stb_vorbis_decode_filename(filePath.c_str(), &channels, &sampleRate, &data);
        if(dataLength > 0)
        {
            getLogger().info("Sound file decoded into data");
            int trueLength = dataLength * channels;

            Array<float>* samples = Array<float>::NewLength(trueLength);
            
            for (int i = 0; i < trueLength; i++)
            {
                short sample = data[i];
                samples->values[i] = (float) ((double) sample) / SHRT_MAX;
            }

            free(data);
            getLogger().info("channels: %d", channels);
            getLogger().info("sampleRate: %d", sampleRate);
            getLogger().info("amount of samples: %d, true data length: %d", dataLength, trueLength);

            getLogger().info("Sound file decoded into C# float array");
            customPreviewBeatmapLevel->previewAudioClip = UnityEngine::AudioClip::Create(customPreviewBeatmapLevel->customLevelPath, dataLength, channels, sampleRate, false);
            getLogger().info("clip created");
            customPreviewBeatmapLevel->previewAudioClip->SetData(samples, 0);
            getLogger().info("Data set on clip");
        }
    }
    getLogger().info("GetPreviewAudioClipAsync Stop %p", customPreviewBeatmapLevel->previewAudioClip);
    return customPreviewBeatmapLevel->previewAudioClip;
}*/

AudioClip* GetPreviewAudioClipAsync(CustomPreviewBeatmapLevel* customPreviewBeatmapLevel) {
    getLogger().info("GetPreviewAudioClipAsync Start %p", customPreviewBeatmapLevel->previewAudioClip);
    if (!customPreviewBeatmapLevel->previewAudioClip && !System::String::IsNullOrEmpty(customPreviewBeatmapLevel->standardLevelInfoSaveData->songFilename))
    {
        Il2CppString* path = Path::Combine(customPreviewBeatmapLevel->customLevelPath, customPreviewBeatmapLevel->standardLevelInfoSaveData->songFilename);
        AudioType audioType = (to_utf8(csstrtostr(((System::String*)Path::GetExtension(path))->ToLower())) == ".wav") ? AudioType::WAV : AudioType::OGGVORBIS;
        UnityWebRequest* www = UnityWebRequestMultimedia::GetAudioClip(FileHelpers::GetEscapedURLForFilePath(path), audioType);
        ((DownloadHandlerAudioClip*)www->m_DownloadHandler)->set_streamAudio(true);
        UnityWebRequestAsyncOperation* request = www->SendWebRequest();
        while (!request->get_isDone())
        {
            getLogger().info("GetPreviewAudioClipAsync Delay");
            Task::Delay(100);
        }
        getLogger().info("GetPreviewAudioClipAsync ErrorStatus %d %d", www->get_isHttpError(), www->get_isNetworkError());
        if(!www->get_isHttpError() && !www->get_isNetworkError())
            customPreviewBeatmapLevel->previewAudioClip = DownloadHandlerAudioClip::GetContent(www);
    }
    getLogger().info("GetPreviewAudioClipAsync Stop %p", customPreviewBeatmapLevel->previewAudioClip);
    return customPreviewBeatmapLevel->previewAudioClip;
}

BeatmapLevelData* LoadBeatmapLevelDataAsync(Il2CppString* customLevelPath, CustomBeatmapLevel* customBeatmapLevel, StandardLevelInfoSaveData* standardLevelInfoSaveData, CancellationToken cancellationToken)
{
    getLogger().info("LoadBeatmapLevelDataAsync Start");
    Array<IDifficultyBeatmapSet*>* difficultyBeatmapSets = LoadDifficultyBeatmapSetsAsync(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData);
    AudioClip* audioClip = GetPreviewAudioClipAsync(((CustomPreviewBeatmapLevel*)customBeatmapLevel));
    if (!audioClip)
        return nullptr;
    getLogger().info("LoadBeatmapLevelDataAsync Stop");
    return BeatmapLevelData::New_ctor(audioClip, difficultyBeatmapSets);
}

CustomBeatmapLevel* LoadCustomBeatmapLevelAsync(CustomPreviewBeatmapLevel* customPreviewBeatmapLevel, CancellationToken cancellationToken)
{
    getLogger().info("LoadCustomBeatmapLevelAsync Start");
    StandardLevelInfoSaveData* standardLevelInfoSaveData = customPreviewBeatmapLevel->standardLevelInfoSaveData;
    Il2CppString* customLevelPath = customPreviewBeatmapLevel->customLevelPath;
    //Texture2D* coverImageTexture2D = customPreviewBeatmapLevel->GetCoverImageTexture2DAsync(cancellationToken)->get_Result();
    Texture2D* coverImageTexture2D = customPreviewBeatmapLevel->coverImageTexture2D;
    //AudioClip* previewAudioClip = GetPreviewAudioClipAsync(customPreviewBeatmapLevel);
    AudioClip* previewAudioClip = customPreviewBeatmapLevel->previewAudioClip;
    CustomBeatmapLevel* customBeatmapLevel = CustomBeatmapLevel::New_ctor(customPreviewBeatmapLevel, previewAudioClip, coverImageTexture2D);
    BeatmapLevelData* beatmapLevelData = LoadBeatmapLevelDataAsync(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData, cancellationToken);
    customBeatmapLevel->SetBeatmapLevelData(beatmapLevelData);
    getLogger().info("LoadCustomBeatmapLevelAsync Stop");
    return customBeatmapLevel;
}

MAKE_HOOK_OFFSETLESS(BeatmapLevelsModel_ClearLoadedBeatmapLevelsCaches, void, BeatmapLevelsModel* self)
{
    getLogger().info("BeatmapLevelsModel_ClearLoadedBeatmapLevelsCaches");
    if(_cachedMediaAsyncLoader)
        _cachedMediaAsyncLoader->ClearCache();
    BeatmapLevelsModel_ClearLoadedBeatmapLevelsCaches(self);
}

MAKE_HOOK_OFFSETLESS(BeatmapLevelsModel_GetCustomLevelPackCollectionAsync, Task_1<IBeatmapLevelPackCollection*>*, BeatmapLevelsModel* self, CancellationToken cancellationToken) {
    getLogger().info("BeatmapLevelsModel_GetCustomLevelPackCollectionAsync Start");
    //Array<IBeatmapLevelPack*>* beatmapLevelPacks = Resources::FindObjectsOfTypeAll<BeatmapLevelPackCollectionSO*>()->values[0]->get_beatmapLevelPacks();
    _defaultPackCoverTexture2D = Texture2D::get_whiteTexture();
    _beatmapCharacteristicCollection = Resources::FindObjectsOfTypeAll<BeatmapCharacteristicCollectionSO*>()->values[0];
    _environmentSceneInfoCollection = Resources::FindObjectsOfTypeAll<EnvironmentsListSO*>()->values[0];
    _cachedMediaAsyncLoader = Resources::FindObjectsOfTypeAll<CachedMediaAsyncLoader*>()->values[0];
    _alwaysOwnedContentContainer = Resources::FindObjectsOfTypeAll<AlwaysOwnedContentContainerSO*>()->values[0];
    
    std::vector<CustomPackFolderInfo> folders = GetSubFoldersInfosAsync("BeatSaberSongs");
    folders.push_back(CustomPackFolderInfo{il2cpp_utils::createcsstr("BeatSaberSongs"), il2cpp_utils::createcsstr("Custom Levels")});
    Array<CustomBeatmapLevelPack*>* beatmapLevelPacks = LoadCustomPreviewBeatmapLevelPacksAsync(folders);
    self->customLevelPackCollection = BeatmapLevelPackCollection::New_ctor((Array<IBeatmapLevelPack*>*)beatmapLevelPacks);
    self->UpdateLoadedPreviewLevels();
    getLogger().info("BeatmapLevelsModel_GetCustomLevelPackCollectionAsync Stop");
    return Task_1<IBeatmapLevelPackCollection*>::New_ctor(self->customLevelPackCollection);
}


IPreviewBeatmapLevel* FindPreviewBeatmapLevel(GlobalNamespace::IBeatmapLevelPackCollection* collection, Il2CppString* levelID){
    Array<IBeatmapLevelPack*>* beatmapLevelPacks = collection->get_beatmapLevelPacks();
    for (int i = 0; i < beatmapLevelPacks->Length(); i++)
    {
        IBeatmapLevelPack* beatmapLevelPack = beatmapLevelPacks->values[i];
        getLogger().info("IBeatmapLevelPack %s", to_utf8(csstrtostr(beatmapLevelPack->get_packName())).c_str());
        Array<IPreviewBeatmapLevel*>* beatmapLevels = beatmapLevelPack->get_beatmapLevelCollection()->get_beatmapLevels();
        for (int j = 0; j < beatmapLevels->Length(); j++)
        {
            IPreviewBeatmapLevel* previewBeatmapLevel = beatmapLevels->values[j];
            getLogger().info("IPreviewBeatmapLevel %s", to_utf8(csstrtostr(previewBeatmapLevel->get_levelID())).c_str());
            if(previewBeatmapLevel->get_levelID() == levelID) {
                getLogger().info("Found levelID %s", to_utf8(csstrtostr(levelID)).c_str());
                return previewBeatmapLevel;
            }
        }
    }
    return nullptr;
}

MAKE_HOOK_OFFSETLESS(BeatmapLevelsModel_GetBeatmapLevelAsync, Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>*, BeatmapLevelsModel* self, Il2CppString* levelID, CancellationToken cancellationToken) {
    getLogger().info("BeatmapLevelsModel_GetBeatmapLevelAsync Start %s", to_utf8(csstrtostr(levelID)).c_str());
    Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>* result = BeatmapLevelsModel_GetBeatmapLevelAsync(self, levelID, cancellationToken);
    if(result->get_Result().isError) {
        IPreviewBeatmapLevel* previewBeatmapLevel = FindPreviewBeatmapLevel(self->customLevelPackCollection, levelID);
        if(previewBeatmapLevel) {
            if (il2cpp_functions::class_is_subclass_of(classof(CustomPreviewBeatmapLevel*), il2cpp_functions::object_get_class((Il2CppObject*)previewBeatmapLevel), true))
            {
                CustomBeatmapLevel* customBeatmapLevel = LoadCustomBeatmapLevelAsync((CustomPreviewBeatmapLevel*)previewBeatmapLevel, cancellationToken);
                if (!customBeatmapLevel || !customBeatmapLevel->get_beatmapLevelData())
                    return Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>::New_ctor(BeatmapLevelsModel::GetBeatmapLevelResult(true, nullptr));
                getLogger().info("BeatmapLevelsModel_GetBeatmapLevelAsync get_levelID %s", to_utf8(csstrtostr(((CustomPreviewBeatmapLevel*)customBeatmapLevel)->get_levelID())).c_str());
                self->loadedBeatmapLevels->PutToCache(levelID, (IBeatmapLevel*)customBeatmapLevel);
                return Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>::New_ctor(BeatmapLevelsModel::GetBeatmapLevelResult(false, (IBeatmapLevel*)customBeatmapLevel));
            }
        }
    }
    getLogger().info("BeatmapLevelsModel_GetBeatmapLevelAsync Stop");
    return result;
}

MAKE_HOOK_OFFSETLESS(LevelFilteringNavigationController_Setup, void, Il2CppObject* self, bool hideIfOneOrNoPacks, bool enableCustomLevels, Il2CppObject* selectedAnnotatedBeatmapLevelCollection) {
    getLogger().info("LevelFilteringNavigationController_Setup");
    LevelFilteringNavigationController_Setup(self, hideIfOneOrNoPacks, true, selectedAnnotatedBeatmapLevelCollection);
}

MAKE_HOOK_OFFSETLESS(FileHelpers_GetEscapedURLForFilePath, Il2CppString*, Il2CppString* filePath)
{
    getLogger().info("FileHelpers_GetEscapedURLForFilePath");
    return il2cpp_utils::createcsstr("file:///" + to_utf8(csstrtostr(filePath)));
}
MAKE_HOOK_OFFSETLESS(StandardLevelDetailViewController_LoadBeatmapLevelAsync, void, StandardLevelDetailViewController* self)
{
    getLogger().info("StandardLevelDetailViewController_LoadBeatmapLevelAsync Start");
    //StandardLevelDetailViewController_LoadBeatmapLevelAsync(self);
    CancellationTokenSource* loadingLevelCancellationTokenSource = self->loadingLevelCancellationTokenSource;
    if (loadingLevelCancellationTokenSource)
    {
        loadingLevelCancellationTokenSource->Cancel();
    }
    self->loadingLevelCancellationTokenSource = new CancellationTokenSource();
    CancellationToken cancellationToken = self->loadingLevelCancellationTokenSource->get_Token();
    getLogger().info("get_levelID 1");
    Il2CppString* levelID = self->previewBeatmapLevel->get_levelID();
    BeatmapLevelsModel::GetBeatmapLevelResult getBeatmapLevelResult = self->beatmapLevelsModel->GetBeatmapLevelAsync(levelID, cancellationToken)->get_Result();
    getLogger().info("getBeatmapLevelResult %d %p", getBeatmapLevelResult.isError, getBeatmapLevelResult.beatmapLevel);
    if (getBeatmapLevelResult.isError || !getBeatmapLevelResult.beatmapLevel)
    {
        self->ShowContent(StandardLevelDetailViewController::ContentType::Error, il2cpp_utils::createcsstr("ERROR"), 0.0f, il2cpp_utils::createcsstr(""));
    }
    else
    {
        if (((CustomPreviewBeatmapLevel*)getBeatmapLevelResult.beatmapLevel)->get_levelID() == self->previewBeatmapLevel->get_levelID())
        {
            self->beatmapLevel = getBeatmapLevelResult.beatmapLevel;
        }
        self->standardLevelDetailView->SetContent(self->beatmapLevel, self->playerDataModel->playerData->lastSelectedBeatmapDifficulty, self->playerDataModel->playerData->lastSelectedBeatmapCharacteristic, self->playerDataModel->playerData, self->showPlayerStats);
        self->ShowContent(StandardLevelDetailViewController::ContentType::OwnedAndReady, il2cpp_utils::createcsstr(""), 0.0f, il2cpp_utils::createcsstr(""));
    }
    getLogger().info("StandardLevelDetailViewController_LoadBeatmapLevelAsync Stop");
}

extern "C" void setup(ModInfo& info) {
    modInfo.id = "SongLoader";
    modInfo.version = "0.1.0";
    info = modInfo;
}

extern "C" void load() {
    getLogger().info("Starting SongLoader installation...");
    INSTALL_HOOK_OFFSETLESS(BeatmapLevelsModel_ClearLoadedBeatmapLevelsCaches, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "ClearLoadedBeatmapLevelsCaches", 0));
    INSTALL_HOOK_OFFSETLESS(BeatmapLevelsModel_GetCustomLevelPackCollectionAsync, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "GetCustomLevelPackCollectionAsync", 1));
    INSTALL_HOOK_OFFSETLESS(BeatmapLevelsModel_GetBeatmapLevelAsync, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "GetBeatmapLevelAsync", 2));
    INSTALL_HOOK_OFFSETLESS(LevelFilteringNavigationController_Setup, il2cpp_utils::FindMethodUnsafe("", "LevelFilteringNavigationController", "Setup", 3));
    INSTALL_HOOK_OFFSETLESS(FileHelpers_GetEscapedURLForFilePath, il2cpp_utils::FindMethodUnsafe("", "FileHelpers", "GetEscapedURLForFilePath", 1));
    INSTALL_HOOK_OFFSETLESS(StandardLevelDetailViewController_LoadBeatmapLevelAsync, il2cpp_utils::FindMethodUnsafe("", "StandardLevelDetailViewController", "LoadBeatmapLevelAsync", 0));
    getLogger().info("Successfully installed SongLoader!");
}