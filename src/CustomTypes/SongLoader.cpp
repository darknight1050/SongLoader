#include "CustomTypes/SongLoader.hpp"

#include "CustomLogger.hpp"

#include "Paths.hpp"

#include "questui/shared/ArrayUtil.hpp"
#include "Utils/HashUtils.hpp"
#include "Utils/FileUtils.hpp"
#include "Utils/CacheUtils.hpp"
#include "Utils/OggVorbisUtils.hpp"
#include "Utils/FindComponentsUtils.hpp"

#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/CustomLevelLoader.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmap.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmapSet.hpp"
#include "GlobalNamespace/PreviewDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/BeatmapSaveData.hpp"
#include "GlobalNamespace/BeatmapSaveData_NoteData.hpp"
#include "GlobalNamespace/BeatmapSaveData_EventData.hpp"
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
#include "GlobalNamespace/HMTask.hpp"
#include "UnityEngine/AudioClip.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Sprite.hpp"
#include "System/Action.hpp"
#include "System/IO/Path.hpp"
#include "System/IO/Directory.hpp"
#include "System/Threading/Thread.hpp"

#include <vector>

using namespace RuntimeSongLoader;
using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace System::IO;
using namespace System::Threading;
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
    auto levelFilteringNavigationController = QuestUI::ArrayUtil::First(Resources::FindObjectsOfTypeAll<LevelFilteringNavigationController*>());
    if(levelFilteringNavigationController && levelFilteringNavigationController->get_isActiveAndEnabled())
        levelFilteringNavigationController->UpdateCustomSongs();
}

void SongLoader::ctor() {
    beatmapDataLoader = BeatmapDataLoader::New_ctor();

    CustomLevelsCollection = CustomBeatmapLevelCollection::New_ctor(Array<CustomPreviewBeatmapLevel*>::NewLength(0));
    static auto customLevelsPackID = il2cpp_utils::createcsstr(CustomLevelPackPrefixID + CustomLevelsFolder, il2cpp_utils::StringType::Manual);
    static auto customLevelsPackName = il2cpp_utils::createcsstr("Custom Levels", il2cpp_utils::StringType::Manual);
    CustomLevelsPack = CustomBeatmapLevelPack::New_ctor(customLevelsPackID, customLevelsPackName, customLevelsPackName, GetCustomLevelLoader()->defaultPackCover, CustomLevelsCollection);
    
    CustomWIPLevelsCollection = CustomBeatmapLevelCollection::New_ctor(Array<CustomPreviewBeatmapLevel*>::NewLength(0));
    static auto customWIPLevelsPackID = il2cpp_utils::createcsstr(CustomLevelPackPrefixID + CustomWIPLevelsFolder, il2cpp_utils::StringType::Manual);
    static auto customWIPLevelsPackName = il2cpp_utils::createcsstr("WIP Levels", il2cpp_utils::StringType::Manual);
    CustomWIPLevelsPack = CustomBeatmapLevelPack::New_ctor(customWIPLevelsPackID, customWIPLevelsPackName, customWIPLevelsPackName, GetCustomLevelLoader()->defaultPackCover, CustomWIPLevelsCollection);
    
    CustomBeatmapLevelPackCollectionSO = RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO::CreateNew();
    CustomBeatmapLevelPackCollectionSO->AddLevelPack(CustomLevelsPack);
    CustomBeatmapLevelPackCollectionSO->AddLevelPack(CustomWIPLevelsPack);
    
}

StandardLevelInfoSaveData* SongLoader::GetStandardLevelInfoSaveData(const std::string& customLevelPath) {
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
    if(!environmentInfoSO)
        environmentInfoSO = (allDirections ? customlevelLoader->defaultAllDirectionsEnvironmentInfo : customlevelLoader->defaultEnvironmentInfo);
    LOG_DEBUG("LoadEnvironmentInfo: %p", environmentInfoSO);
    return environmentInfoSO;
}

CustomPreviewBeatmapLevel* SongLoader::LoadCustomPreviewBeatmapLevel(const std::string& customLevelPath, StandardLevelInfoSaveData* standardLevelInfoSaveData, std::string& outHash) {
    if(!standardLevelInfoSaveData) 
        return nullptr;
    LOG_DEBUG("LoadCustomPreviewBeatmapLevel StandardLevelInfoSaveData: ");
    bool wip = customLevelPath.find("CustomWIPLevels") != std::string::npos;
    auto hashOpt = HashUtils::GetCustomLevelHash(standardLevelInfoSaveData, customLevelPath);
    if(!hashOpt.has_value())
        return nullptr;
    outHash = *hashOpt;
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
    for(int i = 0; i < difficultyBeatmapSets->Length(); i++) {
        StandardLevelInfoSaveData::DifficultyBeatmapSet* difficultyBeatmapSet = difficultyBeatmapSets->values[i];
        BeatmapCharacteristicSO* beatmapCharacteristicBySerializedName = GetCustomLevelLoader()->beatmapCharacteristicCollection->GetBeatmapCharacteristicBySerializedName(difficultyBeatmapSet->beatmapCharacteristicName);
        LOG_DEBUG("beatmapCharacteristicBySerializedName: %p", beatmapCharacteristicBySerializedName);
        LOG_DEBUG("beatmapCharacteristicBySerializedName: %s", to_utf8(csstrtostr(difficultyBeatmapSet->beatmapCharacteristicName)).c_str());
        if(beatmapCharacteristicBySerializedName) {
            Array<BeatmapDifficulty>* array = Array<BeatmapDifficulty>::NewLength(difficultyBeatmapSet->difficultyBeatmaps->Length());
            for(int j = 0; j < difficultyBeatmapSet->difficultyBeatmaps->Length(); j++) {
                BeatmapDifficulty beatmapDifficulty;
                BeatmapDifficultySerializedMethods::BeatmapDifficultyFromSerializedName(difficultyBeatmapSet->difficultyBeatmaps->values[j]->difficulty, beatmapDifficulty);
                array->values[j] = beatmapDifficulty;
            }
            list->Add(PreviewDifficultyBeatmapSet::New_ctor(beatmapCharacteristicBySerializedName, array));
        }
    }
    LOG_DEBUG("LoadCustomPreviewBeatmapLevel Stop");
    auto result = CustomPreviewBeatmapLevel::New_ctor(GetCustomLevelLoader()->defaultPackCover, standardLevelInfoSaveData, il2cpp_utils::createcsstr(customLevelPath), reinterpret_cast<IAudioClipAsyncLoader*>(GetCachedMediaAsyncLoader()), reinterpret_cast<ISpriteAsyncLoader*>(GetCachedMediaAsyncLoader()), levelID, songName, songSubName, songAuthorName, levelAuthorName, beatsPerMinute, songTimeOffset, shuffle, shufflePeriod, previewStartTime, previewDuration, environmentInfo, allDirectionsEnvironmentInfo, list->ToArray());
    UpdateSongDuration(result, customLevelPath);
    return result;
}

void SongLoader::UpdateSongDuration(CustomPreviewBeatmapLevel* level, const std::string& customLevelPath) {
    float length = 0.0f;
    auto cacheDataOpt = CacheUtils::GetCacheData(customLevelPath);
    if(!cacheDataOpt.has_value())
        return;
    auto cacheData = *cacheDataOpt;
    auto cacheSongDuration = cacheData.songDuration;

    if(cacheSongDuration.has_value())
        length = *cacheSongDuration;
    if(length <= 0.0f)
        length = OggVorbisUtils::GetLengthFromOggVorbisFile(customLevelPath + "/" + to_utf8(csstrtostr(level->standardLevelInfoSaveData->songFilename)));
    if(length <= 0.0f)
        length = GetLengthFromMap(level, customLevelPath);
    level->songDuration = length;
    cacheData.songDuration = length;
    CacheUtils::UpdateCacheData(customLevelPath, cacheData);
}

float SongLoader::GetLengthFromMap(CustomPreviewBeatmapLevel* level, const std::string& customLevelPath) {
    std::string diffFile = to_utf8(csstrtostr(QuestUI::ArrayUtil::Last(QuestUI::ArrayUtil::First(level->standardLevelInfoSaveData->difficultyBeatmapSets)->difficultyBeatmaps)->beatmapFilename));
    std::string path = customLevelPath + "/" + diffFile;
    if(!fileexists(path)) {
        LOG_ERROR("GetLengthFromMap File %s did not exist", (path).c_str());
        return 0.0f;
    }
    auto beatmapSaveData = BeatmapSaveData::DeserializeFromJSONString(il2cpp_utils::createcsstr(FileUtils::ReadAllText(path)));
    float highestTime = 0.0f;
    if(beatmapSaveData->notes->get_Count() > 0) {
        highestTime = QuestUI::ArrayUtil::Max<float>(beatmapSaveData->notes->ToArray(), [](BeatmapSaveData::NoteData* x){ return x->time; });
    } else {
        highestTime = QuestUI::ArrayUtil::Max<float>(beatmapSaveData->events->ToArray(), [](BeatmapSaveData::EventData* x){ return x->time; });
    }
    return beatmapDataLoader->GetRealTimeFromBPMTime(highestTime, level->beatsPerMinute, level->shuffle, level->shufflePeriod);
}

List<GlobalNamespace::CustomPreviewBeatmapLevel*>* SongLoader::LoadSongsFromPath(std::string_view path, std::vector<std::string>& loadedPaths) {
    std::mutex values_mutex;
    const int MAX_THREADS = 8;
    auto customLevelsList = List<CustomPreviewBeatmapLevel*>::New_ctor();
    std::vector<CustomPreviewBeatmapLevel*> customLevels;
    std::vector<std::string> customLevelsFolders = FileUtils::GetFolders(path);
    int songsCount = customLevelsFolders.size();
    int songsPerThread = (songsCount / MAX_THREADS) + 1;
    int threadsCount = 0;
    int threadsFinished = 0;
    for(int threadIndex = 0; threadIndex < MAX_THREADS; threadIndex++) {
        int startIndex = threadIndex*songsPerThread;
        int endIndex = std::min<int>((threadIndex+1)*songsPerThread, songsCount);
        if(startIndex >= endIndex)
            break;
        threadsCount++;
        HMTask::New_ctor(il2cpp_utils::MakeDelegate<System::Action*>(classof(System::Action*),
            (std::function<void()>)[this, startIndex, endIndex, customLevelsList, &customLevelsFolders, &customLevels, &threadsFinished, &loadedPaths, &values_mutex] { 
                for(int i = startIndex; i < endIndex; i++) {
                    auto startLevel = std::chrono::high_resolution_clock::now(); 
                    std::string songPath = customLevelsFolders[i];
                    StandardLevelInfoSaveData* saveData = GetStandardLevelInfoSaveData(songPath);
                    std::string hash;
                    auto level = LoadCustomPreviewBeatmapLevel(songPath, saveData, hash);
                    std::chrono::milliseconds durationLevel = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startLevel);
                    if(level) {  
                        std::lock_guard<std::mutex> lock(values_mutex);
                        customLevelsList->Add(level); //To avoid GC freeing it
                        customLevels.push_back(level);
                        loadedPaths.push_back(songPath);
                        LOG_INFO("Loaded %s in %dms!", songPath.c_str(), durationLevel);
                    } else {
                        LOG_ERROR("Failed %s!", songPath.c_str());
                    }
                }
                threadsFinished++;
            }
        ), nullptr)->Run();
    }
    while(threadsFinished < threadsCount) {
        Thread::Yield();
    }
    std::sort(customLevels.begin(), customLevels.end(), [](CustomPreviewBeatmapLevel* first, CustomPreviewBeatmapLevel* second) { return to_utf8(csstrtostr(first->songName)) < to_utf8(csstrtostr(second->songName)); } );
    auto customLevelsListFinal = List<CustomPreviewBeatmapLevel*>::New_ctor();
    for(CustomPreviewBeatmapLevel* level : customLevels) {
        customLevelsListFinal->Add(level);
    }
    return customLevelsListFinal;
}

void SongLoader::RefreshSongs(bool fullRefresh) {
    HMTask::New_ctor(il2cpp_utils::MakeDelegate<System::Action*>(classof(System::Action*),
        (std::function<void()>)[=] { 
            //Thread::Sleep(2000);
            CacheUtils::LoadFromFile();

            std::vector<std::string> loadedPaths;

            auto start = std::chrono::high_resolution_clock::now();

            CustomLevelsCollection->customPreviewBeatmapLevels = LoadSongsFromPath(BaseLevelsPath + CustomLevelsFolder, loadedPaths)->ToArray();
            CustomWIPLevelsCollection->customPreviewBeatmapLevels = LoadSongsFromPath(BaseLevelsPath + CustomWIPLevelsFolder, loadedPaths)->ToArray();
            
            RefreshLevelPacks();
            auto duration = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start); 
            LOG_INFO("Loaded %d songs in %dms!", CustomLevelsCollection->customPreviewBeatmapLevels->Length() + CustomWIPLevelsCollection->customPreviewBeatmapLevels->Length(), duration);
            
            CacheUtils::SaveToFile(loadedPaths);
        }
    ), nullptr)->Run();
}
