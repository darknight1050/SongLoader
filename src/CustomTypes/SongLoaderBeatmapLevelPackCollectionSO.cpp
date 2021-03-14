#include "CustomTypes/SongLoaderBeatmapLevelPackCollectionSO.hpp"

#include "customlogger.hpp"

using namespace RuntimeSongLoader;
using namespace GlobalNamespace;

DEFINE_CLASS(SongLoaderBeatmapLevelPackCollectionSO);

void SongLoaderBeatmapLevelPackCollectionSO::ctor() {
    customBeatmapLevelPacks = List<GlobalNamespace::CustomBeatmapLevelPack*>::New_ctor();
}

SongLoaderBeatmapLevelPackCollectionSO* SongLoaderBeatmapLevelPackCollectionSO::CreateNew(){
    auto newCollection = CreateInstance<SongLoaderBeatmapLevelPackCollectionSO*>();
    newCollection->UpdateArray();
    return newCollection;
}

void SongLoaderBeatmapLevelPackCollectionSO::AddLevelPack(CustomBeatmapLevelPack* pack) {
    if(pack && !customBeatmapLevelPacks->Contains_NEW(pack)) {
        customBeatmapLevelPacks->Add_NEW(pack);
        UpdateArray();
    }
}

void SongLoaderBeatmapLevelPackCollectionSO::RemoveLevelPack(CustomBeatmapLevelPack* pack) {
    if(pack && customBeatmapLevelPacks->Contains_NEW(pack)) {
        customBeatmapLevelPacks->Remove_NEW(pack);
        UpdateArray();
    }
}

void SongLoaderBeatmapLevelPackCollectionSO::UpdateArray() {
    allBeatmapLevelPacks = reinterpret_cast<Array<IBeatmapLevelPack*>*>(customBeatmapLevelPacks->ToArray());
}