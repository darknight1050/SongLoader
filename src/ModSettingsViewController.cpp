#include "ModSettingsViewController.hpp"

#include "HMUI/Touchable.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "CustomTypes/SongLoader.hpp"
#include "API.hpp"
#include "Utils/CacheUtils.hpp"

using namespace QuestUI;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace HMUI;
using namespace RuntimeSongLoader;

void DidActivate(ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if(firstActivation) {
        self->get_gameObject()->AddComponent<Touchable*>();

        GameObject* container = BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());
        Transform* parent = container->get_transform();

        BeatSaberUI::CreateUIButton(parent, "Reload New Songs", [] { 
                API::RefreshSongs(false); 
            }
        );
        BeatSaberUI::CreateUIButton(parent, "Reload All Songs", [] { 
                API::RefreshSongs(); 
            }
        );
        BeatSaberUI::CreateUIButton(parent, "Clear Cache", [] { 
                CacheUtils::ClearCache(); 
            }
        );

    }
}