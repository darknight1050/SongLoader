#include "ModSettingsViewController.hpp"

#include "bsml/shared/BSML-Lite.hpp"
#include "HMUI/Touchable.hpp"

#include "CustomTypes/SongLoader.hpp"
#include "API.hpp"
#include "Utils/CacheUtils.hpp"

using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace HMUI;
using namespace RuntimeSongLoader;

void DidActivate(ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if(firstActivation) {
        self->gameObject->AddComponent<Touchable*>();

        auto container = BSML::Lite::CreateScrollableSettingsContainer(self->transform);
        auto parent = container->transform;

        BSML::Lite::CreateUIButton(parent, "Reload New Songs", [] {
                API::RefreshSongs(false);
            }
        );
        BSML::Lite::CreateUIButton(parent, "Reload All Songs", [] {
                API::RefreshSongs();
            }
        );
        BSML::Lite::CreateUIButton(parent, "Clear Cache", [] {
                CacheUtils::ClearCache();
            }
        );

    }
}
