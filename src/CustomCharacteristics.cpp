#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "CustomCharacteristics.hpp"

#include "UnityEngine/ScriptableObject.hpp"

namespace CustomCharacteristics {

    std::vector<GlobalNamespace::BeatmapCharacteristicSO*> customCharacteristics;

    // TODO: Make this crash-safe
    GlobalNamespace::BeatmapCharacteristicSO* RegisterCustomCharacteristic(UnityEngine::Sprite* icon, std::string_view characteristicName, std::string_view hintText, std::string_view serializedName, std::string_view compoundIdPartName, bool requires360Movement, bool containsRotationEvents, int sortingOrder)
    {
        // To avoid these being GC'd, we should ensure they are allocated manually.
        // If we can't make it, crash horribly.
        auto* characteristic = CRASH_UNLESS(reinterpret_cast<GlobalNamespace::BeatmapCharacteristicSO*>(il2cpp_utils::createManual(classof(GlobalNamespace::BeatmapCharacteristicSO*))));
        // After creating an SO, we must call the CreateInstance method.
        UnityEngine::ScriptableObject::CreateScriptableObject(characteristic);
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

    GlobalNamespace::BeatmapCharacteristicSO* FindByName(std::string_view characteristicName)
    {
        for(auto* characteristic : customCharacteristics){
            if(to_utf8(csstrtostr(characteristic->serializedName)) == characteristicName)
                return characteristic;
        }
        return nullptr;
    }

}