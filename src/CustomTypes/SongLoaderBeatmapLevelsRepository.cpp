#include "CustomTypes/SongLoaderBeatmapLevelsRepository.hpp"

using namespace RuntimeSongLoader;
using namespace GlobalNamespace;

DEFINE_TYPE(RuntimeSongLoader, SongLoaderBeatmapLevelsRepository);

void SongLoaderBeatmapLevelsRepository::ctor() {
    customBeatmapLevelPacks = ListW<GlobalNamespace::BeatmapLevelPack*>::New();
}

SongLoaderBeatmapLevelsRepository* SongLoaderBeatmapLevelsRepository::CreateNew(){
    auto newCollection = SongLoaderBeatmapLevelsRepository::New_ctor();
    newCollection->UpdateArray();
    return newCollection;
}

void SongLoaderBeatmapLevelsRepository::AddLevelPack(BeatmapLevelPack* pack) {
    if(pack && !customBeatmapLevelPacks->Contains(pack)) {
        customBeatmapLevelPacks->Add(pack);
        UpdateArray();
    }
}

void SongLoaderBeatmapLevelsRepository::RemoveLevelPack(BeatmapLevelPack* pack) {
    if(pack && customBeatmapLevelPacks->Contains(pack)) {
        customBeatmapLevelPacks->Remove(pack);
        UpdateArray();
    }
}

void SongLoaderBeatmapLevelsRepository::ClearLevelPacks() {
    customBeatmapLevelPacks->Clear();
    UpdateArray();
}

void SongLoaderBeatmapLevelsRepository::UpdateArray() {
    // _beatmapLevelPacks = ListW<IBeatmapLevelPack*>(reinterpret_cast<Array<BeatmapLevelPack*>*>(customBeatmapLevelPacks->ToArray().convert()));
    _beatmapLevelPacks = customBeatmapLevelPacks->i___System__Collections__Generic__IReadOnlyList_1_T_();
}
