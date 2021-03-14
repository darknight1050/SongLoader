
#include "CustomCharacteristics.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "CustomLogger.hpp"

#include "GlobalNamespace/BeatmapCharacteristicCollectionSO.hpp"
#include "UnityEngine/ScriptableObject.hpp"

using namespace GlobalNamespace;
using namespace UnityEngine;

namespace CustomCharacteristics {

    std::vector<BeatmapCharacteristicSO*> customCharacteristics;

    BeatmapCharacteristicSO* RegisterCustomCharacteristic(Sprite* icon, std::string characteristicName, std::string hintText, std::string serializedName, std::string compoundIdPartName, bool requires360Movement, bool containsRotationEvents, int sortingOrder)
    {
        BeatmapCharacteristicSO* characteristic = ScriptableObject::CreateInstance<BeatmapCharacteristicSO*>();
        characteristic->icon = icon;
        characteristic->descriptionLocalizationKey = il2cpp_utils::createcsstr(hintText);
        characteristic->serializedName = il2cpp_utils::createcsstr(serializedName);
        characteristic->characteristicNameLocalizationKey = il2cpp_utils::createcsstr(compoundIdPartName);
        characteristic->requires360Movement = requires360Movement;
        characteristic->containsRotationEvents = containsRotationEvents;
        characteristic->sortingOrder = sortingOrder;
        
        customCharacteristics.push_back(characteristic);
        return characteristic;
    }

    BeatmapCharacteristicSO* FindByName(std::string characteristicName)
    {
        for(BeatmapCharacteristicSO* characteristic : customCharacteristics){
            if(to_utf8(csstrtostr(characteristic->serializedName)) == characteristicName)
                return characteristic;
        }
        return nullptr;
    }

    MAKE_HOOK_OFFSETLESS(BeatmapCharacteristicCollectionSO_GetBeatmapCharacteristicBySerializedName, BeatmapCharacteristicSO*, BeatmapCharacteristicCollectionSO* self, Il2CppString* serializedName)
    {
        BeatmapCharacteristicSO* result = BeatmapCharacteristicCollectionSO_GetBeatmapCharacteristicBySerializedName(self, serializedName);
        std::string _serializedName = to_utf8(csstrtostr(serializedName));
        if(!result)
        {   
            result = FindByName(_serializedName);
            if(!result)
                result = FindByName("MissingCharacteristic");
        }

        return result;
    }

    void InstallHooks() {
        INSTALL_HOOK_OFFSETLESS(getLogger(), BeatmapCharacteristicCollectionSO_GetBeatmapCharacteristicBySerializedName, il2cpp_utils::FindMethodUnsafe("", "BeatmapCharacteristicCollectionSO", "GetBeatmapCharacteristicBySerializedName", 1));
    }

}


