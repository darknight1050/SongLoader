#include <chrono>

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "custom-types/shared/register.hpp"

#include "CustomLogger.hpp"
#include "CustomConfig.hpp"

#include "Paths.hpp"
#include "Sprites.hpp"

#include "LoadingFixHooks.hpp"
#include "CustomCharacteristics.hpp"
#include "CustomBeatmapLevelLoader.hpp"
#include "LoadingUI.hpp"

#include "Utils/FindComponentsUtils.hpp"
#include "Utils/CacheUtils.hpp"

#include "ModSettingsViewController.hpp"
#include "CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp"
#include "CustomTypes/SongLoader.hpp"
#include "API.hpp"

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"

#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "HMUI/ImageView.hpp"
#include "HMUI/ViewController.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "HMUI/TitleViewController.hpp"
#include "VRUIControls/VRGraphicRaycaster.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/ContentSizeFitter.hpp"
#include "UnityEngine/Events/UnityAction.hpp"
#include "UnityEngine/SceneManagement/Scene.hpp"
#include "System/Action_1.hpp"

ModInfo modInfo;

Logger& getLogger() {
    static auto logger = new Logger(modInfo, LoggerOptions(false, true)); 
    return *logger; 
}

Configuration& getConfig() {
    static Configuration config(modInfo);
    return config;
}

std::string GetBaseLevelsPath() {
    static std::string baseLevelsPath(getDataDir(modInfo));
    return baseLevelsPath;
}

using namespace GlobalNamespace;
using namespace HMUI;
using namespace VRUIControls;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace UnityEngine::Events;
using namespace QuestUI;
using namespace RuntimeSongLoader;

MAKE_HOOK_OFFSETLESS(SceneManager_Internal_ActiveSceneChanged, void, UnityEngine::SceneManagement::Scene prevScene, UnityEngine::SceneManagement::Scene nextScene) {
    SceneManager_Internal_ActiveSceneChanged(prevScene, nextScene);
    if(prevScene.IsValid() && nextScene.IsValid()) {
        std::u16string prevSceneName(csstrtostr(prevScene.get_name()));
        std::u16string nextSceneName(csstrtostr(nextScene.get_name()));
        static bool hasInited = false;
        if(prevSceneName == u"QuestInit"){
            hasInited = true;
        }
        if(nextSceneName.find(u"Menu") != std::string::npos) {
            if(hasInited && prevSceneName == u"EmptyTransition") {
                hasInited = false;
                CustomCharacteristics::SetupCustomCharacteristics();
                FindComponentsUtils::ClearCache();
                API::RefreshSongs(false);
            }
        } else {
            LoadingUI::SetActive(false);
        }
    }
}

MAKE_HOOK_OFFSETLESS(StandardLevelDetailView_RefreshContent, void, StandardLevelDetailView* self) {
    StandardLevelDetailView_RefreshContent(self);

    static SimpleDialogPromptViewController* deleteDialogPromptViewController = nullptr;
    static auto deleteLevelButtonName = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>("DeleteLevelButton");
    auto templateButton = self->practiceButton;
    auto parent = templateButton->get_transform()->get_parent();
    auto deleteLevelButtonTransform = parent->Find(deleteLevelButtonName);
    GameObject* deleteLevelButtonGameObject = nullptr;
    if(deleteLevelButtonTransform) {
        deleteLevelButtonGameObject = deleteLevelButtonTransform->get_gameObject();
    } else {
        deleteDialogPromptViewController = Object::Instantiate<SimpleDialogPromptViewController*>(FindComponentsUtils::GetSimpleDialogPromptViewController());
        deleteDialogPromptViewController->GetComponent<VRGraphicRaycaster*>()->physicsRaycaster = BeatSaberUI::GetPhysicsRaycasterWithCache();
        static auto dialogViewControllerName = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>("DeleteDialogPromptViewController");
        deleteDialogPromptViewController->set_name(dialogViewControllerName);
        deleteDialogPromptViewController->get_gameObject()->SetActive(false);

        deleteLevelButtonGameObject = Object::Instantiate(templateButton->get_gameObject(), parent);
        deleteLevelButtonTransform = deleteLevelButtonGameObject->get_transform();
        deleteLevelButtonGameObject->set_name(deleteLevelButtonName);
        static auto contentName = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>("Content");
        static auto textName = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>("Text");
        auto contentTransform = deleteLevelButtonTransform->Find(contentName);
        Object::Destroy(contentTransform->Find(textName)->get_gameObject());
        Object::Destroy(contentTransform->GetComponent<LayoutElement*>());
        static auto underlineName = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>("Underline");
        Object::Destroy(deleteLevelButtonTransform->Find(underlineName)->get_gameObject());

        static auto iconName = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>("Icon");
        auto iconGameObject = GameObject::New_ctor(iconName);
        auto imageView = iconGameObject->AddComponent<ImageView*>();
        auto iconTransform = imageView->get_rectTransform();
        iconTransform->SetParent(contentTransform, false);
        imageView->set_material(ArrayUtil::First(Resources::FindObjectsOfTypeAll<Material*>(), [] (Material* x) { return to_utf8(csstrtostr(x->get_name())) == "UINoGlow"; }));
        imageView->set_sprite(BeatSaberUI::Base64ToSprite(Sprites::DeleteLevelButtonIcon));
        imageView->set_preserveAspect(true);

        float scale = 1.7f;
        iconTransform->set_localScale(UnityEngine::Vector3(scale, scale, scale));

        ContentSizeFitter* contentSizeFitter = deleteLevelButtonGameObject->AddComponent<ContentSizeFitter*>();
        contentSizeFitter->set_verticalFit(ContentSizeFitter::FitMode::Unconstrained);
        contentSizeFitter->set_horizontalFit(ContentSizeFitter::FitMode::Unconstrained);

        deleteLevelButtonGameObject->GetComponent<LayoutElement*>()->set_preferredWidth(10.0f);

        deleteLevelButtonTransform->SetAsFirstSibling();

        auto onClick = Button::ButtonClickedEvent::New_ctor();
        onClick->AddListener(il2cpp_utils::MakeDelegate<UnityAction*>(classof(UnityAction*), (
            std::function<void()>) [self] () {
                static auto titleName = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>("Delete song");
                static auto deleteName = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>("Delete");
                static auto cancelName = il2cpp_utils::newcsstr<il2cpp_utils::CreationType::Manual>("Cancel");
                CustomPreviewBeatmapLevel* level = reinterpret_cast<CustomPreviewBeatmapLevel*>(self->level);
                auto text = u"Do you really want to delete \"" + std::u16string(csstrtostr(level->songName));
                auto songSubName = std::u16string(csstrtostr(level->songSubName));
                if(!songSubName.empty())
                    text += u" " + std::u16string(csstrtostr(level->songSubName));
                text += u"\"?";
                deleteDialogPromptViewController->Init(titleName, il2cpp_utils::newcsstr(text), deleteName, cancelName, il2cpp_utils::MakeDelegate<System::Action_1<int>*>(classof(System::Action_1<int>*), 
                    (std::function<void(int)>) [self, level] (int selectedButton) {
                        deleteDialogPromptViewController->__DismissViewController(nullptr, ViewController::AnimationDirection::Horizontal, false);
                        FindComponentsUtils::GetScreenSystem()->titleViewController->get_gameObject()->SetActive(true);
                        if (selectedButton == 0) {
                            RuntimeSongLoader::API::DeleteSong(to_utf8(csstrtostr(level->customLevelPath)), 
                                [] {
                                    RuntimeSongLoader::API::RefreshSongs(false);
                                }
                            );
                            
                        }
                    }
                ));
                FindComponentsUtils::GetScreenSystem()->titleViewController->get_gameObject()->SetActive(false);
                FindComponentsUtils::GetLevelSelectionNavigationController()->__PresentViewController(deleteDialogPromptViewController, nullptr, ViewController::AnimationDirection::Horizontal, false);
            }
        ));
        deleteLevelButtonGameObject->GetComponent<Button*>()->set_onClick(onClick);
    }
    deleteLevelButtonGameObject->SetActive(il2cpp_functions::class_is_assignable_from(classof(CustomPreviewBeatmapLevel*), il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(self->level))));
    
}

extern "C" void setup(ModInfo& info) {
    modInfo.id = "SongLoader";
    modInfo.version = VERSION;
    info = modInfo;

    auto baseLevelsPath = GetBaseLevelsPath();
    if(!direxists(baseLevelsPath))
        mkpath(baseLevelsPath);
    if(!direxists(baseLevelsPath + CustomLevelsFolder))
        mkpath(baseLevelsPath + CustomLevelsFolder);
    LOG_INFO("Base path is: %s", baseLevelsPath.c_str());
}

extern "C" void load() {
    LOG_INFO("Starting SongLoader installation...");
    il2cpp_functions::Init();
    QuestUI::Init();
    
    QuestUI::Register::RegisterModSettingsViewController(modInfo, DidActivate);

    custom_types::Register::RegisterTypes<SongLoaderBeatmapLevelPackCollectionSO, SongLoader>();
    INSTALL_HOOK_OFFSETLESS(getLogger(), SceneManager_Internal_ActiveSceneChanged, il2cpp_utils::FindMethodUnsafe("UnityEngine.SceneManagement", "SceneManager", "Internal_ActiveSceneChanged", 2));
    INSTALL_HOOK_OFFSETLESS(getLogger(), StandardLevelDetailView_RefreshContent, il2cpp_utils::FindMethodUnsafe("", "StandardLevelDetailView", "RefreshContent", 0));
    CustomBeatmapLevelLoader::InstallHooks();
    LoadingFixHooks::InstallHooks();
    CustomCharacteristics::InstallHooks();
    LOG_INFO("Successfully installed SongLoader!");
}