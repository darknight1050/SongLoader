#include "LoadingUI.hpp"

#include "UnityEngine/RectOffset.hpp"
#include "HMUI/CurvedCanvasSettings.hpp"
#include "System/Action.hpp"
#include "System/Threading/Thread.hpp"

#include "bsml/shared/BSML-Lite.hpp"
#include "bsml/shared/BSML/Components/Backgroundable.hpp"

#include <chrono>

using namespace BSML;
using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace HMUI;
using namespace TMPro;
using namespace System::Threading;

#define ACTIVE_TIME 6400

namespace RuntimeSongLoader::LoadingUI {

    std::string newText;
    std::string finalText;
    bool needsUpdate = false;

    BSML::ProgressBar* bar = nullptr;
    float barProgress = 0.0f;
    std::chrono::high_resolution_clock::time_point lastActive;
    bool isActive = false;
    bool finished = false;

    void CreateLoadingBar() {
        bar = BSML::Lite::CreateProgressBar({0.0f,3.0f,4.0f}, {0,0,0}, {1.5f,1.5f,1.5f} ,"Loading Songs...", "Loading Songs", "Quest SongLoader");

        /*if(!canvas) {
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
        }*/
    }

    void SetText(std::string_view text) {
        if(bar)
            bar->subText1->set_text(text);
    }

    void UpdateLoadingProgress(int maxFolders, int currentFolder) {
        newText = string_format("%d/%d (%.1f%%)", currentFolder, maxFolders, (float)currentFolder / (float)maxFolders * 100.0f);
        barProgress = (float)currentFolder / (float)maxFolders;
        needsUpdate = true;
    }

    void UpdateLoadedProgress(int levelsCount, int time) {
        newText = string_format("Time Taken: %.1fs", (float)time / 1000.0f);
        finalText = string_format("Loaded %d Songs", levelsCount);
        needsUpdate = true;
        finished = true;
    }

    void SetActive(bool active) {
        isActive = active;
        if(active)
            lastActive = std::chrono::high_resolution_clock::now();
        if(bar)
            bar->get_gameObject()->SetActive(active);
    }

    void UpdateState() {
        if(!bar) {
            LoadingUI::CreateLoadingBar();
            return;
        }

        if(needsUpdate) {
            needsUpdate = false;
            SetActive(true);
            SetText(newText);
            bar->SetProgress(barProgress);
            if(finished){
                bar->loadingBar->get_gameObject()->set_active(false);
                bar->loadingBackground->get_gameObject()->set_active(false);
                bar->headerText->SetText(finalText, false);
                finished = false;
            }else if(!bar->loadingBar->get_gameObject()->get_active()){
                bar->loadingBar->get_gameObject()->set_active(true);
                bar->loadingBackground->get_gameObject()->set_active(true);
                bar->headerText->SetText("Loading Songs...", false);
            }
        }

        if(!isActive)
            return;

        std::chrono::milliseconds delay = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - lastActive);
        if(delay.count() > ACTIVE_TIME){
            SetActive(false);
        }
    }

}