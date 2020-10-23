#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "CustomCharacteristics.hpp"

#include "UnityEngine/ScriptableObject.hpp"

namespace CustomCharacteristics {

    std::vector<GlobalNamespace::BeatmapCharacteristicSO*> customCharacteristics;

    GlobalNamespace::BeatmapCharacteristicSO* RegisterCustomCharacteristic(UnityEngine::Sprite* icon, std::string characteristicName, std::string hintText, std::string serializedName, std::string compoundIdPartName, bool requires360Movement, bool containsRotationEvents, int sortingOrder)
    {
        GlobalNamespace::BeatmapCharacteristicSO* characteristic = UnityEngine::ScriptableObject::CreateInstance<GlobalNamespace::BeatmapCharacteristicSO*>();
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

    GlobalNamespace::BeatmapCharacteristicSO* FindByName(std::string characteristicName)
    {
        for(GlobalNamespace::BeatmapCharacteristicSO* characteristic : customCharacteristics){
            if(to_utf8(csstrtostr(characteristic->serializedName)) == characteristicName)
                return characteristic;
        }
        return nullptr;
    }

}