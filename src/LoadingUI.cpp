#include "LoadingUI.hpp"

#include "GlobalNamespace/HMTask.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "HMUI/CurvedCanvasSettings.hpp"
#include "System/Action.hpp"
#include "System/Threading/Thread.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"

#include <chrono>

using namespace QuestUI;
using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace HMUI;
using namespace TMPro;
using namespace System::Threading;

#define ACTIVE_TIME 6400

namespace RuntimeSongLoader::LoadingUI {

    std::string newText;
    bool needsUpdate = false;

    GameObject* canvas = nullptr;
    TextMeshProUGUI* textObject = nullptr;
    
    std::chrono::high_resolution_clock::time_point lastActive;
    bool isActive = false;

    void CreateCanvas() {
        if(!canvas) {
            canvas = BeatSaberUI::CreateCanvas();
            canvas->AddComponent<CurvedCanvasSettings*>()->SetRadius(100.0f);
            RectTransform* transform = canvas->GetComponent<RectTransform*>();
            transform->set_position(UnityEngine::Vector3(0.0f, 3.2f, 4.2f));
            transform->set_eulerAngles(UnityEngine::Vector3(-8.0f, 0.0f, 0.0f));
            VerticalLayoutGroup* layout = BeatSaberUI::CreateVerticalLayoutGroup(transform);
            GameObject* layoutGameObject = layout->get_gameObject();
            layoutGameObject->GetComponent<ContentSizeFitter*>()->set_verticalFit(ContentSizeFitter::FitMode::PreferredSize);
            static ConstString bgName("round-rect-panel");
            layoutGameObject->AddComponent<Backgroundable*>()->ApplyBackgroundWithAlpha(bgName, 0.96f);
            layout->set_padding(UnityEngine::RectOffset::New_ctor(3, 4, 2, 2));
            textObject = BeatSaberUI::CreateText(layout->get_transform(), "");
            textObject->set_alignment(TextAlignmentOptions::Center);
            textObject->set_fontSize(5.4f);
            Object::DontDestroyOnLoad(canvas);
        }
    }

    void SetText(std::string_view text) {
        if(textObject)
            textObject->set_text(text);
    }

    void UpdateLoadingProgress(int maxFolders, int currentFolder) {
        newText = string_format("Loading Songs %d/%d (%.1f%%)", currentFolder, maxFolders, (float)currentFolder / (float)maxFolders * 100.0f);
        needsUpdate = true;
    }

    void UpdateLoadedProgress(int levelsCount, int time) {
        newText = string_format("Loaded %d Songs in %.1fs", levelsCount, (float)time / 1000.0f);
        needsUpdate = true;
    }

    void SetActive(bool active) {
        isActive = active;
        if(active)
            lastActive = std::chrono::high_resolution_clock::now();
        if(canvas)
            canvas->SetActive(active);
    }

    void UpdateState() {
        if(!canvas) {
            LoadingUI::CreateCanvas();
            return;
        }

        if(needsUpdate) {
            needsUpdate = false;
            SetActive(true);
            SetText(newText);
        }

        if(!isActive)
            return;

        std::chrono::milliseconds delay = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - lastActive);
        if(delay.count() > ACTIVE_TIME)
            SetActive(false);
    }

}