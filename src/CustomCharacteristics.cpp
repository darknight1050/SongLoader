
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
    SafePtr<System::Collections::Generic::List_1<UnityW<BeatmapCharacteristicSO>>> characteristicsList = nullptr;

    void RegisterCharacteristics(BeatmapCharacteristicCollectionSO* collection, std::span<BeatmapCharacteristicSO*> characteristics) {
        if(!characteristicsList) {
            // manual creation
            characteristicsList.emplace(ListW<UnityW<BeatmapCharacteristicSO>>::New());

            auto originalCharacteristics = collection->_beatmapCharacteristics;
            characteristicsList->EnsureCapacity(originalCharacteristics.size() + characteristics.size());

            for (auto characteristic : originalCharacteristics)
                characteristicsList->Add(characteristic);

            for (auto characteristic : characteristics)
                characteristicsList->Add(characteristic);
        }

        collection->_beatmapCharacteristics = characteristicsList->ToArray();
    }

    BeatmapCharacteristicSO* CreateCharacteristic(Sprite* icon, StringW characteristicName, StringW hintText, StringW serializedName, StringW compoundIdPartName, bool requires360Movement, bool containsRotationEvents, int sortingOrder) {
        icon->texture->wrapMode = UnityEngine::TextureWrapMode::Clamp;

        auto characteristic = ScriptableObject::CreateInstance<BeatmapCharacteristicSO*>();
        characteristic->hideFlags = characteristic->hideFlags | UnityEngine::HideFlags::DontUnloadUnusedAsset;
        characteristic->_icon = icon;
        characteristic->_descriptionLocalizationKey = hintText;
        characteristic->_serializedName = serializedName;
        characteristic->_characteristicNameLocalizationKey = characteristicName;
        characteristic->_compoundIdPartName = compoundIdPartName;
        characteristic->_requires360Movement = requires360Movement;
        characteristic->_containsRotationEvents = containsRotationEvents;
        characteristic->_sortingOrder = sortingOrder;

        return characteristic;
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

    static bool registeredCharacteristics = false;
    MAKE_HOOK_MATCH(MainSystemInit_InstallBindings, &MainSystemInit::InstallBindings, void, MainSystemInit* self, ::Zenject::DiContainer * container, bool isRunningFromTests) {
        if (!registeredCharacteristics) {
            registeredCharacteristics = true;
            static SafePtrUnity<BeatmapCharacteristicSO> missingCharacteristic = CustomCharacteristics::CreateCharacteristic(
                BSML::Lite::ArrayToSprite(Assets::CustomCharacteristics::Missing_png),
                "Missing Characteristic",
                "Missing Characteristic",
                "MissingCharacteristic",
                "MissingCharacteristic",
                false,
                false,
                1000
            );

            static SafePtrUnity<BeatmapCharacteristicSO> lightshow = CustomCharacteristics::CreateCharacteristic(
                BSML::Lite::ArrayToSprite(Assets::CustomCharacteristics::Lightshow_png),
                "Lightshow",
                "Lightshow",
                "Lightshow",
                "Lightshow",
                false,
                false,
                100
            );

            static SafePtrUnity<BeatmapCharacteristicSO> lawless = CustomCharacteristics::CreateCharacteristic(
                BSML::Lite::ArrayToSprite(Assets::CustomCharacteristics::Lawless_png),
                "Lawless",
                "Lawless - Anything Goes",
                "Lawless",
                "Lawless",
                false,
                false,
                101
            );

            BeatmapCharacteristicSO* characteristics[3] = {
                missingCharacteristic.ptr(),
                lightshow.ptr(),
                lawless.ptr()
            };

            RegisterCharacteristics(self->_beatmapCharacteristicCollection, std::span<BeatmapCharacteristicSO*>(characteristics));
        }

        MainSystemInit_InstallBindings(self, container, isRunningFromTests);
    }

    MAKE_HOOK_MATCH(BeatmapCharacteristicCollection_GetBeatmapCharacteristicBySerializedName, &BeatmapCharacteristicCollection::GetBeatmapCharacteristicBySerializedName, UnityW<BeatmapCharacteristicSO>, BeatmapCharacteristicCollection* self, StringW serializedName) {
        UnityW<BeatmapCharacteristicSO> result = BeatmapCharacteristicCollection_GetBeatmapCharacteristicBySerializedName(self, serializedName);
        if(!result) {
            std::string cppChar(serializedName);
            LOG_WARN("Could not find characteristic with identifier %s, Attempting to find it manually", cppChar.c_str());
            result = FindByName(serializedName);

            if (!result) {
                LOG_WARN("STILL couldn't find characteristic, returning missing characteristic");
                result = FindByName("MissingCharacteristic");
            }
        }
        return result;
    }

    void InstallHooks() {
        INSTALL_HOOK(getLogger(), BeatmapCharacteristicCollection_GetBeatmapCharacteristicBySerializedName);
        INSTALL_HOOK(getLogger(), MainSystemInit_InstallBindings);
    }
}
