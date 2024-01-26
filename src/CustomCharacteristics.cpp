
#include "CustomCharacteristics.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "CustomLogger.hpp"

#include "bsml/shared/BSML-Lite.hpp"

#include "assets.hpp"

#include "GlobalNamespace/MainSystemInit.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollection.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollectionSO.hpp"
#include "UnityEngine/ScriptableObject.hpp"
#include "UnityEngine/Vector4.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/SpriteMeshType.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/TextureFormat.hpp"
#include "UnityEngine/TextureWrapMode.hpp"
#include "UnityEngine/ImageConversion.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/HideFlags.hpp"
#include "System/Convert.hpp"


using namespace GlobalNamespace;
using namespace UnityEngine;

static inline UnityEngine::HideFlags operator |(UnityEngine::HideFlags a, UnityEngine::HideFlags b) {
    return UnityEngine::HideFlags(a.value__ | b.value__);
}

namespace RuntimeSongLoader::CustomCharacteristics {

    ListW<UnityW<BeatmapCharacteristicSO>> characteristicsList = nullptr;

    BeatmapCharacteristicSO* RegisterCustomCharacteristic(Sprite *icon, StringW characteristicName, StringW hintText, StringW serializedName, StringW compoundIdPartName, bool requires360Movement, bool containsRotationEvents, int sortingOrder)
    {
        SafePtrUnity<BeatmapCharacteristicSO> characteristic = ScriptableObject::CreateInstance<BeatmapCharacteristicSO*>();
        characteristic->set_hideFlags(characteristic->get_hideFlags() | UnityEngine::HideFlags::DontUnloadUnusedAsset);
        characteristic->_icon = icon;
        characteristic->_descriptionLocalizationKey = hintText;
        characteristic->_serializedName = serializedName;
        characteristic->_characteristicNameLocalizationKey = characteristicName;
        characteristic->_compoundIdPartName = compoundIdPartName;
        characteristic->_requires360Movement = requires360Movement;
        characteristic->_containsRotationEvents = containsRotationEvents;
        characteristic->_sortingOrder = sortingOrder;

        static SafePtrUnity<MainSystemInit> mainSystemInit;
        if (!mainSystemInit)
            mainSystemInit = Resources::FindObjectsOfTypeAll<MainSystemInit*>()->FirstOrDefault();

        if(!characteristicsList) {
            // manual creation
            characteristicsList = ListW<UnityW<BeatmapCharacteristicSO>>::New<il2cpp_utils::CreationType::Manual>();

            if(mainSystemInit) {
                auto beatmapCharacteristics = mainSystemInit->_beatmapCharacteristicCollection->_beatmapCharacteristics;
                characteristicsList->EnsureCapacity(beatmapCharacteristics.size());
                for (auto characteristic : beatmapCharacteristics) characteristicsList->Add(characteristic);
            }
        }
        characteristicsList->Add(characteristic.ptr());
        if(mainSystemInit)
            mainSystemInit->_beatmapCharacteristicCollection->_beatmapCharacteristics = characteristicsList->ToArray();

        return characteristic.ptr();
    }

    GlobalNamespace::BeatmapCharacteristicSO* FindByName(StringW characteristicName) {
        if(!characteristicsList)
            return nullptr;
        for(int i = 0; i < characteristicsList->get_Count(); i++){
            auto characteristic = characteristicsList->get_Item(i);
            if(characteristic && characteristic->serializedName)
                if(characteristic->serializedName == characteristicName)
                    return characteristic;
        }
        return nullptr;
    }

    MAKE_HOOK_MATCH(BeatmapCharacteristicCollection_GetBeatmapCharacteristicBySerializedName, &BeatmapCharacteristicCollection::GetBeatmapCharacteristicBySerializedName, UnityW<BeatmapCharacteristicSO>, BeatmapCharacteristicCollection* self, StringW serializedName)
    {
        UnityW<BeatmapCharacteristicSO> result = BeatmapCharacteristicCollection_GetBeatmapCharacteristicBySerializedName(self, serializedName);
        if(!result)
            result = FindByName("MissingCharacteristic");
        return result;
    }

    void InstallHooks() {
        INSTALL_HOOK(getLogger(), BeatmapCharacteristicCollection_GetBeatmapCharacteristicBySerializedName);
    }

    void SetupCustomCharacteristics() {
        static bool created = false;
        if(!created) {
            created = true;

            static SafePtrUnity<BeatmapCharacteristicSO> missingCharacteristic = CustomCharacteristics::RegisterCustomCharacteristic(BSML::Lite::ArrayToSprite(Assets::CustomCharacteristics::Missing_png), "Missing Characteristic", "Missing Characteristic", "MissingCharacteristic", "MissingCharacteristic", false, false, 1000);
            static SafePtrUnity<BeatmapCharacteristicSO> lightshow = CustomCharacteristics::RegisterCustomCharacteristic(BSML::Lite::ArrayToSprite(Assets::CustomCharacteristics::Lightshow_png), "Lightshow", "Lightshow", "Lightshow", "Lightshow", false, false, 100);
            static SafePtrUnity<BeatmapCharacteristicSO> lawless = CustomCharacteristics::RegisterCustomCharacteristic(BSML::Lite::ArrayToSprite(Assets::CustomCharacteristics::Lawless_png), "Lawless", "Lawless - Anything Goes", "Lawless", "Lawless", false, false, 101);
        }
    }

}
