
#include "CustomCharacteristics.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "CustomLogger.hpp"

#include "questui/shared/ArrayUtil.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/WeakPtrGO.hpp"

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
#include "System/Convert.hpp"


using namespace GlobalNamespace;
using namespace UnityEngine;

namespace RuntimeSongLoader::CustomCharacteristics {

    List<BeatmapCharacteristicSO*>* characteristicsList = nullptr;

    BeatmapCharacteristicSO* RegisterCustomCharacteristic(Sprite *icon, std::string_view characteristicName, std::string_view hintText, std::string_view serializedName, std::string_view compoundIdPartName, bool requires360Movement, bool containsRotationEvents, int sortingOrder)
    {
        BeatmapCharacteristicSO* characteristic = ScriptableObject::CreateInstance<BeatmapCharacteristicSO*>();
        characteristic->icon = icon;
        characteristic->descriptionLocalizationKey = il2cpp_utils::newcsstr(hintText);
        characteristic->serializedName = il2cpp_utils::newcsstr(serializedName);
        characteristic->characteristicNameLocalizationKey = il2cpp_utils::newcsstr(characteristicName);
        characteristic->compoundIdPartName = il2cpp_utils::newcsstr(compoundIdPartName);
        characteristic->requires360Movement = requires360Movement;
        characteristic->containsRotationEvents = containsRotationEvents;
        characteristic->sortingOrder = sortingOrder;

        static QuestUI::WeakPtrGO<MainSystemInit> mainSystemInit;
        if (!mainSystemInit)
            mainSystemInit = QuestUI::ArrayUtil::First(Resources::FindObjectsOfTypeAll<MainSystemInit*>());

        if(!characteristicsList) {
            characteristicsList = List<BeatmapCharacteristicSO*>::New_ctor<il2cpp_utils::CreationType::Manual>();
            if(mainSystemInit) {
                auto beatmapCharacteristics = mainSystemInit->beatmapCharacteristicCollection->beatmapCharacteristics;
                for(int i = 0; i < beatmapCharacteristics.Length(); i++){
                    characteristicsList->Add(beatmapCharacteristics[i]);
                }
            }
        }
        characteristicsList->Add(characteristic);
        if(mainSystemInit)
            mainSystemInit->beatmapCharacteristicCollection->beatmapCharacteristics = characteristicsList->ToArray();

        return characteristic;
    }

    GlobalNamespace::BeatmapCharacteristicSO* FindByName(std::string_view characteristicName) {
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

    MAKE_HOOK_MATCH(BeatmapCharacteristicCollectionSO_GetBeatmapCharacteristicBySerializedName,&BeatmapCharacteristicCollectionSO::GetBeatmapCharacteristicBySerializedName, BeatmapCharacteristicSO*, BeatmapCharacteristicCollectionSO* self, StringW serializedName)
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
            
            CustomCharacteristics::RegisterCustomCharacteristic(QuestUI::BeatSaberUI::Base64ToSprite(Sprites::CustomCharacteristics::MissingBase64), "Missing Characteristic", "Missing Characteristic", "MissingCharacteristic", "MissingCharacteristic", false, false, 1000);
            CustomCharacteristics::RegisterCustomCharacteristic(QuestUI::BeatSaberUI::Base64ToSprite(Sprites::CustomCharacteristics::LightshowBase64), "Lightshow", "Lightshow", "Lightshow", "Lightshow", false, false, 100);
            CustomCharacteristics::RegisterCustomCharacteristic(QuestUI::BeatSaberUI::Base64ToSprite(Sprites::CustomCharacteristics::LawlessBase64), "Lawless", "Lawless - Anything Goes", "Lawless", "Lawless", false, false, 101);
        }
    }

}


