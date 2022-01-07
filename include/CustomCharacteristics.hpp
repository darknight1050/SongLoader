#pragma once
#include <string>
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "UnityEngine/Sprite.hpp"

namespace RuntimeSongLoader::CustomCharacteristics {

    GlobalNamespace::BeatmapCharacteristicSO* RegisterCustomCharacteristic(UnityEngine::Sprite* icon, const std::string& characteristicName, const std::string& hintText, const std::string& serializedName, const std::string& compoundIdPartName, bool requires360Movement = false, bool containsRotationEvents = false, int sortingOrder = 99);
    
    GlobalNamespace::BeatmapCharacteristicSO* FindByName(const std::string& characteristicName);

    void InstallHooks();
    
    void SetupCustomCharacteristics();
    
}