#pragma once
#include <string>
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "UnityEngine/Sprite.hpp"

namespace RuntimeSongLoader::CustomCharacteristics {

    GlobalNamespace::BeatmapCharacteristicSO* RegisterCustomCharacteristic(UnityEngine::Sprite* icon, StringW characteristicName, StringW hintText, StringW serializedName, StringW compoundIdPartName, bool requires360Movement = false, bool containsRotationEvents = false, int sortingOrder = 99);
    
    GlobalNamespace::BeatmapCharacteristicSO* FindByName(StringW characteristicName);

    void InstallHooks();
    
    void SetupCustomCharacteristics();
    
}