#pragma once
#include <string_view>
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "UnityEngine/Sprite.hpp"

namespace CustomCharacteristics {
    GlobalNamespace::BeatmapCharacteristicSO* RegisterCustomCharacteristic(UnityEngine::Sprite* icon, std::string_view characteristicName, std::string_view hintText, std::string_view serializedName, std::string_view compoundIdPartName, bool requires360Movement = false, bool containsRotationEvents = false, int sortingOrder = 99);
    GlobalNamespace::BeatmapCharacteristicSO* FindByName(std::string_view characteristicName);
}