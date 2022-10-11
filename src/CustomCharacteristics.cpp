
#include "CustomCharacteristics.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "CustomLogger.hpp"

#include "questui/shared/BeatSaberUI.hpp"

#include "Sprites.hpp"

#include "GlobalNamespace/MainSystemInit.hpp"
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

namespace RuntimeSongLoader::CustomCharacteristics {

    List<BeatmapCharacteristicSO*>* characteristicsList = nullptr;

    BeatmapCharacteristicSO* RegisterCustomCharacteristic(Sprite *icon, StringW characteristicName, StringW hintText, StringW serializedName, StringW compoundIdPartName, bool requires360Movement, bool containsRotationEvents, int sortingOrder)
    {
        SafePtrUnity<BeatmapCharacteristicSO> characteristic = ScriptableObject::CreateInstance<BeatmapCharacteristicSO*>();
        characteristic->set_hideFlags(characteristic->get_hideFlags() | UnityEngine::HideFlags::DontUnloadUnusedAsset);
        characteristic->icon = icon;
        characteristic->descriptionLocalizationKey = hintText;
        characteristic->serializedName = serializedName;
        characteristic->characteristicNameLocalizationKey = characteristicName;
        characteristic->compoundIdPartName = compoundIdPartName;
        characteristic->requires360Movement = requires360Movement;
        characteristic->containsRotationEvents = containsRotationEvents;
        characteristic->sortingOrder = sortingOrder;

        static SafePtrUnity<MainSystemInit> mainSystemInit;
        if (!mainSystemInit)
            mainSystemInit = Resources::FindObjectsOfTypeAll<MainSystemInit*>().FirstOrDefault();

        if(!characteristicsList) {
            characteristicsList = List<BeatmapCharacteristicSO*>::New_ctor<il2cpp_utils::CreationType::Manual>();
            if(mainSystemInit) {
                auto beatmapCharacteristics = mainSystemInit->beatmapCharacteristicCollection->beatmapCharacteristics;
                characteristicsList->EnsureCapacity(beatmapCharacteristics.Length());
                for (auto characteristic : beatmapCharacteristics) characteristicsList->Add(characteristic);
            }
        }
        characteristicsList->Add(characteristic.ptr());
        if(mainSystemInit)
            mainSystemInit->beatmapCharacteristicCollection->beatmapCharacteristics = characteristicsList->ToArray();

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

    MAKE_HOOK_MATCH(BeatmapCharacteristicCollectionSO_GetBeatmapCharacteristicBySerializedName, &BeatmapCharacteristicCollectionSO::GetBeatmapCharacteristicBySerializedName, BeatmapCharacteristicSO*, BeatmapCharacteristicCollectionSO* self, StringW serializedName)
    {
        BeatmapCharacteristicSO* result = BeatmapCharacteristicCollectionSO_GetBeatmapCharacteristicBySerializedName(self, serializedName);
        if(!result)
            result = FindByName("MissingCharacteristic");
        return result;
    }

    void InstallHooks() {
        INSTALL_HOOK(getLogger(), BeatmapCharacteristicCollectionSO_GetBeatmapCharacteristicBySerializedName);
    }

    void SetupCustomCharacteristics() {
        static bool created = false;
        if(!created) {
            created = true;

            static SafePtrUnity<BeatmapCharacteristicSO> missingCharacteristic = CustomCharacteristics::RegisterCustomCharacteristic(QuestUI::BeatSaberUI::Base64ToSprite(Sprites::CustomCharacteristics::MissingBase64), "Missing Characteristic", "Missing Characteristic", "MissingCharacteristic", "MissingCharacteristic", false, false, 1000);
            static SafePtrUnity<BeatmapCharacteristicSO> lightshow = CustomCharacteristics::RegisterCustomCharacteristic(QuestUI::BeatSaberUI::Base64ToSprite(Sprites::CustomCharacteristics::LightshowBase64), "Lightshow", "Lightshow", "Lightshow", "Lightshow", false, false, 100);
            static SafePtrUnity<BeatmapCharacteristicSO> lawless = CustomCharacteristics::RegisterCustomCharacteristic(QuestUI::BeatSaberUI::Base64ToSprite(Sprites::CustomCharacteristics::LawlessBase64), "Lawless", "Lawless - Anything Goes", "Lawless", "Lawless", false, false, 101);
        }
    }

}


