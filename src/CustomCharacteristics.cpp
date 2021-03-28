
#include "CustomCharacteristics.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "CustomLogger.hpp"

#include "questui/shared/ArrayUtil.hpp"

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

//From questui: https://github.com/darknight1050/questui
Sprite* Base64ToSprite(std::string& base64)
{
    Array<uint8_t>* bytes = System::Convert::FromBase64String(il2cpp_utils::createcsstr(base64));
    Texture2D* texture = Texture2D::New_ctor(0, 0, TextureFormat::RGBA32, false, false);
    if(ImageConversion::LoadImage(texture, bytes, false)) {
        texture->set_wrapMode(TextureWrapMode::Clamp);
        return Sprite::Create(texture, UnityEngine::Rect(0.0f, 0.0f, (float)texture->get_width(), (float)texture->get_height()), UnityEngine::Vector2(0.5f,0.5f), 1024.0f, 1u, SpriteMeshType::FullRect, UnityEngine::Vector4(0.0f, 0.0f, 0.0f, 0.0f), false);
    }
    return nullptr;
}

namespace RuntimeSongLoader::CustomCharacteristics {

    List<BeatmapCharacteristicSO*>* characteristicsList = nullptr;

    BeatmapCharacteristicSO* RegisterCustomCharacteristic(Sprite* icon, std::string characteristicName, std::string hintText, std::string serializedName, std::string compoundIdPartName, bool requires360Movement, bool containsRotationEvents, int sortingOrder)
    {
        BeatmapCharacteristicSO* characteristic = ScriptableObject::CreateInstance<BeatmapCharacteristicSO*>();
        characteristic->icon = icon;
        characteristic->descriptionLocalizationKey = il2cpp_utils::createcsstr(hintText);
        characteristic->serializedName = il2cpp_utils::createcsstr(serializedName);
        characteristic->characteristicNameLocalizationKey = il2cpp_utils::createcsstr(characteristicName);
        characteristic->compoundIdPartName = il2cpp_utils::createcsstr(compoundIdPartName);
        characteristic->requires360Movement = requires360Movement;
        characteristic->containsRotationEvents = containsRotationEvents;
        characteristic->sortingOrder = sortingOrder;

        auto mainSystemInit = QuestUI::ArrayUtil::First(Resources::FindObjectsOfTypeAll<MainSystemInit*>());
        if(!characteristicsList) {
            characteristicsList = List<BeatmapCharacteristicSO*>::New_ctor<il2cpp_utils::CreationType::Manual>();
            if(mainSystemInit) {
                auto beatmapCharacteristics = mainSystemInit->beatmapCharacteristicCollection->beatmapCharacteristics;
                for(int i = 0; i < beatmapCharacteristics->Length(); i++){
                    characteristicsList->Add(beatmapCharacteristics->values[i]);
                }
            }
        }
        characteristicsList->Add(characteristic);
        if(mainSystemInit)
            mainSystemInit->beatmapCharacteristicCollection->beatmapCharacteristics = characteristicsList->ToArray();

        return characteristic;
    }

    GlobalNamespace::BeatmapCharacteristicSO* FindByName(const std::string& characteristicName) {
        if(!characteristicsList)
            return nullptr;
        for(int i = 0; i < characteristicsList->get_Count(); i++){
            auto characteristic = characteristicsList->get_Item(i);
            if(characteristic && characteristic->serializedName)
                if(to_utf8(csstrtostr(characteristic->serializedName)) == characteristicName)
                    return characteristic;
        }
        return nullptr;
    }  

    MAKE_HOOK_OFFSETLESS(BeatmapCharacteristicCollectionSO_GetBeatmapCharacteristicBySerializedName, BeatmapCharacteristicSO*, BeatmapCharacteristicCollectionSO* self, Il2CppString* serializedName)
    {
        BeatmapCharacteristicSO* result = BeatmapCharacteristicCollectionSO_GetBeatmapCharacteristicBySerializedName(self, serializedName);
        if(!result)
            result = FindByName("MissingCharacteristic");
        return result;
    }

    void InstallHooks() {
        INSTALL_HOOK_OFFSETLESS(getLogger(), BeatmapCharacteristicCollectionSO_GetBeatmapCharacteristicBySerializedName, il2cpp_utils::FindMethodUnsafe("", "BeatmapCharacteristicCollectionSO", "GetBeatmapCharacteristicBySerializedName", 1));
    }

    void SetupCustomCharacteristics() {
        static bool created = false;
        if(!created) {
            created = true;
            CustomCharacteristics::RegisterCustomCharacteristic(Base64ToSprite(Sprites::MissingBase64), "Missing Characteristic", "Missing Characteristic", "MissingCharacteristic", "MissingCharacteristic", false, false, 1000);
            CustomCharacteristics::RegisterCustomCharacteristic(Base64ToSprite(Sprites::LightshowBase64), "Lightshow", "Lightshow", "Lightshow", "Lightshow", false, false, 100);
            CustomCharacteristics::RegisterCustomCharacteristic(Base64ToSprite(Sprites::LawlessBase64), "Lawless", "Lawless - Anything Goes", "Lawless", "Lawless", false, false, 101);
        }
    }

}


