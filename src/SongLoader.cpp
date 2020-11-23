#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"

#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/SongPackMask.hpp"
#include "GlobalNamespace/SelectLevelCategoryViewController.hpp"
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
#include "UnityEngine/Networking/UnityWebRequestAsyncOperation.hpp"
#include "UnityEngine/Networking/UnityWebRequest.hpp"
#include "UnityEngine/Networking/UnityWebRequestMultimedia.hpp"
#include "UnityEngine/Networking/DownloadHandlerAudioClip.hpp"
#include "UnityEngine/AudioType.hpp"
#include "UnityEngine/AudioClip.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Vector4.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/SpriteMeshType.hpp"
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
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"

#include "customlogger.hpp"
#include "CustomCharacteristics.hpp"
#include "Sprites.hpp"

#include <unistd.h>
#include <chrono>

#define BASEPATH "/sdcard"
#define CUSTOMSONGS_FOLDER "BeatSaberSongs"

static ModInfo modInfo;

const Logger& getLogger() {
    static const Logger logger(modInfo, LoggerOptions(false, false));
    return logger;
}

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace UnityEngine::Networking;
using namespace System::Collections::Generic;
using namespace System::Collections;
using namespace System::IO;
using namespace System::Threading;
using namespace Tasks;


Il2CppString* baseProjectPath = nullptr;
CustomLevelLoader* customLevelLoader = nullptr;
CachedMediaAsyncLoader* _cachedMediaAsyncLoader = nullptr;
AlwaysOwnedContentContainerSO* _alwaysOwnedContentContainer = nullptr;

//From questui: https://github.com/darknight1050/questui
inline Sprite* Base64ToSprite(std::string base64, int width, int height)
{
    Array<uint8_t>* bytes = System::Convert::FromBase64String(il2cpp_utils::createcsstr(base64));
    Texture2D* texture = Texture2D::New_ctor(width, height, TextureFormat::RGBA32, false, false);
    if(ImageConversion::LoadImage(texture, bytes, false))
        return Sprite::Create(texture, UnityEngine::Rect(0.0f, 0.0f, (float)width, (float)height), UnityEngine::Vector2(0.5f,0.5f), 1024.0f, 1u, SpriteMeshType::FullRect, UnityEngine::Vector4(0.0f, 0.0f, 0.0f, 0.0f), false);
    return nullptr;
}

void SetupCustomCharacteristics()
{
    static bool created = false;
    if(!created) {
        created = true;
        CustomCharacteristics::RegisterCustomCharacteristic(Base64ToSprite(Sprites::MissingBase64, 128, 128), "Missing Characteristic", "Missing Characteristic", "MissingCharacteristic", "MissingCharacteristic", false, false, 1000);
        CustomCharacteristics::RegisterCustomCharacteristic(Base64ToSprite(Sprites::LightshowBase64, 128, 128), "Lightshow", "Lightshow", "Lightshow", "Lightshow", false, false, 100);
        CustomCharacteristics::RegisterCustomCharacteristic(Base64ToSprite(Sprites::LawlessBase64, 128, 128), "Lawless", "Lawless - Anything Goes", "Lawless", "Lawless", false, false, 101);
    }
}

std::string GetCustomLevelHash(StandardLevelInfoSaveData* level, std::string customLevelPath)
{
    LOG_DEBUG("GetCustomLevelHash Start");
    std::string actualPath = customLevelPath + "/Info.dat";
    if (!fileexists(actualPath)) actualPath = customLevelPath + "/info.dat";
        
    std::string hash = "";

    LOG_DEBUG("GetCustomLevelHash Reading all bytes from %s", actualPath.c_str());

    std::vector<char> bytesAsChar = readbytes(actualPath);
    LOG_DEBUG("GetCustomLevelHash Starting reading beatmaps");
    for (int i = 0; i < level->get_difficultyBeatmapSets()->Length(); i++)
    {
        for (int j = 0; j < level->get_difficultyBeatmapSets()->values[i]->get_difficultyBeatmaps()->Length(); j++)
        {
            std::string diffFile = to_utf8(csstrtostr(level->get_difficultyBeatmapSets()->values[i]->get_difficultyBeatmaps()->values[j]->get_beatmapFilename()));
            if (!fileexists(customLevelPath + "/" + diffFile))
            {
                LOG_ERROR("GetCustomLevelHash File %s did not exist", (customLevelPath + "/" + diffFile).c_str());
                continue;
            } 

            std::vector<char> currentDiff = readbytes(customLevelPath + "/" + diffFile);
            bytesAsChar.insert(bytesAsChar.end(), currentDiff.begin(), currentDiff.end());
        }
    }
    Array<uint8_t>* bytes = Array<uint8_t>::NewLength(bytesAsChar.size());
    for(int i = 0;i<bytes->Length();i++){
        bytes->values[i] = bytesAsChar[i];
    }
    LOG_DEBUG("GetCustomLevelHash computing Hash, found %d bytes", bytes->Length());
    hash = to_utf8(csstrtostr(System::BitConverter::ToString(System::Security::Cryptography::SHA1::Create()->ComputeHash(bytes))));

    hash.erase(std::remove(hash.begin(), hash.end(), '-'), hash.end());
        
    LOG_DEBUG("GetCustomLevelHash Stop %s", hash.c_str());
    return hash;
}

StandardLevelInfoSaveData* LoadCustomLevelInfoSaveData(Il2CppString* customLevelPath)
{
    Il2CppString* path = Path::Combine(customLevelPath, il2cpp_utils::createcsstr("Info.dat"));
    if (!File::Exists(path))
    {
        path = Path::Combine(customLevelPath, il2cpp_utils::createcsstr("info.dat"));
    }
    if (File::Exists(path))
    {
        Il2CppString* str = File::ReadAllText(path);
        return StandardLevelInfoSaveData::DeserializeFromJSONString(str);        
    }
    return nullptr;
}

EnvironmentInfoSO* LoadEnvironmentInfo(Il2CppString* environmentName, bool allDirections)
{
    EnvironmentInfoSO* environmentInfoSO = customLevelLoader->environmentSceneInfoCollection->GetEnviromentInfoBySerializedName(environmentName);
    if (!environmentInfoSO)
        environmentInfoSO = (allDirections ? customLevelLoader->defaultAllDirectionsEnvironmentInfo : customLevelLoader->defaultEnvironmentInfo);
    LOG_DEBUG("LoadEnvironmentInfo: %p", environmentInfoSO);
    return environmentInfoSO;
}

CustomPreviewBeatmapLevel* LoadCustomPreviewBeatmapLevelAsync(Il2CppString* customLevelPath, StandardLevelInfoSaveData* standardLevelInfoSaveData)
{
    if(!standardLevelInfoSaveData) return nullptr;
    LOG_DEBUG("LoadCustomPreviewBeatmapLevelAsync StandardLevelInfoSaveData: ");
    Il2CppString* levelID = il2cpp_utils::createcsstr("custom_level_" + GetCustomLevelHash(standardLevelInfoSaveData, to_utf8(csstrtostr(customLevelPath))))->ToLower();
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
    LOG_DEBUG("levelID: %s", to_utf8(csstrtostr(levelID)).c_str());
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
    for (int i = 0;i<difficultyBeatmapSets->Length();i++)
    {
        StandardLevelInfoSaveData::DifficultyBeatmapSet* difficultyBeatmapSet = difficultyBeatmapSets->values[i];
        BeatmapCharacteristicSO* beatmapCharacteristicBySerializedName = customLevelLoader->beatmapCharacteristicCollection->GetBeatmapCharacteristicBySerializedName(difficultyBeatmapSet->beatmapCharacteristicName);
        LOG_DEBUG("beatmapCharacteristicBySerializedName: %p", beatmapCharacteristicBySerializedName);
        LOG_DEBUG("beatmapCharacteristicBySerializedName: %s", to_utf8(csstrtostr(difficultyBeatmapSet->beatmapCharacteristicName)).c_str());
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
    return CustomPreviewBeatmapLevel::New_ctor(customLevelLoader->defaultPackCover, standardLevelInfoSaveData, customLevelPath, reinterpret_cast<IAudioClipAsyncLoader*>(_cachedMediaAsyncLoader), reinterpret_cast<ISpriteAsyncLoader*>(_cachedMediaAsyncLoader), levelID, songName, songSubName, songAuthorName, levelAuthorName, beatsPerMinute, songTimeOffset, shuffle, shufflePeriod, previewStartTime, previewDuration, environmentInfo, allDirectionsEnvironmentInfo, list->ToArray());
}

Array<CustomPreviewBeatmapLevel*>* LoadCustomPreviewBeatmapLevelsAsync(Il2CppString* customLevelPackPath) {
    List<CustomPreviewBeatmapLevel*>* customPreviewBeatmapLevels = List<CustomPreviewBeatmapLevel*>::New_ctor();
    Array<Il2CppString*>* directories = Directory::GetDirectories(customLevelPackPath);
    if(directories->Length() < 1)
        return customPreviewBeatmapLevels->ToArray();
    for (int i = 0;i<directories->Length(); i++)
    {
        Il2CppString* customLevelPath = directories->values[i];
        CustomPreviewBeatmapLevel* customPreviewBeatmapLevel = LoadCustomPreviewBeatmapLevelAsync(customLevelPath, LoadCustomLevelInfoSaveData(customLevelPath));
        LOG_DEBUG("LoadCustomPreviewBeatmapLevelsAsync customPreviewBeatmapLevel %p", customPreviewBeatmapLevel);
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
    return CustomBeatmapLevelPack::New_ctor(il2cpp_utils::createcsstr("custom_levelpack_" + to_utf8(csstrtostr(customLevelPackPath))), packName, packName, customLevelLoader->defaultPackCover, customBeatmapLevelCollection);
}

struct CustomPackFolderInfo
{
    Il2CppString* folderName;
    Il2CppString* packName;
};

Array<IBeatmapLevelPack*>* LoadCustomPreviewBeatmapLevelPacksAsync(std::vector<CustomPackFolderInfo> customPackFolderInfos) {
    int numberOfPacks = customPackFolderInfos.size();
    List<IBeatmapLevelPack*>* customBeatmapLevelPacks = List<IBeatmapLevelPack*>::New_ctor(numberOfPacks);
    for (int i = 0; i < numberOfPacks; i++)
    {
        Il2CppString* customLevelPackPath = Path::Combine(baseProjectPath, customPackFolderInfos[i].folderName);
        if (Directory::Exists(customLevelPackPath))
        {
            CustomBeatmapLevelPack* customBeatmapLevelPack = LoadCustomBeatmapLevelPackAsync(customLevelPackPath, customPackFolderInfos[i].packName);
            if (customBeatmapLevelPack && customBeatmapLevelPack->get_beatmapLevelCollection()->get_beatmapLevels()->Length() != 0)
            {
                _alwaysOwnedContentContainer->alwaysOwnedPacksIds->Add(customBeatmapLevelPack->packID);
                customBeatmapLevelPacks->Add(reinterpret_cast<IBeatmapLevelPack*>(customBeatmapLevelPack));
            }
        }
    }
    return customBeatmapLevelPacks->ToArray();
}

std::vector<CustomPackFolderInfo> GetSubFoldersInfosAsync(std::string rootPath)
{
    std::vector<CustomPackFolderInfo> subDirFolderInfo;
    Il2CppString* rootPathCS = Path::Combine(baseProjectPath, il2cpp_utils::createcsstr(rootPath));
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
    LOG_DEBUG("LoadBeatmapDataBeatmapData Start");
    BeatmapData* data = nullptr;
    Il2CppString* path = Path::Combine(customLevelPath, difficultyFileName);
    if (File::Exists(path))
    {
        Il2CppString* json = File::ReadAllText(path);
        data = BeatmapDataLoader::New_ctor()->GetBeatmapDataFromJson(json, standardLevelInfoSaveData->beatsPerMinute, standardLevelInfoSaveData->shuffle, standardLevelInfoSaveData->shufflePeriod);
    }
    LOG_DEBUG("LoadBeatmapDataBeatmapData Stop");
    return data;
}

CustomDifficultyBeatmap* LoadDifficultyBeatmapAsync(Il2CppString* customLevelPath, CustomBeatmapLevel* parentCustomBeatmapLevel, CustomDifficultyBeatmapSet* parentDifficultyBeatmapSet, StandardLevelInfoSaveData* standardLevelInfoSaveData, StandardLevelInfoSaveData::DifficultyBeatmap* difficultyBeatmapSaveData)
{
    LOG_DEBUG("LoadDifficultyBeatmapAsync Start");
    BeatmapData* beatmapData = LoadBeatmapDataBeatmapData(customLevelPath, difficultyBeatmapSaveData->beatmapFilename, standardLevelInfoSaveData);
    BeatmapDifficulty difficulty;
    BeatmapDifficultySerializedMethods::BeatmapDifficultyFromSerializedName(difficultyBeatmapSaveData->difficulty, difficulty);
    LOG_DEBUG("LoadDifficultyBeatmapAsync Stop");
    return CustomDifficultyBeatmap::New_ctor(reinterpret_cast<IBeatmapLevel*>(parentCustomBeatmapLevel), reinterpret_cast<IDifficultyBeatmapSet*>(parentDifficultyBeatmapSet), difficulty, difficultyBeatmapSaveData->difficultyRank, difficultyBeatmapSaveData->noteJumpMovementSpeed, difficultyBeatmapSaveData->noteJumpStartBeatOffset, beatmapData);
}

IDifficultyBeatmapSet* LoadDifficultyBeatmapSetAsync(Il2CppString* customLevelPath, CustomBeatmapLevel* customBeatmapLevel, StandardLevelInfoSaveData* standardLevelInfoSaveData, StandardLevelInfoSaveData::DifficultyBeatmapSet* difficultyBeatmapSetSaveData)
{
    LOG_DEBUG("LoadDifficultyBeatmapSetAsync Start");
    BeatmapCharacteristicSO* beatmapCharacteristicBySerializedName = customLevelLoader->beatmapCharacteristicCollection->GetBeatmapCharacteristicBySerializedName(difficultyBeatmapSetSaveData->beatmapCharacteristicName);
    Array<CustomDifficultyBeatmap*>* difficultyBeatmaps = Array<CustomDifficultyBeatmap*>::NewLength(difficultyBeatmapSetSaveData->difficultyBeatmaps->Length());
    CustomDifficultyBeatmapSet* difficultyBeatmapSet = CustomDifficultyBeatmapSet::New_ctor(beatmapCharacteristicBySerializedName);
    for (int i = 0; i < difficultyBeatmapSetSaveData->difficultyBeatmaps->Length(); i++)
    {
        CustomDifficultyBeatmap* customDifficultyBeatmap = LoadDifficultyBeatmapAsync(customLevelPath, customBeatmapLevel, difficultyBeatmapSet, standardLevelInfoSaveData, difficultyBeatmapSetSaveData->difficultyBeatmaps->values[i]);
        difficultyBeatmaps->values[i] = customDifficultyBeatmap;
    }
    difficultyBeatmapSet->SetCustomDifficultyBeatmaps(difficultyBeatmaps);
    LOG_DEBUG("LoadDifficultyBeatmapSetAsync Stop");
    return reinterpret_cast<IDifficultyBeatmapSet*>(difficultyBeatmapSet);
}

Array<IDifficultyBeatmapSet*>* LoadDifficultyBeatmapSetsAsync(Il2CppString* customLevelPath, CustomBeatmapLevel* customBeatmapLevel, StandardLevelInfoSaveData* standardLevelInfoSaveData)
{
    LOG_DEBUG("LoadDifficultyBeatmapSetsAsync Start");
    Array<IDifficultyBeatmapSet*>* difficultyBeatmapSets = Array<IDifficultyBeatmapSet*>::NewLength(standardLevelInfoSaveData->difficultyBeatmapSets->Length());
    for (int i = 0; i < difficultyBeatmapSets->Length(); i++)
    {
        IDifficultyBeatmapSet* difficultyBeatmapSet = LoadDifficultyBeatmapSetAsync(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData, standardLevelInfoSaveData->difficultyBeatmapSets->values[i]);
        difficultyBeatmapSets->values[i] = difficultyBeatmapSet;
    }
    LOG_DEBUG("LoadDifficultyBeatmapSetsAsync Stop");
    return difficultyBeatmapSets;
}

AudioClip* GetPreviewAudioClipAsync(CustomPreviewBeatmapLevel* customPreviewBeatmapLevel) {
    LOG_DEBUG("GetPreviewAudioClipAsync Start %p", customPreviewBeatmapLevel->previewAudioClip);
    if (!customPreviewBeatmapLevel->previewAudioClip && !System::String::IsNullOrEmpty(customPreviewBeatmapLevel->standardLevelInfoSaveData->songFilename))
    {
        Il2CppString* path = Path::Combine(customPreviewBeatmapLevel->customLevelPath, customPreviewBeatmapLevel->standardLevelInfoSaveData->songFilename);
        AudioType audioType = (to_utf8(csstrtostr(Path::GetExtension(path)->ToLower())) == ".wav") ? AudioType::WAV : AudioType::OGGVORBIS;
        UnityWebRequest* www = UnityWebRequestMultimedia::GetAudioClip(FileHelpers::GetEscapedURLForFilePath(path), audioType);
        ((DownloadHandlerAudioClip*)www->m_DownloadHandler)->set_streamAudio(true);
        UnityWebRequestAsyncOperation* request = www->SendWebRequest();
        while (!request->get_isDone())
        {
            LOG_DEBUG("GetPreviewAudioClipAsync Delay");
            usleep(100* 1000);
        }
        LOG_DEBUG("GetPreviewAudioClipAsync ErrorStatus %d %d", www->get_isHttpError(), www->get_isNetworkError());
        if(!www->get_isHttpError() && !www->get_isNetworkError())
            customPreviewBeatmapLevel->previewAudioClip = DownloadHandlerAudioClip::GetContent(www);
    }
    LOG_DEBUG("GetPreviewAudioClipAsync Stop %p", customPreviewBeatmapLevel->previewAudioClip);
    return customPreviewBeatmapLevel->previewAudioClip;
}

BeatmapLevelData* LoadBeatmapLevelDataAsync(Il2CppString* customLevelPath, CustomBeatmapLevel* customBeatmapLevel, StandardLevelInfoSaveData* standardLevelInfoSaveData, CancellationToken cancellationToken)
{
    LOG_DEBUG("LoadBeatmapLevelDataAsync Start");
    Array<IDifficultyBeatmapSet*>* difficultyBeatmapSets = LoadDifficultyBeatmapSetsAsync(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData);
    AudioClip* audioClip = GetPreviewAudioClipAsync(reinterpret_cast<CustomPreviewBeatmapLevel*>(customBeatmapLevel));
    if (!audioClip)
        return nullptr;
    LOG_DEBUG("LoadBeatmapLevelDataAsync Stop");
    return BeatmapLevelData::New_ctor(audioClip, difficultyBeatmapSets);
}

CustomBeatmapLevel* LoadCustomBeatmapLevelAsync(CustomPreviewBeatmapLevel* customPreviewBeatmapLevel, CancellationToken cancellationToken)
{
    LOG_DEBUG("LoadCustomBeatmapLevelAsync Start");
    StandardLevelInfoSaveData* standardLevelInfoSaveData = customPreviewBeatmapLevel->standardLevelInfoSaveData;
    Il2CppString* customLevelPath = customPreviewBeatmapLevel->customLevelPath;
    AudioClip* previewAudioClip = GetPreviewAudioClipAsync(customPreviewBeatmapLevel);
    CustomBeatmapLevel* customBeatmapLevel = CustomBeatmapLevel::New_ctor(customPreviewBeatmapLevel, previewAudioClip);
    BeatmapLevelData* beatmapLevelData = LoadBeatmapLevelDataAsync(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData, cancellationToken);
    customBeatmapLevel->SetBeatmapLevelData(beatmapLevelData);
    LOG_DEBUG("LoadCustomBeatmapLevelAsync Stop");
    return customBeatmapLevel;
}

MAKE_HOOK_OFFSETLESS(BeatmapLevelsModel_ClearLoadedBeatmapLevelsCaches, void, BeatmapLevelsModel* self)
{
    LOG_DEBUG("BeatmapLevelsModel_ClearLoadedBeatmapLevelsCaches");
    if(_cachedMediaAsyncLoader)
        _cachedMediaAsyncLoader->ClearCache();
    BeatmapLevelsModel_ClearLoadedBeatmapLevelsCaches(self);
}

MAKE_HOOK_OFFSETLESS(BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync, Task_1<IBeatmapLevelPackCollection*>*, BeatmapLevelsModel* self, CancellationToken cancellationToken) {
    LOG_DEBUG("BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync Start");
    customLevelLoader = Resources::FindObjectsOfTypeAll<CustomLevelLoader*>()->values[0];
    _cachedMediaAsyncLoader = Resources::FindObjectsOfTypeAll<CachedMediaAsyncLoader*>()->values[0];
    _alwaysOwnedContentContainer = Resources::FindObjectsOfTypeAll<AlwaysOwnedContentContainerSO*>()->values[0];

    std::vector<CustomPackFolderInfo> folders = GetSubFoldersInfosAsync(CUSTOMSONGS_FOLDER);
    folders.insert(folders.begin(), CustomPackFolderInfo{il2cpp_utils::createcsstr(CUSTOMSONGS_FOLDER), il2cpp_utils::createcsstr("Custom Levels")});
    auto start = std::chrono::high_resolution_clock::now(); 
    Array<IBeatmapLevelPack*>* beatmapLevelPacks = LoadCustomPreviewBeatmapLevelPacksAsync(folders);
    std::chrono::microseconds duration = duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start); 
    LOG_INFO("BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync Loading time %d", duration);
    self->customLevelPackCollection = reinterpret_cast<IBeatmapLevelPackCollection*>(BeatmapLevelPackCollection::New_ctor(beatmapLevelPacks));
    self->UpdateLoadedPreviewLevels();
    LOG_DEBUG("BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync Stop");
    return Task_1<IBeatmapLevelPackCollection*>::New_ctor(self->customLevelPackCollection);
}

MAKE_HOOK_OFFSETLESS(BeatmapLevelsModel_GetBeatmapLevelAsync, Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>*, BeatmapLevelsModel* self, Il2CppString* levelID, CancellationToken cancellationToken) {
    LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync Start %s", to_utf8(csstrtostr(levelID)).c_str());
    Task_1<BeatmapLevelsModel::GetBeatmapLevelResult>* result = BeatmapLevelsModel_GetBeatmapLevelAsync(self, levelID, cancellationToken);
    if(result->get_Result().isError) {
        IPreviewBeatmapLevel* previewBeatmapLevel = self->loadedPreviewBeatmapLevels->get_Item(levelID);
        if(previewBeatmapLevel) {
            LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync previewBeatmapLevel %p", previewBeatmapLevel);
            if (il2cpp_functions::class_is_subclass_of(classof(CustomPreviewBeatmapLevel*), il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(previewBeatmapLevel)), true))
            {
                CustomBeatmapLevel* customBeatmapLevel = LoadCustomBeatmapLevelAsync(reinterpret_cast<CustomPreviewBeatmapLevel*>(previewBeatmapLevel), cancellationToken);
                LOG_DEBUG("BeatmapLevelsModel_GetBeatmapLevelAsync");
                if (!customBeatmapLevel || !customBeatmapLevel->get_beatmapLevelData()){
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
    list->AddRange(reinterpret_cast<IEnumerable_1<IBeatmapLevelPack*>*>(self->ostAndExtrasPackCollection->get_beatmapLevelPacks()));
    list->AddRange(reinterpret_cast<IEnumerable_1<IBeatmapLevelPack*>*>(self->dlcLevelPackCollectionContainer->beatmapLevelPackCollection->get_beatmapLevelPacks()));
    if (self->customLevelPackCollection)
    {
        list->AddRange(reinterpret_cast<IEnumerable_1<IBeatmapLevelPack*>*>(self->customLevelPackCollection->get_beatmapLevelPacks()));
    }
    self->allLoadedBeatmapLevelPackCollection = reinterpret_cast<IBeatmapLevelPackCollection*>(BeatmapLevelPackCollection::New_ctor(list->ToArray()));
    LOG_DEBUG("BeatmapLevelsModel_UpdateAllLoadedBeatmapLevelPacks Stop");
}

MAKE_HOOK_OFFSETLESS(LevelFilteringNavigationController_Setup, void, LevelFilteringNavigationController* self, SongPackMask* songPackMask, IBeatmapLevelPack* levelPackToBeSelectedAfterPresent, SelectLevelCategoryViewController::LevelCategory startLevelCategory, bool hidePacksIfOneOrNone, bool enableCustomLevels) {
    LOG_DEBUG("LevelFilteringNavigationController_Setup");
    LevelFilteringNavigationController_Setup(self, songPackMask, levelPackToBeSelectedAfterPresent, startLevelCategory, hidePacksIfOneOrNone, enableCustomLevels);
    self->enableCustomLevels = true;
    SetupCustomCharacteristics();
}

MAKE_HOOK_OFFSETLESS(FileHelpers_GetEscapedURLForFilePath, Il2CppString*, Il2CppString* filePath)
{
    LOG_DEBUG("FileHelpers_GetEscapedURLForFilePath");
    return il2cpp_utils::createcsstr("file:///" + to_utf8(csstrtostr(filePath)));
}


MAKE_HOOK_OFFSETLESS(BeatmapCharacteristicCollectionSO_GetBeatmapCharacteristicBySerializedName, BeatmapCharacteristicSO*, BeatmapCharacteristicCollectionSO* self, Il2CppString* serializedName)
{
    BeatmapCharacteristicSO* result = BeatmapCharacteristicCollectionSO_GetBeatmapCharacteristicBySerializedName(self, serializedName);
    std::string _serializedName = to_utf8(csstrtostr(serializedName));
    if(!result)
    {   
        result = CustomCharacteristics::FindByName(_serializedName);
        if(!result)
            result = CustomCharacteristics::FindByName("MissingCharacteristic");
    }

    return result;
}

extern "C" void setup(ModInfo& info) {
    modInfo.id = "SongLoader";
    modInfo.version = VERSION;
    info = modInfo;
}

extern "C" void load() {
    LOG_INFO("Starting SongLoader installation...");
    il2cpp_functions::Init();
    INSTALL_HOOK_OFFSETLESS(BeatmapLevelsModel_ClearLoadedBeatmapLevelsCaches, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "ClearLoadedBeatmapLevelsCaches", 0));
    INSTALL_HOOK_OFFSETLESS(BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "ReloadCustomLevelPackCollectionAsync", 1));
    INSTALL_HOOK_OFFSETLESS(BeatmapLevelsModel_GetBeatmapLevelAsync, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "GetBeatmapLevelAsync", 2));
    INSTALL_HOOK_OFFSETLESS(BeatmapLevelsModel_UpdateAllLoadedBeatmapLevelPacks, il2cpp_utils::FindMethodUnsafe("", "BeatmapLevelsModel", "UpdateAllLoadedBeatmapLevelPacks", 0));
    INSTALL_HOOK_OFFSETLESS(LevelFilteringNavigationController_Setup, il2cpp_utils::FindMethodUnsafe("", "LevelFilteringNavigationController", "Setup", 5));
    INSTALL_HOOK_OFFSETLESS(FileHelpers_GetEscapedURLForFilePath, il2cpp_utils::FindMethodUnsafe("", "FileHelpers", "GetEscapedURLForFilePath", 1));
    INSTALL_HOOK_OFFSETLESS(BeatmapCharacteristicCollectionSO_GetBeatmapCharacteristicBySerializedName, il2cpp_utils::FindMethodUnsafe("", "BeatmapCharacteristicCollectionSO", "GetBeatmapCharacteristicBySerializedName", 1));
    baseProjectPath = il2cpp_utils::createcsstr(BASEPATH, il2cpp_utils::StringType::Manual);
    LOG_INFO("Successfully installed SongLoader!");
}