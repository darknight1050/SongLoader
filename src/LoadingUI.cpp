#include "LoadingUI.hpp"

#include "GlobalNamespace/HMTask.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "HMUI/CurvedCanvasSettings.hpp"
#include "System/Action.hpp"
#include "System/Threading/Thread.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"

using namespace QuestUI;
using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace HMUI;
using namespace TMPro;
using namespace System::Threading;

namespace LoadingUI {

    
    GameObject* canvas = nullptr;
    TextMeshProUGUI* textObject = nullptr;

    void CreateCanvas() {
        if(!canvas) {
            canvas = BeatSaberUI::CreateCanvas();
            canvas->AddComponent<CurvedCanvasSettings*>()->SetRadius(100.0f);
            RectTransform* transform = canvas->GetComponent<RectTransform*>();
            transform->set_position(UnityEngine::Vector3(0.0f, 3.1f, 3.8f));
            transform->set_eulerAngles(UnityEngine::Vector3(-8.0f, 0.0f, 0.0f));
            VerticalLayoutGroup* layout = BeatSaberUI::CreateVerticalLayoutGroup(transform);
            GameObject* layoutGameObject = layout->get_gameObject();
            layoutGameObject->GetComponent<ContentSizeFitter*>()->set_verticalFit(ContentSizeFitter::FitMode::PreferredSize);
            layoutGameObject->AddComponent<Backgroundable*>()->ApplyBackgroundWithAlpha(il2cpp_utils::createcsstr("round-rect-panel"), 0.98f);
            layout->set_padding(UnityEngine::RectOffset::New_ctor(3, 4, 2, 2));
            textObject = BeatSaberUI::CreateText(layout->get_transform(), "");
            textObject->set_alignment(TextAlignmentOptions::Center);
            textObject->set_fontSize(5.4f);
            Object::DontDestroyOnLoad(canvas);
        }
    }

    void SetText(std::string_view text) {
        if(textObject)
            textObject->set_text(il2cpp_utils::createcsstr(text));
    }

    void UpdateLoadingProgress(int maxFolders, int currentFolder) {
        SetActive(true);
        SetText(string_format("Loading Songs %d/%d (%.1f%%)", currentFolder, maxFolders, (float)currentFolder / (float)maxFolders * 100.0f));
    }

    void UpdateLoadedProgress(int levelsCount, int time) {
        SetText(string_format("Loaded %d Songs in %.1fs", levelsCount, (float)time / 1000.0f));
        HMTask::New_ctor(il2cpp_utils::MakeDelegate<System::Action*>(classof(System::Action*),
            (std::function<void()>)[] {
                Thread::Sleep(6400);
                SetActive(false);
            }
        ), nullptr)->Run();
    }

    void SetActive(bool active) {
        if(canvas)
            canvas->SetActive(active);
    }

}