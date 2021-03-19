#include <chrono>

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "custom-types/shared/register.hpp"

#include "CustomLogger.hpp"
#include "CustomConfig.hpp"

#include "Paths.hpp"

#include "LoadingFixHooks.hpp"
#include "CustomCharacteristics.hpp"
#include "CustomBeatmapLevelLoader.hpp"

#include "Utils/FindComponentsUtils.hpp"
#include "Utils/CacheUtils.hpp"

#include "CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp"
#include "CustomTypes/SongLoader.hpp"

#include "questui/shared/QuestUI.hpp"

#include "UnityEngine/SceneManagement/Scene.hpp"

ModInfo modInfo;

Logger& getLogger() {
    static auto logger = new Logger(modInfo, LoggerOptions(false, true)); 
    return *logger; 
}

Configuration& getConfig() {
    static Configuration config(modInfo);
    return config;
}

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace RuntimeSongLoader;

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
            CustomCharacteristics::SetupCustomCharacteristics();
            FindComponentsUtils::ClearCache();
            SongLoader::GetInstance()->RefreshSongs(true);
            
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
    QuestUI::Init();
    custom_types::Register::RegisterTypes<SongLoaderBeatmapLevelPackCollectionSO, SongLoader>();
    INSTALL_HOOK_OFFSETLESS(getLogger(), SceneManager_Internal_ActiveSceneChanged, il2cpp_utils::FindMethodUnsafe("UnityEngine.SceneManagement", "SceneManager", "Internal_ActiveSceneChanged", 2));
    CustomBeatmapLevelLoader::InstallHooks();
    LoadingFixHooks::InstallHooks();
    CustomCharacteristics::InstallHooks();
    LOG_INFO("Successfully installed SongLoader!");
}