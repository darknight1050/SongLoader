#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"

#include "CodegenIncludes.hpp"
#include "ArrayUtil.hpp"
#include "customlogger.hpp"
#include "CustomCharacteristics.hpp"
#include "ExtraData.hpp"
#include "Sprites.hpp"

#include <map>

#include <unistd.h>
#include <chrono>

#define BASEPATH "/sdcard"
#define CUSTOMSONGS_FOLDER "BeatSaberSongs"

static ModInfo modInfo;

Logger& getLogger() {
    static auto logger = new Logger(modInfo, LoggerOptions(false, true));
    return *logger;
}
//Hash,ExtraSongData
std::map<std::string, ExtraSongData> DataCache;

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
inline Sprite* Base64ToSprite(std::string_view base64, int width, int height)
{
    Array<uint8_t>* bytes = System::Convert::FromBase64String(il2cpp_utils::createcsstr(base64));
    Texture2D* texture = Texture2D::New_ctor(width, height, TextureFormat::RGBA32, false, false);
    if (ImageConversion::LoadImage(texture, bytes, false))
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
    // TODO: Hash using libcrypto instead
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

// This really isn't async, it shouldn't be called async
CustomPreviewBeatmapLevel* LoadCustomPreviewBeatmapLevelAsync(Il2CppString* customLevelPath, StandardLevelInfoSaveData* standardLevelInfoSaveData)
{
    if(!standardLevelInfoSaveData) return nullptr;
    Il2CppString* path = Path::Combine(customLevelPath, il2cpp_utils::createcsstr("Info.dat"));
    if (!File::Exists(path))
    {
        path = Path::Combine(customLevelPath, il2cpp_utils::createcsstr("info.dat"));
    }
    Il2CppString* str = File::ReadAllText(path);
    rapidjson::Document d;
    d.Parse(to_utf8(csstrtostr(str)).c_str());
    // Can safely assert this property always exists.
    auto arr = d["_difficultyBeatmapSets"].GetArray();
    for(int i = 0; i < arr.Size(); i++)
    {
        auto itr = arr[i].FindMember("_customData");
        if (itr != arr[i].MemberEnd()) {
            if (!itr->value.IsObject()) {
                LOG_ERROR("_customData on beatmap at path: %s is not an object!", to_utf8(csstrtostr(path)).c_str());
                continue;
            }
            auto reqItr = itr->value.FindMember("_requirements");
            if (reqItr != itr->value.MemberEnd()) {
                if (!reqItr->value.IsArray()) {
                    LOG_ERROR("_requirements on _customData on beatmap at path: %s is not an array!", to_utf8(csstrtostr(path)).c_str());
                    continue;
                }
                auto reqArr = reqItr->value.GetArray();
                for (int i = 0; i < reqArr.Size(); i++) {
                    if (reqArr[i].GetString() == "Noodle Extensions") {
                        // Short circuit if we see Noodle Extensions.
                        // TODO: This should be avoided if possible.
                        return nullptr;
                    }
                }
            }
        }
    }
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
    Array<StandardLevelInfoSaveData::DifficultyBeatmapSet*>* difficultyBeatmapSets = standardLevelInfoSaveData->difficultyBeatmapSets;
    List<PreviewDifficultyBeatmapSet*>* list = List<PreviewDifficultyBeatmapSet*>::New_ctor(difficultyBeatmapSets->Length());
    for (int i = 0; i < difficultyBeatmapSets->Length(); i++)
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

// Same here, this isn't async, don't call it async :)
Array<CustomPreviewBeatmapLevel*>* LoadCustomPreviewBeatmapLevelsAsync(Il2CppString* customLevelPackPath) {
    List<CustomPreviewBeatmapLevel*>* customPreviewBeatmapLevels = List<CustomPreviewBeatmapLevel*>::New_ctor();
    Array<Il2CppString*>* directories = Directory::GetDirectories(customLevelPackPath);
    if (directories->Length() < 1)
        return customPreviewBeatmapLevels->ToArray();
    for (int i = 0; i < directories->Length(); i++)
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

void GetPreviewAudioClipAsync(CustomPreviewBeatmapLevel* customPreviewBeatmapLevel, std::function<void (AudioClip*)> continuation) {
    LOG_DEBUG("GetPreviewAudioClipAsync Start %p", customPreviewBeatmapLevel->previewAudioClip);
    
    Il2CppString* path = Path::Combine(customPreviewBeatmapLevel->customLevelPath, customPreviewBeatmapLevel->standardLevelInfoSaveData->songFilename);
    auto tokenSource = System::Threading::CancellationTokenSource::New_ctor();
    auto* task = customPreviewBeatmapLevel->audioClipAsyncLoader->LoadAudioClipAsync(path, tokenSource->get_Token());

    auto lmb = [](System::Threading::Tasks::Task* task) {
        
    };

    static_cast<System::Threading::Tasks::Task*>(task)->ContinueWith(reinterpret_cast<System::Action_1<System::Threading::Tasks::Task*>*>(
        il2cpp_utils::MakeAction(reinterpret_cast<function_ptr_t<void, System::Threading::Tasks::Task*>>(&lmb))
    ));
    
    if (!customPreviewBeatmapLevel->previewAudioClip && !System::String::IsNullOrEmpty(customPreviewBeatmapLevel->standardLevelInfoSaveData->songFilename))
    {
        Il2CppString* path = Path::Combine(customPreviewBeatmapLevel->customLevelPath, customPreviewBeatmapLevel->standardLevelInfoSaveData->songFilename);
        AudioType audioType = (to_utf8(csstrtostr(Path::GetExtension(path)->ToLower())) == ".wav") ? AudioType::WAV : AudioType::OGGVORBIS;
        UnityWebRequest* www = UnityWebRequestMultimedia::GetAudioClip(FileHelpers::GetEscapedURLForFilePath(path), audioType);
        static_cast<DownloadHandlerAudioClip*>(www->m_DownloadHandler)->set_streamAudio(true);
        UnityWebRequestAsyncOperation* request = www->SendWebRequest();

        // Log our error status, set the previewAudioClip, and call our continuation function
        // This gives us some degree of "async"
        auto lmb = [www, customPreviewBeatmapLevel, continuation](UnityEngine::AsyncOperation* operation) {
            LOG_DEBUG("GetPreviewAudioClipAsync ErrorStatus %d %d", www->get_isHttpError(), www->get_isNetworkError());
            if (!www->get_isHttpError() && !www->get_isNetworkError())
                customPreviewBeatmapLevel->previewAudioClip = DownloadHandlerAudioClip::GetContent(www);
            continuation(customPreviewBeatmapLevel->previewAudioClip);
        };

        // When the request is complete, call our lambda
        request->add_completed(reinterpret_cast<System::Action_1<UnityEngine::AsyncOperation*>*>(il2cpp_utils::MakeAction(
            reinterpret_cast<function_ptr_t<void, UnityEngine::AsyncOperation*>>(&lmb)
        )));
    }
}

void LoadBeatmapLevelDataAsync(Il2CppString* customLevelPath, CustomBeatmapLevel* customBeatmapLevel, StandardLevelInfoSaveData* standardLevelInfoSaveData, CancellationToken cancellationToken, std::function<void (BeatmapLevelData*)> continuation)
{
    LOG_DEBUG("LoadBeatmapLevelDataAsync Start");
    Array<IDifficultyBeatmapSet*>* difficultyBeatmapSets = LoadDifficultyBeatmapSetsAsync(customLevelPath, customBeatmapLevel, standardLevelInfoSaveData);
    GetPreviewAudioClipAsync(reinterpret_cast<CustomPreviewBeatmapLevel*>(customBeatmapLevel), [difficultyBeatmapSets, continuation](AudioClip* clip) {
        LOG_DEBUG("LoadBeatmapLevelDataAsync Stop");
        continuation(clip ? BeatmapLevelData::New_ctor(clip, difficultyBeatmapSets) : nullptr);
    });
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

MAKE_HOOK_OFFSETLESS(LevelFilteringNavigationController_SetupBeatmapLevelPacks, void, LevelFilteringNavigationController* self) {
    LOG_DEBUG("LevelFilteringNavigationController_SetupBeatmapLevelPacks");
    self->enableCustomLevels = true;
    LevelFilteringNavigationController_SetupBeatmapLevelPacks(self);
    
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



void AddSong(std::string levelID, std::string path)
{
    if (!DataCache.contains(levelID))
        DataCache[levelID] = ExtraSongData(levelID, path);
}

ExtraSongData RetrieveExtraSongData(std::string levelID, std::string loadIfNullPath)
{
    if(DataCache.contains(levelID))
        return DataCache.at(levelID);

    AddSong(levelID, loadIfNullPath);

    if(DataCache.contains(levelID))
        return DataCache.at(levelID);

    return ExtraSongData();
}


DifficultyData RetrieveDifficultyData(CustomDifficultyBeatmap* beatmap)
{
    CustomBeatmapLevel* level = reinterpret_cast<CustomBeatmapLevel*>(beatmap->get_level());
    if (!level) {
        getLogger().warning("Cannot retrieve difficulty data for custom song: this beatmap does not contain a valid level!");
        return {};
    }

    ExtraSongData songData = RetrieveExtraSongData(to_utf8(csstrtostr(level->levelID)), to_utf8(csstrtostr(level->customLevelPath)));
    CustomDifficultyBeatmapSet* ParentSet = reinterpret_cast<CustomDifficultyBeatmapSet*>(beatmap->parentDifficultyBeatmapSet);
    if (!ParentSet) {
        getLogger().warning("Cannot retrieve difficulty data for custom song: this beatmap does not have parent difficulty beatmap set!");
        return {};
    }

    std::vector<DifficultyData>::iterator it = std::find_if (songData._difficulties.begin(), songData._difficulties.end(), [ParentSet, beatmap](DifficultyData x) {
        return x._difficulty == beatmap->get_difficulty() && (
            x._beatmapCharacteristicName == to_utf8(csstrtostr(ParentSet->get_beatmapCharacteristic()->get_characteristicNameLocalizationKey()))
            || x._beatmapCharacteristicName == to_utf8(csstrtostr(ParentSet->get_beatmapCharacteristic()->get_serializedName()))
        );
    });

    if (it == songData._difficulties.end()) {
        getLogger().info("RetrieveDifficultyData: difficulty data not found.");
        return {};
    }

    return *it;
}

struct OverrideLabels
{
    std::string EasyOverride = "";
    std::string NormalOverride = "";
    std::string HardOverride = "";
    std::string ExpertOverride = "";
    std::string ExpertPlusOverride = "";
    OverrideLabels()
    {

    };
};

static std::map<std::string, OverrideLabels> levelLabels;

static OverrideLabels currentLabels = OverrideLabels();

static void SetCurrentLabels(OverrideLabels labels)
{
    currentLabels.EasyOverride = labels.EasyOverride;
    currentLabels.NormalOverride = labels.NormalOverride;
    currentLabels.HardOverride = labels.HardOverride;
    currentLabels.ExpertOverride = labels.ExpertOverride;
    currentLabels.ExpertPlusOverride = labels.ExpertPlusOverride;
}

static void clearOverrideLabels()
{
    currentLabels.EasyOverride = "";
    currentLabels.NormalOverride = "";
    currentLabels.HardOverride = "";
    currentLabels.ExpertOverride = "";
    currentLabels.ExpertPlusOverride = "";
}

MAKE_HOOK_OFFSETLESS(StandardLevelDetailView_RefreshContent, void, StandardLevelDetailView* self)
{
    StandardLevelDetailView_RefreshContent(self);

    auto originalDifficultyBeatmap = self->get_selectedDifficultyBeatmap();
    auto originalDifficultyBeatmapClass = il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(originalDifficultyBeatmap));
    if (!il2cpp_functions::class_is_assignable_from(classof(CustomDifficultyBeatmap*), originalDifficultyBeatmapClass)) {
        return;
    }

    getLogger().info("Refreshing custom level content...");
    auto customDifficultyBeatmap = reinterpret_cast<CustomDifficultyBeatmap*>(originalDifficultyBeatmap);
    CustomPreviewBeatmapLevel* level = reinterpret_cast<CustomPreviewBeatmapLevel*>(customDifficultyBeatmap->get_level());

    if(level)
    {
        ExtraSongData songData = RetrieveExtraSongData(to_utf8(csstrtostr(level->levelID)), to_utf8(csstrtostr(level->customLevelPath)));

        CustomDifficultyBeatmap* selectedDiff = reinterpret_cast<CustomDifficultyBeatmap*>(self->get_selectedDifficultyBeatmap());

        DifficultyData diffData = RetrieveDifficultyData(selectedDiff);

        levelLabels.clear();
        std::string currentCharacteristic = "";
        for(DifficultyData diffLevel : songData._difficulties)
        {
            auto difficulty = diffLevel._difficulty;
            std::string characteristic = diffLevel._beatmapCharacteristicName;
            CustomDifficultyBeatmapSet* ParentSet = reinterpret_cast<CustomDifficultyBeatmapSet*>(selectedDiff->parentDifficultyBeatmapSet);
            if(characteristic == to_utf8(csstrtostr(ParentSet->get_beatmapCharacteristic()->serializedName)))
                currentCharacteristic = characteristic;
            if (!levelLabels.contains(characteristic))
                levelLabels.emplace(characteristic, OverrideLabels());
                OverrideLabels charLabels = levelLabels[characteristic];
            switch (difficulty)
            {
                case BeatmapDifficulty::Easy:
                    charLabels.EasyOverride = diffLevel._difficultyLabel;
                    break;
                case BeatmapDifficulty::Normal:
                    charLabels.NormalOverride = diffLevel._difficultyLabel;
                    break;
                case BeatmapDifficulty::Hard:
                    charLabels.HardOverride = diffLevel._difficultyLabel;
                    break;
                case BeatmapDifficulty::Expert:
                    charLabels.ExpertOverride = diffLevel._difficultyLabel;
                    break;
                case BeatmapDifficulty::ExpertPlus:
                    charLabels.ExpertPlusOverride = diffLevel._difficultyLabel;
                    break;
            }
            SetCurrentLabels(levelLabels[currentCharacteristic]);
        }
    }
    
}

MAKE_HOOK_OFFSETLESS(StandardLevelDetailViewController_UpdateActionButtonIntractability, void, StandardLevelDetailViewController* self)
{

}

MAKE_HOOK_OFFSETLESS(BeatmapDifficultyMethods_Name, Il2CppString*, BeatmapDifficulty difficulty)
{
    using namespace il2cpp_utils;
    Il2CppString* Result = BeatmapDifficultyMethods_Name(difficulty);

    if (difficulty == BeatmapDifficulty::Easy)
    {
        if (currentLabels.EasyOverride != "")
            Result = createcsstr(currentLabels.EasyOverride);
    }
    if (difficulty == BeatmapDifficulty::Normal)
    {
        if (currentLabels.NormalOverride != "")
            Result = createcsstr(currentLabels.NormalOverride);
    }
    if (difficulty == BeatmapDifficulty::Hard)
    {
        if (currentLabels.HardOverride != "")
            Result = createcsstr(currentLabels.HardOverride);
    }
    if (difficulty == BeatmapDifficulty::Expert)
    {
        if (currentLabels.ExpertOverride != "")
            Result = createcsstr(currentLabels.ExpertOverride);
    }
    if (difficulty == BeatmapDifficulty::ExpertPlus)
    {
        if (currentLabels.ExpertPlusOverride != "")
            Result = createcsstr(currentLabels.ExpertPlusOverride);
    }
    return Result;
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
    INSTALL_HOOK_OFFSETLESS(LevelFilteringNavigationController_SetupBeatmapLevelPacks, il2cpp_utils::FindMethodUnsafe("", "LevelFilteringNavigationController", "SetupBeatmapLevelPacks", 0));
    INSTALL_HOOK_OFFSETLESS(FileHelpers_GetEscapedURLForFilePath, il2cpp_utils::FindMethodUnsafe("", "FileHelpers", "GetEscapedURLForFilePath", 1));
    INSTALL_HOOK_OFFSETLESS(BeatmapCharacteristicCollectionSO_GetBeatmapCharacteristicBySerializedName, il2cpp_utils::FindMethodUnsafe("", "BeatmapCharacteristicCollectionSO", "GetBeatmapCharacteristicBySerializedName", 1));
    INSTALL_HOOK_OFFSETLESS(StandardLevelDetailView_RefreshContent, il2cpp_utils::FindMethodUnsafe("", "StandardLevelDetailView", "RefreshContent", 0));
    INSTALL_HOOK_OFFSETLESS(StandardLevelDetailViewController_UpdateActionButtonIntractability, il2cpp_utils::FindMethodUnsafe("", "StandardLevelDetailViewController", "UpdateActionButtonIntractability", 0));
    INSTALL_HOOK_OFFSETLESS(BeatmapDifficultyMethods_Name, il2cpp_utils::FindMethodUnsafe("", "BeatmapDifficultyMethods", "Name", 1));
    baseProjectPath = il2cpp_utils::createcsstr(BASEPATH, il2cpp_utils::StringType::Manual);
    LOG_INFO("Successfully installed SongLoader!");
}
