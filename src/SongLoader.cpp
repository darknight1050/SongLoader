#include "CustomTypes/SongLoader.hpp"
#include "CustomLogger.hpp"
#include "Paths.hpp"
#include "Utils/ArrayUtil.hpp"
#include "Utils/HashUtils.hpp"
#include "Utils/FileUtils.hpp"
#include "Utils/FindComponentsUtils.hpp"

#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/CustomLevelLoader.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmap.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmapSet.hpp"
#include "GlobalNamespace/PreviewDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/BeatmapDifficultySerializedMethods.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollectionSO.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmap.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp"
#include "GlobalNamespace/CustomBeatmapLevelCollection.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollection.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollectionSO.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/EnvironmentsListSO.hpp"
#include "GlobalNamespace/CachedMediaAsyncLoader.hpp"
#include "GlobalNamespace/IAudioClipAsyncLoader.hpp"
#include "GlobalNamespace/ISpriteAsyncLoader.hpp"
#include "UnityEngine/AudioClip.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Sprite.hpp"

using namespace RuntimeSongLoader;
using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace FindComponentsUtils;

DEFINE_CLASS(SongLoader);

SongLoader* SongLoader::Instance = nullptr;

SongLoader* SongLoader::GetInstance() {
    if(!Instance) {
        static auto name = il2cpp_utils::createcsstr("SongLoader", il2cpp_utils::StringType::Manual);
        auto gameObject = GameObject::New_ctor(name);
        Instance = gameObject->AddComponent<SongLoader*>();
        GameObject::DontDestroyOnLoad(gameObject);
    }
    return Instance;
}

void SongLoader::RefreshLevelPacks() {
    NeedRefresh = true;
}

void SongLoader::Update() {
    if(!NeedRefresh)
        return;
    NeedRefresh = false;
    auto beatmapLevelsModel = GetBeatmapLevelsModel();
    beatmapLevelsModel->customLevelPackCollection = reinterpret_cast<IBeatmapLevelPackCollection*>(CustomBeatmapLevelPackCollectionSO);
    beatmapLevelsModel->UpdateLoadedPreviewLevels();
    auto levelFilteringNavigationController = ArrayUtil::First(Resources::FindObjectsOfTypeAll<LevelFilteringNavigationController*>());
    if(levelFilteringNavigationController && levelFilteringNavigationController->get_isActiveAndEnabled())
        levelFilteringNavigationController->UpdateCustomSongs();
}

void SongLoader::Awake() {
    if(CustomLevelsCollection)
        return;
    CustomLevelsCollection = CustomBeatmapLevelCollection::New_ctor(Array<CustomPreviewBeatmapLevel*>::NewLength(0));
    static auto customLevelsPackID = il2cpp_utils::createcsstr(CustomLevelPackPrefixID + "CustomLevels", il2cpp_utils::StringType::Manual);
    static auto customLevelsPackName = il2cpp_utils::createcsstr("Custom Levels", il2cpp_utils::StringType::Manual);
    CustomLevelsPack = CustomBeatmapLevelPack::New_ctor(customLevelsPackID, customLevelsPackName, customLevelsPackName, GetCustomLevelLoader()->defaultPackCover, CustomLevelsCollection);
    
    WIPLevelsCollection = CustomBeatmapLevelCollection::New_ctor(Array<CustomPreviewBeatmapLevel*>::NewLength(0));
    static auto WIPLevelsPackID = il2cpp_utils::createcsstr(CustomLevelPackPrefixID + "CustomWIPLevels", il2cpp_utils::StringType::Manual);
    static auto WIPLevelsPackName = il2cpp_utils::createcsstr("WIP Levels", il2cpp_utils::StringType::Manual);
    WIPLevelsPack = CustomBeatmapLevelPack::New_ctor(WIPLevelsPackID, WIPLevelsPackName, WIPLevelsPackName, GetCustomLevelLoader()->defaultPackCover, WIPLevelsCollection);
    
    CustomBeatmapLevelPackCollectionSO = RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO::CreateNew();
    CustomBeatmapLevelPackCollectionSO->AddLevelPack(CustomLevelsPack);
    CustomBeatmapLevelPackCollectionSO->AddLevelPack(WIPLevelsPack);
    
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
    auto customlevelLoader = GetCustomLevelLoader();
    EnvironmentInfoSO* environmentInfoSO = customlevelLoader->environmentSceneInfoCollection->GetEnviromentInfoBySerializedName(environmentName);
    if (!environmentInfoSO)
        environmentInfoSO = (allDirections ? customlevelLoader->defaultAllDirectionsEnvironmentInfo : customlevelLoader->defaultEnvironmentInfo);
    LOG_DEBUG("LoadEnvironmentInfo: %p", environmentInfoSO);
    return environmentInfoSO;
}

CustomPreviewBeatmapLevel* SongLoader::LoadCustomPreviewBeatmapLevel(std::string customLevelPath, StandardLevelInfoSaveData* standardLevelInfoSaveData, std::string& outHash) {
    if(!standardLevelInfoSaveData) 
        return nullptr;
    LOG_DEBUG("LoadCustomPreviewBeatmapLevel StandardLevelInfoSaveData: ");
    bool wip = customLevelPath.find("CustomWIPLevels") != std::string::npos;

    outHash = HashUtils::GetCustomLevelHash(standardLevelInfoSaveData, customLevelPath);
    std::string stringLevelID = CustomLevelPrefixID + outHash;
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
    LOG_DEBUG("LoadCustomPreviewBeatmapLevel Stop");
    return CustomPreviewBeatmapLevel::New_ctor(GetCustomLevelLoader()->defaultPackCover, standardLevelInfoSaveData, il2cpp_utils::createcsstr(customLevelPath), reinterpret_cast<IAudioClipAsyncLoader*>(GetCachedMediaAsyncLoader()), reinterpret_cast<ISpriteAsyncLoader*>(GetCachedMediaAsyncLoader()), levelID, songName, songSubName, songAuthorName, levelAuthorName, beatsPerMinute, songTimeOffset, shuffle, shufflePeriod, previewStartTime, previewDuration, environmentInfo, allDirectionsEnvironmentInfo, list->ToArray());
}
