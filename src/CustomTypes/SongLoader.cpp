#include "CustomTypes/SongLoader.hpp"

#include "CustomLogger.hpp"

#include "Paths.hpp"
#include "assets.hpp"

#include "LoadingUI.hpp"

#include "API.hpp"

#include "custom-types/shared/delegate.hpp"

#include "Utils/HashUtils.hpp"
#include "Utils/FileUtils.hpp"
#include "Utils/CacheUtils.hpp"
#include "Utils/OggVorbisUtils.hpp"
#include "Utils/FindComponentsUtils.hpp"

#include "bsml/shared/BSML-Lite.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "bsml/shared/Helpers/getters.hpp"

#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/BeatmapBasicData.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/BeatmapDifficultySerializedMethods.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollection.hpp"
#include "GlobalNamespace/BeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapLevelPack.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/CachedMediaAsyncLoader.hpp"
#include "GlobalNamespace/ISpriteAsyncLoader.hpp"
#include "GlobalNamespace/ColorScheme.hpp"
#include "GlobalNamespace/BpmTimeProcessor.hpp"
#include "GlobalNamespace/EnvironmentsListModel.hpp"
#include "GlobalNamespace/FileSystemPreviewMediaData.hpp"
#include "GlobalNamespace/FileDifficultyBeatmap.hpp"
#include "GlobalNamespace/FileSystemBeatmapLevelData.hpp"
#include "GlobalNamespace/AudioClipAsyncLoader.hpp"
#include "GlobalNamespace/PlayerSaveData.hpp"
#include "BeatmapSaveDataVersion4/BeatmapSaveData.hpp"
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
#include "System/ValueTuple_2.hpp"
#include "System/Threading/Thread.hpp"
#include "System/Threading/CancellationToken.hpp"

#include <vector>
#include <atomic>
#include <time.h>

#define MAX_THREADS 8

using namespace RuntimeSongLoader;
using namespace GlobalNamespace;
using namespace BeatmapSaveDataVersion3;
using namespace UnityEngine;
using namespace System;
using namespace System::IO;
using namespace System::Threading;
using namespace System::Collections::Generic;
using namespace FindComponentsUtils;

#define FixEmptyString(name) \
if(!name) { \
    LOG_WARN("Fixed nullptr string \"%s\"! THIS SHOULDN'T HAPPEN!", #name);\
    name = ""; \
}

DEFINE_TYPE(RuntimeSongLoader, SongLoader);

SongLoader* SongLoader::Instance = nullptr;

SongLoader* SongLoader::GetInstance() {
    if(!Instance) {
        static ConstString name("SongLoader");
        auto gameObject = GameObject::New_ctor(name);
        GameObject::DontDestroyOnLoad(gameObject);
        Instance = gameObject->AddComponent<SongLoader*>();
    }
    return Instance;
}

std::vector<std::function<void(std::vector<BeatmapLevel*> const&)>> SongLoader::LoadedEvents;
std::mutex SongLoader::LoadedEventsMutex;

std::vector<std::function<void(SongLoaderBeatmapLevelsRepository*)>> SongLoader::RefreshLevelPacksEvents;
std::mutex SongLoader::RefreshLevelPacksEventsMutex;

std::vector<std::function<void()>> SongLoader::SongDeletedEvents;

std::vector<BeatmapLevel*> SongLoader::GetLoadedLevels() {
    return LoadedLevels;
}

void SongLoader::ctor() {
    INVOKE_CTOR();
    IsLoading = false;
    HasLoaded = false;
    LoadingCancelled = false;
    MaxFolders = 0;
    CurrentFolder = 0;
}

void SongLoader::Awake() {
    LOG_INFO("SongLoader Awake");
    beatmapDataLoader = BeatmapDataLoader::New_ctor();

    CustomLevels = Dictionary_2<StringW, BeatmapLevel*>::New_ctor();
    CustomWIPLevels = Dictionary_2<StringW, BeatmapLevel*>::New_ctor();

    LevelDatas = Dictionary_2<StringW, IBeatmapLevelData*>::New_ctor();

    time_t theTime = time(NULL);
    struct tm *aTime = localtime(&theTime);

    int day = aTime->tm_mday;
    int month = aTime->tm_mon + 1;

    bool aprilFools = day == 1 && month == 4;
    bool EVIL = aprilFools || false;
    if(!aprilFools) {
        if (!(rand() % 20)) {
            EVIL = true;
        }
    }

    CustomLevelsPack = SongLoaderCustomBeatmapLevelPack::Make_New(CustomLevelsFolder, "Custom Levels", EVIL ? BSML::Lite::ArrayToSprite(Assets::EVILCustomLevelsCover_png) : BSML::Lite::ArrayToSprite(Assets::CustomLevelsCover_png));
    CustomWIPLevelsPack = SongLoaderCustomBeatmapLevelPack::Make_New(CustomWIPLevelsFolder, "WIP Levels", BSML::Lite::ArrayToSprite(Assets::CustomWIPLevelsCover_png));
    CustomBeatmapLevelPackCollectionSO = RuntimeSongLoader::SongLoaderBeatmapLevelsRepository::CreateNew();

    // precache things we use off main thread
    FindComponentsUtils::GetLevelSelectionNavigationController();

    if(IsLoading)
        LoadingCancelled = true;
}

void SongLoader::Update() {
    if(IsLoading)
        LoadingUI::UpdateLoadingProgress(MaxFolders, CurrentFolder);
    if(!MenuOpened)
        return;
    LoadingUI::UpdateState();
}

void SongLoader::MenuLoaded() {
    if(queuedRefresh) {
        RefreshSongs(queuedRefresh.value(), queuedCallback);
        queuedRefresh = std::nullopt;
        queuedCallback = nullptr;
    }
}

CustomJSONData::CustomLevelInfoSaveData* SongLoader::GetStandardLevelInfoSaveData(std::string const& customLevelPath) {
    std::string path = customLevelPath + "/info.dat";
    if(!fileexists(path))
        path = customLevelPath + "/Info.dat";
    if(fileexists(path)) {
        try {
            auto standardLevelInfoSaveData = StandardLevelInfoSaveData::DeserializeFromJSONString(FileUtils::ReadAllText16(path));
            if (!standardLevelInfoSaveData) {
                LOG_ERROR("GetStandardLevelInfoSaveData Can't Load File %s!", (path).c_str());
                return nullptr;
            }
            auto optional = il2cpp_utils::try_cast<CustomJSONData::CustomLevelInfoSaveData>(standardLevelInfoSaveData);
            if (!optional.has_value()) {
                LOG_ERROR("GetStandardLevelInfoSaveData Can't Load File %s as CustomLevelInfoSaveData!", (path).c_str());
                return nullptr;
            }
            return optional.value();
        } catch(const std::runtime_error& e) {
            LOG_ERROR("GetStandardLevelInfoSaveData Can't Load File %s: %s!", (path).c_str(), e.what());
        }
    } else {
        LOG_ERROR("GetStandardLevelInfoSaveData File %s doesn't exist!", (path).c_str());
    }
    return nullptr;
}

ArrayW<ColorScheme*> SongLoader::LoadColorSchemes(ArrayW<BeatmapLevelColorSchemeSaveData*> colorSchemeDatas) {
    // if null input, just return 0 length
    if (!colorSchemeDatas) return ArrayW<ColorScheme*>::Empty();

    ListW<ColorScheme*> colorSchemes = ListW<ColorScheme*>::New();
    for (auto saveData : colorSchemeDatas) {
        auto colorScheme = saveData->colorScheme;
        if (colorScheme && saveData->useOverride) {
            colorSchemes->Add(
                ColorScheme::New_ctor(
                    colorScheme->colorSchemeId,
                    "",
                    false,
                    "",
                    false,
                    colorScheme->saberAColor,
                    colorScheme->saberBColor,
                    colorScheme->environmentColor0,
                    colorScheme->environmentColor1,
                    {1, 1, 1, 1},
                    true,
                    colorScheme->environmentColor0Boost,
                    colorScheme->environmentColor1Boost,
                    {1, 1, 1, 1},
                    colorScheme->obstaclesColor
                )
            );
        } else {
            colorSchemes->Add(nullptr);
        }
    }
    return colorSchemes->ToArray();
}

ValueTuple_2<BeatmapLevel*, IBeatmapLevelData*> SongLoader::LoadBeatmapLevel(std::string const& customLevelPath, bool wip, CustomJSONData::CustomLevelInfoSaveData* standardLevelInfoSaveData, std::string& outHash) {
    static auto logger = getLogger().WithContext("LoadBeatmapLevel");
    RET_0_UNLESS(logger, standardLevelInfoSaveData);

    LOG_DEBUG("LoadBeatmapLevel StandardLevelInfoSaveData: ");
    auto hashOpt = HashUtils::GetCustomLevelHash(standardLevelInfoSaveData, customLevelPath);
    RET_0_UNLESS(logger, hashOpt);

    outHash = *hashOpt;
    std::string stringLevelID = CustomLevelPrefixID + outHash;
    if(wip)
        stringLevelID += " WIP";
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
    LOG_DEBUG("songName: %s", static_cast<std::string>(songName).c_str());
    LOG_DEBUG("songSubName: %s", static_cast<std::string>(songSubName).c_str());
    LOG_DEBUG("songAuthorName: %s", static_cast<std::string>(songAuthorName).c_str());
    LOG_DEBUG("levelAuthorName: %s", static_cast<std::string>(levelAuthorName).c_str());
    LOG_DEBUG("beatsPerMinute: %f", beatsPerMinute);
    LOG_DEBUG("songTimeOffset: %f", songTimeOffset);
    LOG_DEBUG("shuffle: %f", shuffle);
    LOG_DEBUG("shufflePeriod: %f", shufflePeriod);
    LOG_DEBUG("previewStartTime: %f", previewStartTime);
    LOG_DEBUG("previewDuration: %f", previewDuration);

    ArrayW<ColorScheme*> colorSchemes = LoadColorSchemes(standardLevelInfoSaveData->colorSchemes);

	auto createEnvironmentName = [&](StringW environmentSerializedField, bool allDirections = false) {
        auto environmentInfoSO = environmentsListModel->GetEnvironmentInfoBySerializedName(environmentSerializedField);
        if(!environmentInfoSO) {
            environmentInfoSO = environmentsListModel->GetEnvironmentInfoBySerializedName(allDirections ? "GlassDesertEnvironment" : "TriangleEnvironment");
        }
        return EnvironmentName(environmentInfoSO->serializedName);
    };

	bool hasEnvironment = standardLevelInfoSaveData->environmentNames && standardLevelInfoSaveData->environmentNames.size() != 0;
	ListW<GlobalNamespace::EnvironmentName> list = ListW<GlobalNamespace::EnvironmentName>::New();

	if (!hasEnvironment) {
		list->Add(createEnvironmentName(standardLevelInfoSaveData->environmentName, false));
		list->Add(createEnvironmentName(standardLevelInfoSaveData->allDirectionsEnvironmentName, true));
	} else {
        for (size_t i = 0; i < standardLevelInfoSaveData->_environmentNames.size(); i++) {
            list->Add(createEnvironmentName(standardLevelInfoSaveData->_environmentNames[i], false));
        }
	}

	auto dict = System::Collections::Generic::Dictionary_2<ValueTuple_2<UnityW<BeatmapCharacteristicSO>, BeatmapDifficulty>, BeatmapBasicData*>::New_ctor();

    for(auto difficultyBeatmapSet : standardLevelInfoSaveData->difficultyBeatmapSets) {
        if (!difficultyBeatmapSet) continue;

        auto beatmapCharacteristicBySerializedName = beatmapCharacteristicCollection->GetBeatmapCharacteristicBySerializedName(difficultyBeatmapSet->beatmapCharacteristicName);

        if (beatmapCharacteristicBySerializedName) {
            for(int j = 0; j < difficultyBeatmapSet->difficultyBeatmaps.size(); j++) {
                auto diffMap = difficultyBeatmapSet->difficultyBeatmaps[j];

				GlobalNamespace::BeatmapDifficulty beatmapDifficulty;
				BeatmapDifficultySerializedMethods::BeatmapDifficultyFromSerializedName(
                    difficultyBeatmapSet->difficultyBeatmaps[j]->difficulty,
                    byref(beatmapDifficulty));

				dict->TryAdd(
                    ValueTuple_2<UnityW<BeatmapCharacteristicSO>, BeatmapDifficulty>(
                        beatmapCharacteristicBySerializedName,
                        beatmapDifficulty
                    ),
					BeatmapBasicData::New_ctor(
						diffMap->noteJumpMovementSpeed,
                        diffMap->noteJumpStartBeatOffset,
						hasEnvironment ? list[diffMap->environmentNameIdx] : (beatmapCharacteristicBySerializedName->containsRotationEvents ? list[1] : list[0]),
						(diffMap->beatmapColorSchemeIdx >= 0 && diffMap->beatmapColorSchemeIdx < colorSchemes.size()) ? colorSchemes[diffMap->beatmapColorSchemeIdx] : nullptr,
						0,
						0,
						0,
						ArrayW<StringW>(),
						ArrayW<StringW>()
                    )
                );
            }
        }
    }

    static SafePtr<CachedMediaAsyncLoader> cachedMediaAsyncLoader = CachedMediaAsyncLoader::New_ctor();
    LOG_DEBUG("LoadBeatmapLevel Stop");
    auto result = BeatmapLevel::New_ctor(
		false,
		stringLevelID,
        songName,
        songSubName,
        songAuthorName,
		{ levelAuthorName },
		{},
        beatsPerMinute,
		-6.0f,
		songTimeOffset,
		previewStartTime,
        previewDuration,
		0.0f,
		::GlobalNamespace::PlayerSensitivityFlag::Safe,
		FileSystemPreviewMediaData::New_ctor(cachedMediaAsyncLoader->i___GlobalNamespace__ISpriteAsyncLoader(), AudioClipAsyncLoader::CreateDefault(), customLevelPath, standardLevelInfoSaveData->coverImageFilename, standardLevelInfoSaveData->songFilename)->i___GlobalNamespace__IPreviewMediaData(),
        dict->i___System__Collections__Generic__IReadOnlyDictionary_2_TKey_TValue_()
    );
    
    UpdateSongDuration(result, customLevelPath, standardLevelInfoSaveData->songFilename);

    // Load IBeatmapLevelData
    IBeatmapLevelData* levelData = nullptr;
    {
        auto dataDictionary = System::Collections::Generic::Dictionary_2<ValueTuple_2<UnityW<BeatmapCharacteristicSO>, BeatmapDifficulty>, FileDifficultyBeatmap*>::New_ctor();

        for (auto difficultyBeatmapSet : standardLevelInfoSaveData->difficultyBeatmapSets) {
            auto beatmapCharacteristicBySerializedName = beatmapCharacteristicCollection->GetBeatmapCharacteristicBySerializedName(difficultyBeatmapSet->beatmapCharacteristicName);
            if (beatmapCharacteristicBySerializedName != nullptr) {
                for (auto difficultyBeatmap : difficultyBeatmapSet->difficultyBeatmaps) {
                    BeatmapDifficulty beatmapDifficulty;
                    if (BeatmapDifficultySerializedMethods::BeatmapDifficultyFromSerializedName(difficultyBeatmap->difficulty, byref(beatmapDifficulty))) {
                        auto beatmapPath = customLevelPath + "/" + static_cast<std::string>(difficultyBeatmap->beatmapFilename);
                        dataDictionary->TryAdd(ValueTuple_2<UnityW<BeatmapCharacteristicSO>, BeatmapDifficulty>(beatmapCharacteristicBySerializedName, beatmapDifficulty), FileDifficultyBeatmap::New_ctor("", beatmapPath, ""));
                    }
                }
            }
        }
        levelData = FileSystemBeatmapLevelData::New_ctor(standardLevelInfoSaveData->songName, customLevelPath + "/" + static_cast<std::string>(standardLevelInfoSaveData->songFilename), "", dataDictionary)->i___GlobalNamespace__IBeatmapLevelData();
    }

    return ValueTuple_2<BeatmapLevel*, IBeatmapLevelData*>(result, levelData);
}

void SongLoader::UpdateSongDuration(BeatmapLevel* level, std::string const& customLevelPath, std::string const& songFilename) {
    float length = 0.0f;
    auto cacheDataOpt = CacheUtils::GetCacheData(customLevelPath);
    if(!cacheDataOpt.has_value())
        return;
    auto cacheData = *cacheDataOpt;
    auto cacheSongDuration = cacheData.songDuration;
    if(cacheSongDuration.has_value()) {
        length = *cacheSongDuration;
    } else {
        if(length <= 0.0f || length == INFINITY)
            length = OggVorbisUtils::GetLengthFromOggVorbisFile(customLevelPath + "/" + songFilename);
    }
    if(length < 0.0f || length == INFINITY)
        length = 0.0f;
    level->songDuration = length;
    cacheData.songDuration = length;
    CacheUtils::UpdateCacheData(customLevelPath, cacheData);
}

template<typename Out, typename T, class Predicate>
static inline Out Max(ArrayW<T> array, Predicate pred) {
    Out max = std::numeric_limits<Out>::min();
    for (auto item : array) {
        auto value = pred(item);
        if (value > max) max = value;
    }
    return max;
}

ArrayW<BeatmapLevel*> GetDictionaryValues(Dictionary_2<StringW, BeatmapLevel*>* dictionary) {
    if(!dictionary)
        return ArrayW<BeatmapLevel*>();
    auto array = ArrayW<BeatmapLevel*>(dictionary->get_Count());
    dictionary->get_Values()->CopyTo(array, 0);
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

    CustomLevelsPack->CustomLevelsPack->beatmapLevels = CustomLevelsPack->CustomLevelsCollection;
    CustomWIPLevelsPack->CustomLevelsPack->beatmapLevels = CustomWIPLevelsPack->CustomLevelsCollection;

    std::lock_guard<std::mutex> lock(RefreshLevelPacksEventsMutex);
    for (auto& event : RefreshLevelPacksEvents) {
        event(CustomBeatmapLevelPackCollectionSO);
    }

    static SafePtrUnity<LevelFilteringNavigationController> levelFilteringNavigationController;
    if (!levelFilteringNavigationController)
        levelFilteringNavigationController = Resources::FindObjectsOfTypeAll<LevelFilteringNavigationController*>()->FirstOrDefault();

    levelFilteringNavigationController->_beatmapLevelsModel->UpdateLoadedPreviewLevels();

    if(levelFilteringNavigationController)
        levelFilteringNavigationController->UpdateCustomSongs();
}

void SongLoader::RefreshSong_thread(std::atomic_int& index, std::atomic_int& threadsFinished, std::vector<std::string>& customLevelsFolders, std::vector<std::string>& loadedPaths, std::mutex& valuesMutex) {
    int i = index++;
    while(i < MaxFolders) {
        std::string const& songPath = customLevelsFolders.at(i);
        LOG_INFO("Loading %s ...", songPath.c_str());
        try {
            auto startLevel = std::chrono::high_resolution_clock::now();
            bool wip = songPath.find(CustomWIPLevelsFolder) != std::string::npos;

            BeatmapLevel* level = nullptr;
            IBeatmapLevelData* levelData = nullptr;
            auto songPathCS = StringW(songPath);
            bool containsKey = CustomLevels->ContainsKey(songPathCS);

            if(containsKey) {
                level = reinterpret_cast<BeatmapLevel*>(CustomLevels->get_Item(songPathCS));
            } else {
                containsKey = CustomWIPLevels->ContainsKey(songPathCS);
                if(containsKey)
                    level = reinterpret_cast<BeatmapLevel*>(CustomWIPLevels->get_Item(songPathCS));
            }

            if(!level) {
                CustomJSONData::CustomLevelInfoSaveData* saveData = GetStandardLevelInfoSaveData(songPath);
                std::string hash;
                auto pair = LoadBeatmapLevel(songPath, wip, saveData, hash);
                level = pair.Item1;
                levelData = pair.Item2;
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

                if(levelData != nullptr) {
                    LevelDatas->Add(level->levelID, levelData);
                }

                loadedPaths.push_back(songPath);
                CurrentFolder++;
                std::chrono::milliseconds durationLevel = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startLevel);
                LOG_INFO("Loaded %s in %dms!", songPath.c_str(), (int)durationLevel.count());
            } else {
                LOG_ERROR("Failed loading %s, null level returned!", songPath.c_str());
            }
        } catch(il2cpp_utils::RunMethodException const& e) {
            LOG_ERROR("RunMethodException thrown while loading %s: %s", songPath.c_str(), e.what());
            e.log_backtrace();
        } catch (std::exception const& e) {
            LOG_ERROR("Exception thrown while loading %s: %s", songPath.c_str(), e.what());
        } catch (...) {
            LOG_ERROR("Unknown exception thrown while loading %s!", songPath.c_str());
        }
        i = index++;
    }
    threadsFinished++;
}

void SongLoader::RefreshSongs_internal(bool fullRefresh, std::function<void(std::vector<BeatmapLevel*> const&)> songsLoaded) {
    LOG_INFO("RefreshSongs_internal");

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
    std::atomic_int threadsFinished = 0;
    std::atomic_int index = 0;
    int threadsCount = std::min(MaxFolders, MAX_THREADS);

    std::vector<il2cpp_utils::il2cpp_aware_thread> refresh_threads;
    for(int threadIndex = 0; threadIndex < threadsCount; threadIndex++) {
        LOG_INFO("creating thread %d", threadIndex);

        refresh_threads.emplace_back(
            [&](){
                SongLoader::RefreshSong_thread(
                    index,
                    threadsFinished,
                    customLevelsFolders,
                    loadedPaths,
                    valuesMutex
                );
            }
        );
    }

    //Wait for threads to finish
    for (auto& thread : refresh_threads) {
        thread.join();
    }

    auto customPreviewLevels = GetDictionaryValues(CustomLevels);
    auto customWIPPreviewLevels = GetDictionaryValues(CustomWIPLevels);

    CustomLevelsPack->SetCustomBeatmapLevels(customPreviewLevels);
    CustomWIPLevelsPack->SetCustomBeatmapLevels(customWIPPreviewLevels);

    int levelsCount = customPreviewLevels.size() + customWIPPreviewLevels.size();

    auto duration = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);

    LoadingUI::UpdateLoadedProgress(levelsCount, duration.count());
    LOG_INFO("Loaded %d songs in %dms!", levelsCount, (int)duration.count());

    LoadedLevels.clear();
    LoadedLevels.insert(LoadedLevels.end(), customPreviewLevels.begin(), customPreviewLevels.end());
    LoadedLevels.insert(LoadedLevels.end(), customWIPPreviewLevels.begin(), customWIPPreviewLevels.end());

    BSML::MainThreadScheduler::Schedule(
        [this, songsLoaded] {
            LOG_INFO("Loading songs done, invoking levelpack load");

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

void SongLoader::RefreshSongs(bool fullRefresh, std::function<void(std::vector<BeatmapLevel*> const&)> const& songsLoaded) {
    if(IsLoading)
        return;

    auto activeScene = SceneManagement::SceneManager::GetActiveScene();
    std::string activeSceneName = "Invalid";
    if (activeScene.IsValid()) activeSceneName = static_cast<std::string>(activeScene.get_name());
    LOG_INFO("Active scene: %s", activeSceneName.c_str());

    if(!activeScene.IsValid() || static_cast<std::string>(activeScene.get_name()).find("Menu") == std::string::npos) {
        LOG_INFO("Queueing refresh for later");
        queuedRefresh = queuedRefresh.value_or(fullRefresh) | fullRefresh;
        if(songsLoaded) {
            queuedCallback = [songsLoaded = std::move(songsLoaded), queuedCallback = std::move(queuedCallback)](std::vector<BeatmapLevel*> const& levels) {
                if(queuedCallback)
                    queuedCallback(levels);
                songsLoaded(levels);
            };
        }
        return;
    }

    IsLoading = true;
    HasLoaded = false;
    CurrentFolder = 0;

    LOG_INFO("Creating bound func");


    il2cpp_utils::il2cpp_aware_thread(
        &SongLoader::RefreshSongs_internal,
        this,
        fullRefresh,
        songsLoaded
    ).detach();
}

void SongLoader::DeleteSong(std::string_view path, std::function<void()> const& finished) {
    FileUtils::DeleteFolder(path);
    auto songPathCS = StringW(path);
    CustomLevels->Remove(songPathCS);
    CustomWIPLevels->Remove(songPathCS);
    LOG_INFO("Deleted Song %s!", path.data());
    for (auto& event : SongDeletedEvents) {
        event();
    }
    finished();
}
