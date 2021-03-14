#include <chrono>

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "custom-types/shared/register.hpp"

#include "CustomLogger.hpp"

#include "Paths.hpp"

#include "LoadingFixHooks.hpp"
#include "CustomCharacteristics.hpp"
#include "CustomBeatmapLevelLoader.hpp"

#include "Utils/FindComponentsUtils.hpp"

#include "CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp"
#include "CustomTypes/SongLoader.hpp"

#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/HMTask.hpp"
#include "UnityEngine/SceneManagement/Scene.hpp"
#include "System/Action.hpp"
#include "System/IO/Path.hpp"
#include "System/IO/Directory.hpp"

static ModInfo modInfo;

Logger& getLogger() {
    static auto logger = new Logger(modInfo, LoggerOptions(false, true)); 
    return *logger; 
}

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace System::IO;
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
    INSTALL_HOOK_OFFSETLESS(getLogger(), SceneManager_Internal_ActiveSceneChanged, il2cpp_utils::FindMethodUnsafe("UnityEngine.SceneManagement", "SceneManager", "Internal_ActiveSceneChanged", 2));
    CustomBeatmapLevelLoader::InstallHooks();
    LoadingFixHooks::InstallHooks();
    CustomCharacteristics::InstallHooks();
    LOG_INFO("Successfully installed SongLoader!");
}