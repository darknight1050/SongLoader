#include "CustomTypes/SongLoader.hpp"

#include "CustomLogger.hpp"

#include "Paths.hpp"
#include "Sprites.hpp"

#include "LoadingUI.hpp"

#include "API.hpp"

#include "Utils/HashUtils.hpp"
#include "Utils/FileUtils.hpp"
#include "Utils/CacheUtils.hpp"
#include "Utils/OggVorbisUtils.hpp"
#include "Utils/FindComponentsUtils.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "questui/shared/CustomTypes/Components/WeakPtrGO.hpp"

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
#include "GlobalNamespace/ISpriteAsyncLoader.hpp"
#include "GlobalNamespace/HMTask.hpp"
#include "UnityEngine/AudioClip.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"
#include "UnityEngine/SceneManagement/Scene.hpp"
#include "System/Array.hpp"
#include "System/Action.hpp"
#include "System/IO/Path.hpp"
#include "System/IO/Directory.hpp"
#include "System/Threading/Thread.hpp"

#include <vector>

#define MAX_THREADS 8

using namespace RuntimeSongLoader;
using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace System::IO;
using namespace System::Threading;
using namespace System::Collections::Generic;
using namespace FindComponentsUtils;

#define FixEmptyString(name) \
if(!name) { \
    LOG_WARN("Fixed nullptr string \"%s\"! THIS SHOULDN'T HAPPEN!", #name);\
    name = System::String::_get_Empty(); \
}

DEFINE_TYPE(RuntimeSongLoader, SongLoader);

SongLoader* SongLoader::Instance = nullptr;

SongLoader* SongLoader::GetInstance() {
    if(!Instance) {
        static auto name = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>("SongLoader");
        auto gameObject = GameObject::New_ctor(name);
        GameObject::DontDestroyOnLoad(gameObject);
        Instance = gameObject->AddComponent<SongLoader*>();
    }
    return Instance;
}

std::vector<std::function<void(std::vector<CustomPreviewBeatmapLevel*> const&)>> SongLoader::LoadedEvents;
std::mutex SongLoader::LoadedEventsMutex;

std::vector<std::function<void(SongLoaderBeatmapLevelPackCollectionSO*)>> SongLoader::RefreshLevelPacksEvents;
std::mutex SongLoader::RefreshLevelPacksEventsMutex;

std::vector<CustomPreviewBeatmapLevel*> SongLoader::GetLoadedLevels() {
    return LoadedLevels;
}

void SongLoader::ctor() {
    INVOKE_CTOR();
    IsLoading = false;
    HasLoaded = false;
    LoadingCancelled = false;
    MaxFolders = 0;
    CurrentFolder = 0;

    beatmapDataLoader = BeatmapDataLoader::New_ctor();

    CustomLevels = Dictionary_2<StringW, CustomPreviewBeatmapLevel*>::New_ctor();
    CustomWIPLevels = Dictionary_2<StringW, CustomPreviewBeatmapLevel*>::New_ctor();

    CustomLevelsPack = SongLoaderCustomBeatmapLevelPack::Make_New(CustomLevelsFolder, "Custom Levels");
    CustomWIPLevelsPack = SongLoaderCustomBeatmapLevelPack::Make_New(CustomWIPLevelsFolder, "WIP Levels", QuestUI::BeatSaberUI::Base64ToSprite(Sprites::CustomWIPLevelsCover));
    CustomBeatmapLevelPackCollectionSO = RuntimeSongLoader::SongLoaderBeatmapLevelPackCollectionSO::CreateNew();
}

void SongLoader::Awake() {
    if(IsLoading)
        LoadingCancelled = true;
}

void SongLoader::Update() {
    if(IsLoading)
        LoadingUI::UpdateLoadingProgress(MaxFolders, CurrentFolder);
    LoadingUI::UpdateState();
}

CustomJSONData::CustomLevelInfoSaveData* SongLoader::GetStandardLevelInfoSaveData(std::string const& customLevelPath) {
    std::string path = customLevelPath + "/info.dat";
    if(!fileexists(path))
        path = customLevelPath + "/Info.dat";
    if(fileexists(path)) {
        //Temporary fix because exceptions don't work
        auto optional = il2cpp_utils::RunMethod<StandardLevelInfoSaveData*>("", "StandardLevelInfoSaveData", "DeserializeFromJSONString", il2cpp_utils::newcsstr(FileUtils::ReadAllText16(path)));
        if(!optional.has_value()) {
            LOG_ERROR("GetStandardLevelInfoSaveData File %s is corrupted!", (path).c_str());
            return nullptr;
        }
        if (!il2cpp_utils::try_cast<CustomJSONData::CustomLevelInfoSaveData>(*optional)) {
            LOG_ERROR("GetStandardLevelInfoSaveData File %s is not parsed as custom level info save data!", (path).c_str());
            return nullptr;
        }

        return static_cast<CustomJSONData::CustomLevelInfoSaveData *>(*optional);

        //return StandardLevelInfoSaveData::DeserializeFromJSONString(il2cpp_utils::newcsstr(FileUtils::ReadAllText16(path)));
    }
    return nullptr;
}

EnvironmentInfoSO* SongLoader::LoadEnvironmentInfo(StringW environmentName, bool allDirections) {
    auto customlevelLoader = GetCustomLevelLoader();
    EnvironmentInfoSO* environmentInfoSO = customlevelLoader->environmentSceneInfoCollection->GetEnvironmentInfoBySerializedName(environmentName);
    if(!environmentInfoSO)
        environmentInfoSO = (allDirections ? customlevelLoader->defaultAllDirectionsEnvironmentInfo : customlevelLoader->defaultEnvironmentInfo);
    LOG_DEBUG("LoadEnvironmentInfo: %p", environmentInfoSO);
    return environmentInfoSO;
}

CustomPreviewBeatmapLevel* SongLoader::LoadCustomPreviewBeatmapLevel(std::string const& customLevelPath, bool wip, CustomJSONData::CustomLevelInfoSaveData* standardLevelInfoSaveData, std::string& outHash) {
    if(!standardLevelInfoSaveData) 
        return nullptr;
    LOG_DEBUG("LoadCustomPreviewBeatmapLevel StandardLevelInfoSaveData: ");
    auto hashOpt = HashUtils::GetCustomLevelHash(standardLevelInfoSaveData, customLevelPath);
    if(!hashOpt.has_value())
        return nullptr;
    outHash = *hashOpt;
    std::string stringLevelID = CustomLevelPrefixID + outHash;
    if(wip)
        stringLevelID += " WIP";
    StringW levelID = il2cpp_utils::newcsstr(stringLevelID);
    StringW songName = standardLevelInfoSaveData->songName;
    FixEmptyString(songName)
    StringW songSubName = standardLevelInfoSaveData->songSubName;
    FixEmptyString(songSubName)
    StringW songAuthorName = standardLevelInfoSaveData->songAuthorName;
    FixEmptyString(songAuthorName)
    StringW levelAuthorName = standardLevelInfoSaveData->levelAuthorName;
    FixEmptyString(levelAuthorName)
    float beatsPerMinute = standardLevelInfoSaveData->beatsPerMinute;
    float songTimeOffset = standardLevelInfoSaveData->songTimeOffset;
    float shuffle = standardLevelInfoSaveData->shuffle;
    float shufflePeriod = standardLevelInfoSaveData->shufflePeriod;
    float previewStartTime = standardLevelInfoSaveData->previewStartTime;
    float previewDuration = standardLevelInfoSaveData->previewDuration;
    LOG_DEBUG("levelID: %s", stringLevelID.c_str());
    LOG_DEBUG("songName: %s", songName.operator std::u16string_view().c_str());
    LOG_DEBUG("songSubName: %s", songSubName.operator std::u16string_view().c_str());
    LOG_DEBUG("songAuthorName: %s", songAuthorName.operator std::u16string_view().c_str());
    LOG_DEBUG("levelAuthorName: %s", levelAuthorName.operator std::u16string_view().c_str());
    LOG_DEBUG("beatsPerMinute: %f", beatsPerMinute);
    LOG_DEBUG("songTimeOffset: %f", songTimeOffset);
    LOG_DEBUG("shuffle: %f", shuffle);
    LOG_DEBUG("shufflePeriod: %f", shufflePeriod);
    LOG_DEBUG("previewStartTime: %f", previewStartTime);
    LOG_DEBUG("previewDuration: %f", previewDuration);

    EnvironmentInfoSO* environmentInfo = LoadEnvironmentInfo(standardLevelInfoSaveData->environmentName, false);
    EnvironmentInfoSO* allDirectionsEnvironmentInfo = LoadEnvironmentInfo(standardLevelInfoSaveData->allDirectionsEnvironmentName, true);
    List<PreviewDifficultyBeatmapSet*>* list = List<PreviewDifficultyBeatmapSet*>::New_ctor();
    for(StandardLevelInfoSaveData::DifficultyBeatmapSet* difficultyBeatmapSet : standardLevelInfoSaveData->difficultyBeatmapSets) {
        if (!difficultyBeatmapSet)
            continue;

        BeatmapCharacteristicSO* beatmapCharacteristicBySerializedName = GetCustomLevelLoader()->beatmapCharacteristicCollection->GetBeatmapCharacteristicBySerializedName(difficultyBeatmapSet->beatmapCharacteristicName);
        LOG_DEBUG("beatmapCharacteristicBySerializedName: %s", difficultyBeatmapSet->beatmapCharacteristicName->chars);
        if(beatmapCharacteristicBySerializedName) {
            ArrayW<BeatmapDifficulty> array = ArrayW<BeatmapDifficulty>(difficultyBeatmapSet->difficultyBeatmaps.Length());
            for(int j = 0; j < array.Length(); j++) {
                BeatmapDifficulty beatmapDifficulty;
                BeatmapDifficultySerializedMethods::BeatmapDifficultyFromSerializedName(difficultyBeatmapSet->difficultyBeatmaps[j]->difficulty, beatmapDifficulty);
                array[j] = beatmapDifficulty;
            }
            list->Add(PreviewDifficultyBeatmapSet::New_ctor(beatmapCharacteristicBySerializedName, array));
        }
    }
    LOG_DEBUG("LoadCustomPreviewBeatmapLevel Stop");
    
    auto result = CustomPreviewBeatmapLevel::New_ctor(GetCustomLevelLoader()->defaultPackCover, standardLevelInfoSaveData, il2cpp_utils::newcsstr(to_utf16(customLevelPath)), reinterpret_cast<ISpriteAsyncLoader*>(GetCachedMediaAsyncLoader()), levelID, songName, songSubName, songAuthorName, levelAuthorName, beatsPerMinute, songTimeOffset, shuffle, shufflePeriod, previewStartTime, previewDuration, environmentInfo, allDirectionsEnvironmentInfo, list->ToArray());
    UpdateSongDuration(result, customLevelPath);
    return result;
}

void SongLoader::UpdateSongDuration(CustomPreviewBeatmapLevel* level, std::string const& customLevelPath) {
    float length = 0.0f;
    auto cacheDataOpt = CacheUtils::GetCacheData(customLevelPath);
    if(!cacheDataOpt.has_value())
        return;
    auto cacheData = *cacheDataOpt;
    auto cacheSongDuration = cacheData.songDuration;

    if(cacheSongDuration.has_value())
        length = *cacheSongDuration;
    if(length <= 0.0f)
        length = OggVorbisUtils::GetLengthFromOggVorbisFile(customLevelPath + "/" + level->standardLevelInfoSaveData->songFilename.operator std::string());
    if(length <= 0.0f)
        length = GetLengthFromMap(level, customLevelPath);
    level->songDuration = length;
    cacheData.songDuration = length;
    CacheUtils::UpdateCacheData(customLevelPath, cacheData);
}

#define FindMethodGetter(methodName) \
    ::il2cpp_utils::il2cpp_type_check::MetadataGetter<methodName>::get();

float SongLoader::GetLengthFromMap(CustomPreviewBeatmapLevel* level, std::string const& customLevelPath) {
    std::string diffFile = QuestUI::ArrayUtil::Last(QuestUI::ArrayUtil::First(level->standardLevelInfoSaveData->difficultyBeatmapSets)->difficultyBeatmaps)->beatmapFilename;
    std::string path = customLevelPath + "/" + diffFile;
    if(!fileexists(path)) {
        LOG_ERROR("GetLengthFromMap File %s doesn't exist!", (path).c_str());
        return 0.0f;
    }

    static auto DeserializeFromJSONStringMethodInfo = FindMethodGetter(&BeatmapSaveData::DeserializeFromJSONString);

    //Temporary fix because exceptions don't work
    auto optional = il2cpp_utils::RunMethod<BeatmapSaveData *>(nullptr, DeserializeFromJSONStringMethodInfo, il2cpp_utils::newcsstr(FileUtils::ReadAllText16(path)));
    if(!optional.has_value() || !optional.value()) {
        LOG_ERROR("GetLengthFromMap File %s is corrupted!", (path).c_str());
        return 0.0f;
    }
    auto beatmapSaveData = *optional;

    //auto beatmapSaveData = BeatmapSaveData::DeserializeFromJSONString(il2cpp_utils::newcsstr(FileUtils::ReadAllText16(path)));
    float highestTime = 0.0f;
    if(beatmapSaveData->notes->get_Count() > 0) {
        highestTime = QuestUI::ArrayUtil::Max<float>(beatmapSaveData->notes->ToArray(), [](BeatmapSaveData::NoteData* x){ return x->time; });
    } else {
        highestTime = QuestUI::ArrayUtil::Max<float>(beatmapSaveData->events->ToArray(), [](BeatmapSaveData::EventData* x){ return x->time; });
    }
    return beatmapDataLoader->GetRealTimeFromBPMTime(highestTime, level->beatsPerMinute, level->shuffle, level->shufflePeriod);
}

ArrayW<CustomPreviewBeatmapLevel*> GetDictionaryValues(Dictionary_2<StringW, CustomPreviewBeatmapLevel*>* dictionary) {
    if(!dictionary)
        return ArrayW<CustomPreviewBeatmapLevel*>();
    auto array = ArrayW<CustomPreviewBeatmapLevel*>(dictionary->get_Count());
    //dictionary->get_Values()->CopyTo(array, 0);
    il2cpp_utils::RunMethodUnsafe(dictionary->get_Values(), "CopyTo", reinterpret_cast<System::Array*>(array.convert()), 0);
    return array;
}

void SongLoader::RefreshLevelPacks(bool includeDefault) const {
    CustomBeatmapLevelPackCollectionSO->ClearLevelPacks();

    if(includeDefault) {
        CustomLevelsPack->SortLevels();
        CustomLevelsPack->AddTo(CustomBeatmapLevelPackCollectionSO);
        CustomWIPLevelsPack->SortLevels();
        CustomWIPLevelsPack->AddTo(CustomBeatmapLevelPackCollectionSO);
    }

    std::lock_guard<std::mutex> lock(RefreshLevelPacksEventsMutex);
    for (auto& event : RefreshLevelPacksEvents) {
        event(CustomBeatmapLevelPackCollectionSO);
    }

    auto beatmapLevelsModel = GetBeatmapLevelsModel();
    beatmapLevelsModel->customLevelPackCollection = reinterpret_cast<IBeatmapLevelPackCollection*>(CustomBeatmapLevelPackCollectionSO);
    beatmapLevelsModel->UpdateLoadedPreviewLevels();
    static QuestUI::WeakPtrGO<LevelFilteringNavigationController> levelFilteringNavigationController;
    if (!levelFilteringNavigationController)
        levelFilteringNavigationController = QuestUI::ArrayUtil::First(Resources::FindObjectsOfTypeAll<LevelFilteringNavigationController*>());

    if(levelFilteringNavigationController && levelFilteringNavigationController->get_isActiveAndEnabled())
        levelFilteringNavigationController->UpdateCustomSongs();
}

void SongLoader::RefreshSongs(bool fullRefresh, std::function<void(std::vector<CustomPreviewBeatmapLevel*> const&)> const& songsLoaded) {
    if(IsLoading)
        return;
    SceneManagement::Scene activeScene = SceneManagement::SceneManager::GetActiveScene();
    
    if(!activeScene.IsValid() || activeScene.get_name().operator std::u16string_view().find(u"Menu") == std::string::npos)
        return;

    IsLoading = true;
    HasLoaded = false;
    CurrentFolder = 0;

    HMTask::New_ctor(il2cpp_utils::MakeDelegate<System::Action*>(classof(System::Action*),
        (std::function<void()>)[=] {

            auto start = std::chrono::high_resolution_clock::now();

            if(fullRefresh) {
                CustomLevels->Clear();
                CustomWIPLevels->Clear();
            }

            std::mutex valuesMutex;
            std::vector<std::string> loadedPaths;

            std::vector<std::string> customLevelsFolders = FileUtils::GetFolders(API::GetCustomLevelsPath());
            std::vector<std::string> customWIPLevelsFolders = FileUtils::GetFolders(API::GetCustomWIPLevelsPath());
            customLevelsFolders.insert(std::end(customLevelsFolders), std::begin(customWIPLevelsFolders), std::end(customWIPLevelsFolders));

            MaxFolders = customLevelsFolders.size();

            int songsPerThread = (MaxFolders / MAX_THREADS) + 1;
            int threadsCount = 0;
            int threadsFinished = 0;
            for(int threadIndex = 0; threadIndex < MAX_THREADS; threadIndex++) {
                int startIndex = threadIndex*songsPerThread;
                int endIndex = std::min<int>((threadIndex+1)*songsPerThread, MaxFolders);
                if(startIndex >= endIndex)
                    break;
                threadsCount++;
                HMTask::New_ctor(il2cpp_utils::MakeDelegate<System::Action*>(classof(System::Action*),
                    (std::function<void()>)[this, startIndex, endIndex, &customLevelsFolders, &threadsFinished, &loadedPaths, &valuesMutex] { 
                        for(int i = startIndex; i < endIndex; i++) {
                            std::string const& songPath = customLevelsFolders[i];
                            LOG_INFO("Loading %s ...", songPath.c_str());
                            try {
                                auto startLevel = std::chrono::high_resolution_clock::now(); 
                                bool wip = songPath.find(CustomWIPLevelsFolder) != std::string::npos;
                                
                                CustomPreviewBeatmapLevel* level = nullptr;
                                auto songPathCS = il2cpp_utils::newcsstr(songPath);
                                bool containsKey = CustomLevels->ContainsKey(songPathCS);
                                if(containsKey) {
                                    level = reinterpret_cast<CustomPreviewBeatmapLevel*>(CustomLevels->get_Item(songPathCS));
                                } else {
                                    containsKey = CustomWIPLevels->ContainsKey(songPathCS);
                                    if(containsKey) 
                                        level = reinterpret_cast<CustomPreviewBeatmapLevel*>(CustomWIPLevels->get_Item(songPathCS));
                                }
                                if(!level) {
                                    CustomJSONData::CustomLevelInfoSaveData* saveData = GetStandardLevelInfoSaveData(songPath);
                                    std::string hash;
                                    level = LoadCustomPreviewBeatmapLevel(songPath, wip, saveData, hash);
                                }
                                if(level) { 
                                    std::lock_guard<std::mutex> lock(valuesMutex);
                                    if(!containsKey) {
                                        if(wip) {
                                            CustomWIPLevels->Add(songPathCS, level);
                                        } else {
                                            CustomLevels->Add(songPathCS, level);
                                        }
                                    }
                                    loadedPaths.push_back(songPath);
                                    CurrentFolder++;
                                    std::chrono::milliseconds durationLevel = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startLevel);
                                    LOG_INFO("Loaded %s in %dms!", songPath.c_str(), (int)durationLevel.count());
                                } else {
                                    LOG_ERROR("Failed loading %s!", songPath.c_str());
                                }
                            } catch (...) {
                                LOG_ERROR("Failed loading %s!", songPath.c_str());
                            }
                        }
                        threadsFinished++;
                    }
                ), nullptr)->Run();
            }
            //Wait for threads to finish
            while(threadsFinished < threadsCount) {
                Thread::Yield();
            }

            auto customPreviewLevels = static_cast<Array<CustomPreviewBeatmapLevel*>*>(GetDictionaryValues(CustomLevels));
            auto customWIPPreviewLevels = static_cast<Array<CustomPreviewBeatmapLevel*>*>(GetDictionaryValues(CustomWIPLevels));
            
            CustomLevelsPack->SetCustomPreviewBeatmapLevels(customPreviewLevels);
            CustomWIPLevelsPack->SetCustomPreviewBeatmapLevels(customWIPPreviewLevels);

            int levelsCount = customPreviewLevels->Length() + customWIPPreviewLevels->Length();
            
            auto duration = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start); 
            
            LoadingUI::UpdateLoadedProgress(levelsCount, duration.count());
            LOG_INFO("Loaded %d songs in %dms!", levelsCount, (int)duration.count());
            
            LoadedLevels.clear();
            LoadedLevels.insert(LoadedLevels.end(), customPreviewLevels->values, customPreviewLevels->values + customPreviewLevels->Length());
            LoadedLevels.insert(LoadedLevels.end(), customWIPPreviewLevels->values, customWIPPreviewLevels->values + customWIPPreviewLevels->Length());
            
            QuestUI::MainThreadScheduler::Schedule(
                [this, songsLoaded] {
                    
                    RefreshLevelPacks(true);

                    IsLoading = false;
                    HasLoaded = true;

                    if(songsLoaded)
                        songsLoaded(LoadedLevels);

                    std::lock_guard<std::mutex> lock(LoadedEventsMutex);
                    for (auto& event : LoadedEvents) {
                        event(LoadedLevels);
                    }
                }
            );

            CacheUtils::SaveToFile(loadedPaths);
        }
    ), nullptr)->Run();
}

void SongLoader::DeleteSong(std::string_view path, std::function<void()> const& finished) {
    HMTask::New_ctor(il2cpp_utils::MakeDelegate<System::Action*>(classof(System::Action*),
        (std::function<void()>)[this, path, finished] {
            FileUtils::DeleteFolder(path);
            auto songPathCS = il2cpp_utils::newcsstr(path);
            CustomLevels->Remove(songPathCS);
            CustomWIPLevels->Remove(songPathCS);
            LOG_INFO("Deleted Song %s!", path.data());
            finished();
        }
    ), nullptr)->Run();
}
