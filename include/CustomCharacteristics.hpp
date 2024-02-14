#pragma once
#include <string>
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollectionSO.hpp"
#include "UnityEngine/Sprite.hpp"

namespace RuntimeSongLoader::CustomCharacteristics {

    void RegisterCharacteristics(GlobalNamespace::BeatmapCharacteristicCollectionSO* collection, std::span<GlobalNamespace::BeatmapCharacteristicSO*> characteristics);
    GlobalNamespace::BeatmapCharacteristicSO* CreateCharacteristic(UnityEngine::Sprite* icon, StringW characteristicName, StringW hintText, StringW serializedName, StringW compoundIdPartName, bool requires360Movement, bool containsRotationEvents, int sortingOrder);

    void InstallHooks();
}
