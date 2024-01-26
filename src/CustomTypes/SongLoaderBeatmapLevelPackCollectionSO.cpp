#include "CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp"

using namespace RuntimeSongLoader;
using namespace GlobalNamespace;

DEFINE_TYPE(RuntimeSongLoader, SongLoaderBeatmapLevelPackCollectionSO);

void SongLoaderBeatmapLevelPackCollectionSO::ctor() {
    customBeatmapLevelPacks = ListW<GlobalNamespace::CustomBeatmapLevelPack*>::New();
}

SongLoaderBeatmapLevelPackCollectionSO* SongLoaderBeatmapLevelPackCollectionSO::CreateNew(){
    auto newCollection = CreateInstance<SongLoaderBeatmapLevelPackCollectionSO*>();
    newCollection->UpdateArray();
    return newCollection;
}

void SongLoaderBeatmapLevelPackCollectionSO::AddLevelPack(CustomBeatmapLevelPack* pack) {
    if(pack && !customBeatmapLevelPacks->Contains(pack)) {
        customBeatmapLevelPacks->Add(pack);
        UpdateArray();
    }
}

void SongLoaderBeatmapLevelPackCollectionSO::RemoveLevelPack(CustomBeatmapLevelPack* pack) {
    if(pack && customBeatmapLevelPacks->Contains(pack)) {
        customBeatmapLevelPacks->Remove(pack);
        UpdateArray();
    }
}

void SongLoaderBeatmapLevelPackCollectionSO::ClearLevelPacks() {
    customBeatmapLevelPacks->Clear();
    UpdateArray();
}

void SongLoaderBeatmapLevelPackCollectionSO::UpdateArray() {
    _allBeatmapLevelPacks = ArrayW<IBeatmapLevelPack*>(reinterpret_cast<Array<IBeatmapLevelPack*>*>(customBeatmapLevelPacks->ToArray().convert()));
}
