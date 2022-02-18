#include "CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"

#include "Paths.hpp"
#include "Utils/FindComponentsUtils.hpp"

#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp" 


using namespace RuntimeSongLoader;
using namespace GlobalNamespace;
using namespace UnityEngine;

DEFINE_TYPE(RuntimeSongLoader, SongLoaderCustomBeatmapLevelPack);

SongLoaderCustomBeatmapLevelPack* SongLoaderCustomBeatmapLevelPack::Make_New(std::string const& packID, std::string_view packName, Sprite* coverImage) {
    return SongLoaderCustomBeatmapLevelPack::New_ctor(StringW(CustomLevelPackPrefixID + packID), StringW(packName), coverImage);
}

void SongLoaderCustomBeatmapLevelPack::ctor(StringW packID, StringW packName, Sprite* coverImage) {
    CustomLevelsCollection = CustomBeatmapLevelCollection::New_ctor(ArrayW<CustomPreviewBeatmapLevel*>());
    auto newCoverImage = coverImage ? coverImage : FindComponentsUtils::GetCustomLevelLoader()->defaultPackCover;
    CustomLevelsPack = CustomBeatmapLevelPack::New_ctor(packID, packName, packName, newCoverImage, newCoverImage, CustomLevelsCollection);
}

void SongLoaderCustomBeatmapLevelPack::SortLevels() {
    auto array = CustomLevelsCollection->customPreviewBeatmapLevels;
    if(!array)
        return;
    if(array.Length() > 0)
        std::sort(array.begin(), array.end(), [](CustomPreviewBeatmapLevel* first, CustomPreviewBeatmapLevel* second) { return first->songName < second->songName; } );
}

ArrayW<GlobalNamespace::CustomPreviewBeatmapLevel*> SongLoaderCustomBeatmapLevelPack::GetCustomPreviewBeatmapLevels() {
    return CustomLevelsCollection->customPreviewBeatmapLevels;
}

void SongLoaderCustomBeatmapLevelPack::SetCustomPreviewBeatmapLevels(ArrayW<GlobalNamespace::CustomPreviewBeatmapLevel*> customPreviewBeatmapLevels) {
    CustomLevelsCollection->customPreviewBeatmapLevels = customPreviewBeatmapLevels;
}

void SongLoaderCustomBeatmapLevelPack::AddTo(SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO, bool addIfEmpty) {
    if(addIfEmpty || CustomLevelsCollection->customPreviewBeatmapLevels.Length() > 0) {
        customBeatmapLevelPackCollectionSO->AddLevelPack(CustomLevelsPack);
    }
}