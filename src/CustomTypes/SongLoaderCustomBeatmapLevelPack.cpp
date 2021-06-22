#include "CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"

#include "Paths.hpp"
#include "Utils/FindComponentsUtils.hpp"

#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp" 

using namespace RuntimeSongLoader;
using namespace GlobalNamespace;
using namespace UnityEngine;

DEFINE_TYPE(RuntimeSongLoader, SongLoaderCustomBeatmapLevelPack);

SongLoaderCustomBeatmapLevelPack* SongLoaderCustomBeatmapLevelPack::New_ctor(std::string packID, std::string packName, Sprite* coverImage) {
    auto customLevelsPackID = il2cpp_utils::newcsstr(CustomLevelPackPrefixID + packID);
    auto customLevelsPackName = il2cpp_utils::newcsstr(packName);
    return *il2cpp_utils::New<SongLoaderCustomBeatmapLevelPack*>(customLevelsPackID, customLevelsPackName, coverImage);
}

void SongLoaderCustomBeatmapLevelPack::ctor(Il2CppString* packID, Il2CppString* packName, Sprite* coverImage) {
    CustomLevelsCollection = CustomBeatmapLevelCollection::New_ctor(Array<CustomPreviewBeatmapLevel*>::NewLength(0));
    CustomLevelsPack = CustomBeatmapLevelPack::New_ctor(packID, packName, packName, coverImage ? coverImage : FindComponentsUtils::GetCustomLevelLoader()->defaultPackCover, CustomLevelsCollection);
}

void SongLoaderCustomBeatmapLevelPack::SortLevels() {
    if(!CustomLevelsCollection->customPreviewBeatmapLevels)
        return;
    auto arrayValues = CustomLevelsCollection->customPreviewBeatmapLevels->values;
    auto length = CustomLevelsCollection->customPreviewBeatmapLevels->Length();
    if(length > 0)
        std::sort(arrayValues, arrayValues + length, [](CustomPreviewBeatmapLevel* first, CustomPreviewBeatmapLevel* second) { return to_utf8(csstrtostr(first->songName)) < to_utf8(csstrtostr(second->songName)); } );
}

Array<GlobalNamespace::CustomPreviewBeatmapLevel*>* SongLoaderCustomBeatmapLevelPack::GetCustomPreviewBeatmapLevels() {
    return CustomLevelsCollection->customPreviewBeatmapLevels;
}

void SongLoaderCustomBeatmapLevelPack::SetCustomPreviewBeatmapLevels(Array<GlobalNamespace::CustomPreviewBeatmapLevel*>* customPreviewBeatmapLevels) {
    CustomLevelsCollection->customPreviewBeatmapLevels = customPreviewBeatmapLevels;
}

void SongLoaderCustomBeatmapLevelPack::AddTo(SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO, bool addIfEmpty) {
    if(addIfEmpty || CustomLevelsCollection->customPreviewBeatmapLevels->Length() > 0) {
        customBeatmapLevelPackCollectionSO->AddLevelPack(CustomLevelsPack);
    }
}