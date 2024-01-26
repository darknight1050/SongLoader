#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "custom-types/shared/register.hpp"
#include "custom-types/shared/delegate.hpp"

#include "CustomLogger.hpp"
#include "CustomConfig.hpp"

#include "Paths.hpp"
#include "assets.hpp"
#include "LevelData.hpp"

#include "CustomBeatmapLevelLoader.hpp"
#include "CustomCharacteristics.hpp"
#include "LoadingFixHooks.hpp"
#include "LoadingUI.hpp"

#include "Utils/FindComponentsUtils.hpp"
#include "Utils/CacheUtils.hpp"

#include "ModSettingsViewController.hpp"
#include "CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp"
#include "CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"
#include "CustomTypes/SongLoader.hpp"
#include "API.hpp"

#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "GlobalNamespace/LoadingControl.hpp"
#include "GlobalNamespace/PlayerDataFileManagerSO.hpp"
#include "GlobalNamespace/PlayerSaveData.hpp"
#include "HMUI/ImageView.hpp"
#include "HMUI/ViewController.hpp"
#include "HMUI/TitleViewController.hpp"
#include "TMPro/TextOverflowModes.hpp"
#include "TMPro/TextAlignmentOptions.hpp"
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
#include "UnityEngine/SceneManagement/SceneManager.hpp"
#include "GlobalNamespace/RichPresenceManager.hpp"
#include "GlobalNamespace/ScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/MenuScenesTransitionSetupDataSO.hpp"
#include "Zenject/DiContainer.hpp"
#include "System/Action_1.hpp"

#include "bsml/shared/BSML.hpp"

modloader::ModInfo modInfo{MOD_ID, VERSION, 0};

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
using namespace RuntimeSongLoader;

// These don't need to be atomics since they will only ever be called by the main thread
static bool hasInited = false;
static bool shouldRefresh = false;

MAKE_HOOK_MATCH(RichPresenceManager_HandleGameScenesManagerTransitionDidFinish,
                &GlobalNamespace::RichPresenceManager::HandleGameScenesManagerTransitionDidFinish,
                void, GlobalNamespace::RichPresenceManager* self, GlobalNamespace::ScenesTransitionSetupDataSO* setupData,
                Zenject::DiContainer* container) {
    // We can always safely call our orig first
    RichPresenceManager_HandleGameScenesManagerTransitionDidFinish(self, setupData, container);
    // First, check to see if we need to refresh. If we do, do that right away.
    if (shouldRefresh) {
        shouldRefresh = false;
        hasInited = false;
        FindComponentsUtils::ClearCache();
        API::RefreshSongs(false);
        return;
    }
    // First, check to make sure both instances are non-null
    // This is mostly just a failsafe
    if (self == nullptr || self->_menuScenesTransitionSetupData == nullptr) [[unlikely]] {
        LOG_WARN("SHOULD NOT GET TO THIS POINT! " EXPAND_FILE " self: %p, with potentially also null menuScenesTransitionSetupData!", self);
        return;
    }
    if (setupData == nullptr && self->_menuWasLoaded) {
        // We know the menu was loaded once already, so we would be displaying the menu transition at this point.
        // So, we can simply perform a MenuLoad at this point.
        SongLoader::GetInstance()->MenuLoaded();
    } else if (setupData != nullptr && static_cast<void*>(setupData->m_CachedPtr) == static_cast<void*>(self->_menuScenesTransitionSetupData->m_CachedPtr)) {
        // We check to see if we have transitioned to anything to do with menu
        // We do this by doing a raw unity pointer check
        SongLoader::GetInstance()->MenuLoaded();
    } else {
        // We know we aren't in a menu scene, so set the loading UI activity to false
        LoadingUI::SetActive(false);
    }
}

MAKE_HOOK_MATCH(SceneManager_Internal_ActiveSceneChanged,
                &UnityEngine::SceneManagement::SceneManager::Internal_ActiveSceneChanged,
                void, UnityEngine::SceneManagement::Scene prevScene, UnityEngine::SceneManagement::Scene nextScene) {
    SceneManager_Internal_ActiveSceneChanged(prevScene, nextScene);
    if(prevScene.IsValid() && nextScene.IsValid()) {
        std::u16string_view prevSceneName(prevScene.get_name());
        std::u16string_view nextSceneName(nextScene.get_name());
        static bool hasInited = false;
        if(prevSceneName == u"QuestInit"){
            hasInited = true;
        }
        if(nextSceneName.find(u"Menu") != std::string::npos) {
            LevelData::difficultyBeatmap = nullptr;
            if(hasInited && prevSceneName == u"EmptyTransition") {
                shouldRefresh = true;
            }
        } else {
            LoadingUI::SetActive(false);
        }
    }
}

ModalView* deleteDialogPromptModal = nullptr;
CustomPreviewBeatmapLevel* selectedlevel = nullptr;

ModalView* getDeleteDialogPromptModal(std::u16string const& songName) {
    static TMPro::TextMeshProUGUI* songText = nullptr;
    if(!deleteDialogPromptModal) {
        songText = nullptr;
        deleteDialogPromptModal = BSML::Lite::CreateModal(FindComponentsUtils::GetLevelSelectionNavigationController(), Vector2(60, 30), nullptr);

        static ConstString contentName("Content");
        auto deleteButton = BSML::Lite::CreateUIButton(deleteDialogPromptModal, "Delete", Vector2(-15, -8.25), [] {
            deleteDialogPromptModal->Hide(true, nullptr);
            RuntimeSongLoader::API::DeleteSong(static_cast<std::string>(selectedlevel->customLevelPath),
                [] {
                    RuntimeSongLoader::API::RefreshSongs(false);
                }
            );
        });
        Object::Destroy(deleteButton->get_transform()->Find(contentName)->GetComponent<LayoutElement*>());
        auto cancelButton = BSML::Lite::CreateUIButton(deleteDialogPromptModal, "Cancel", Vector2(15, -8.25), [] {
            deleteDialogPromptModal->Hide(true, nullptr);
        });
        Object::Destroy(cancelButton->get_transform()->Find(contentName)->GetComponent<LayoutElement*>());
    }
    if(!songText) {
        songText = BSML::Lite::CreateText(deleteDialogPromptModal, u"Do you really want to delete \"" + songName + u"\"?", false, {0, 5}, {55, 20});
        songText->set_enableWordWrapping(true);
        songText->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);
        songText->set_alignment(TMPro::TextAlignmentOptions::Center);
    } else
        songText->set_text(u"Do you really want to delete \"" + songName + u"\"?");
    deleteDialogPromptModal->get_transform()->SetAsLastSibling();
    return deleteDialogPromptModal;
}


std::function<void()> getDeleteFunction() {
    static std::function<void()> deleteFunction = (std::function<void()>) [] () {
        std::u16string subName = selectedlevel->songSubName;
        if(!subName.empty())
            subName = u" " + subName;
        getDeleteDialogPromptModal(selectedlevel->songName + subName)->Show(true, true, nullptr);
    };
    return deleteFunction;
}

Button::ButtonClickedEvent* createDeleteOnClick() {
    auto onClick = Button::ButtonClickedEvent::New_ctor();
    onClick->AddListener(custom_types::MakeDelegate<UnityAction*>(getDeleteFunction()));
    return onClick;
}

MAKE_HOOK_MATCH(StandardLevelDetailView_RefreshContent,
                &StandardLevelDetailView::RefreshContent,
                void, StandardLevelDetailView* self) {
    LOG_DEBUG("StandardLevelDetailView_RefreshContent");
    StandardLevelDetailView_RefreshContent(self);
    static ConstString deleteLevelButtonName("DeleteLevelButton");
    auto templateButton = self->practiceButton;
    auto parent = templateButton->get_transform()->get_parent();
    auto deleteLevelButtonTransform = parent->Find(deleteLevelButtonName);
    GameObject* deleteLevelButtonGameObject = nullptr;
    if(deleteLevelButtonTransform) {
        deleteLevelButtonGameObject = deleteLevelButtonTransform->get_gameObject();
    } else {
        deleteDialogPromptModal = nullptr;
        deleteLevelButtonGameObject = Object::Instantiate(templateButton->get_gameObject(), parent);
        deleteLevelButtonTransform = deleteLevelButtonGameObject->get_transform();
        deleteLevelButtonGameObject->set_name(deleteLevelButtonName);
        static ConstString contentName("Content");
        static ConstString textName("Text");
        auto contentTransform = deleteLevelButtonTransform->Find(contentName);
        Object::Destroy(contentTransform->Find(textName)->get_gameObject());
        Object::Destroy(contentTransform->GetComponent<LayoutElement*>());
        static ConstString underlineName("Underline");
        Object::Destroy(deleteLevelButtonTransform->Find(underlineName)->get_gameObject());

        static ConstString iconName("Icon");
        auto iconGameObject = GameObject::New_ctor(iconName);
        auto imageView = iconGameObject->AddComponent<ImageView*>();
        auto iconTransform = imageView->get_rectTransform();
        iconTransform->SetParent(contentTransform, false);
        imageView->set_material(Resources::FindObjectsOfTypeAll<Material*>()->First([] (Material* x) { return x->get_name() == u"UINoGlow"; }));
        imageView->set_sprite(BSML::Lite::ArrayToSprite(Assets::DeleteLevelButtonIcon_png));
        imageView->set_preserveAspect(true);

        float scale = 1.7f;
        iconTransform->set_localScale(UnityEngine::Vector3(scale, scale, scale));

        ContentSizeFitter* contentSizeFitter = deleteLevelButtonGameObject->AddComponent<ContentSizeFitter*>();
        contentSizeFitter->set_verticalFit(ContentSizeFitter::FitMode::Unconstrained);
        contentSizeFitter->set_horizontalFit(ContentSizeFitter::FitMode::Unconstrained);

        deleteLevelButtonGameObject->GetComponent<LayoutElement*>()->set_preferredWidth(10.0f);

        deleteLevelButtonTransform->SetAsFirstSibling();

        deleteLevelButtonGameObject->GetComponent<Button*>()->set_onClick(createDeleteOnClick());
        deleteLevelButtonGameObject->GetComponent<Button*>()->set_interactable(true);
    }
    static Il2CppClass* customPreviewBeatmapLevelClass = classof(CustomPreviewBeatmapLevel*);
    bool customLevel = self->_level && il2cpp_functions::class_is_assignable_from(customPreviewBeatmapLevelClass, il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(self->_level)));
    if(customLevel)
        selectedlevel = reinterpret_cast<CustomPreviewBeatmapLevel*>(self->_level);
    deleteLevelButtonGameObject->SetActive(customLevel);
}

MAKE_HOOK_MATCH(StandardLevelDetailViewController_ShowContent,
                &StandardLevelDetailViewController::ShowContent,
                void, StandardLevelDetailViewController* self, StandardLevelDetailViewController::ContentType contentType, StringW errorText, float downloadingProgress, StringW downloadingText) {
    StandardLevelDetailViewController_ShowContent(self, contentType, errorText, downloadingProgress, downloadingText);
    static Il2CppClass* customPreviewBeatmapLevelClass = classof(CustomPreviewBeatmapLevel*);
    bool customLevel = self->_previewBeatmapLevel && il2cpp_functions::class_is_assignable_from(customPreviewBeatmapLevelClass, il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject*>(self->_previewBeatmapLevel)));
    if(customLevel)
        selectedlevel = reinterpret_cast<CustomPreviewBeatmapLevel*>(self->_previewBeatmapLevel);
    if(contentType == StandardLevelDetailViewController::ContentType::Error) {
        static ConstString deleteLevelButtonName("DeleteLevelButton");
        auto templateButton = self->_loadingControl->_refreshButton;
        auto parent = templateButton->get_transform()->get_parent();
        auto deleteLevelButtonTransform = parent->Find(deleteLevelButtonName);
        GameObject* deleteLevelButtonGameObject = nullptr;
        if(deleteLevelButtonTransform) {
            deleteLevelButtonGameObject = deleteLevelButtonTransform->get_gameObject();
        } else {
            deleteDialogPromptModal = nullptr;

            ContentSizeFitter* parentContentSizeFitter = parent->get_gameObject()->AddComponent<ContentSizeFitter*>();
            parentContentSizeFitter->set_verticalFit(ContentSizeFitter::FitMode::PreferredSize);

            auto layout = BSML::Lite::CreateHorizontalLayoutGroup(parent.unsafePtr());
            GameObject* layoutGameObject = layout->get_gameObject();
            auto layoutTransform = layoutGameObject->get_transform();

            layout->set_spacing(1.2f);
            ContentSizeFitter* layoutContentSizeFitter = layoutGameObject->GetComponent<ContentSizeFitter*>();
            layoutContentSizeFitter->set_horizontalFit(ContentSizeFitter::FitMode::PreferredSize);

            templateButton->get_transform()->SetParent(layoutTransform, false);

            deleteLevelButtonGameObject = Object::Instantiate(templateButton->get_gameObject(), layoutTransform);
            deleteLevelButtonTransform = deleteLevelButtonGameObject->get_transform();
            deleteLevelButtonGameObject->set_name(deleteLevelButtonName);

            static ConstString iconName("Icon");
            auto iconTransform = deleteLevelButtonTransform->Find(iconName);
            auto imageView = iconTransform->GetComponent<ImageView*>();
            imageView->set_material(Resources::FindObjectsOfTypeAll<Material*>()->FirstOrDefault([] (Material* x) { return x->get_name() == u"UINoGlow"; }));
            imageView->set_sprite(BSML::Lite::ArrayToSprite(Assets::DeleteLevelButtonIcon_png));
            imageView->set_preserveAspect(true);

            float scale = 1.7f;
            iconTransform->set_localScale(UnityEngine::Vector3(scale, scale, scale));

            ContentSizeFitter* contentSizeFitter = deleteLevelButtonGameObject->AddComponent<ContentSizeFitter*>();
            contentSizeFitter->set_verticalFit(ContentSizeFitter::FitMode::Unconstrained);
            contentSizeFitter->set_horizontalFit(ContentSizeFitter::FitMode::Unconstrained);

            deleteLevelButtonGameObject->GetComponent<LayoutElement*>()->set_preferredWidth(10.0f);

            deleteLevelButtonTransform->SetAsFirstSibling();

            deleteLevelButtonGameObject->GetComponent<Button*>()->set_onClick(createDeleteOnClick());
        }
    }
}

MAKE_HOOK_MATCH(PlayerDataFileManagerSO_LoadFromCurrentVersion, &PlayerDataFileManagerSO::LoadFromCurrentVersion, PlayerData*, PlayerDataFileManagerSO* self, PlayerSaveData* playerSaveData, BeatmapCharacteristicCollection* beatmapCharacteristicCollection){
    CustomCharacteristics::SetupCustomCharacteristics();
    return PlayerDataFileManagerSO_LoadFromCurrentVersion(self, playerSaveData, beatmapCharacteristicCollection);
}

extern "C" void setup(CModInfo* info) {
    info->id = "SongLoader";
    info->version = VERSION;
    info->version_long = 0;

    auto baseLevelsPath = GetBaseLevelsPath();
    if(!direxists(baseLevelsPath))
        mkpath(baseLevelsPath);
    if(!direxists(baseLevelsPath + CustomLevelsFolder))
        mkpath(baseLevelsPath + CustomLevelsFolder);
    LOG_INFO("Base path is: %s", baseLevelsPath.c_str());
}

extern "C" void late_load() {
    LOG_INFO("Starting SongLoader installation...");

    BSML::Init();
    custom_types::Register::AutoRegister();
    BSML::Register::RegisterSettingsMenu(MOD_ID, DidActivate, false);

    INSTALL_HOOK(getLogger(), SceneManager_Internal_ActiveSceneChanged);
    INSTALL_HOOK(getLogger(), StandardLevelDetailView_RefreshContent);
    INSTALL_HOOK(getLogger(), StandardLevelDetailViewController_ShowContent);
    INSTALL_HOOK(getLogger(), PlayerDataFileManagerSO_LoadFromCurrentVersion);
    INSTALL_HOOK(getLogger(), RichPresenceManager_HandleGameScenesManagerTransitionDidFinish);

    CustomBeatmapLevelLoader::InstallHooks();
    CustomCharacteristics::InstallHooks();
    LoadingFixHooks::InstallHooks();

    CacheUtils::LoadFromFile();
    LOG_INFO("Successfully installed SongLoader!");
}
