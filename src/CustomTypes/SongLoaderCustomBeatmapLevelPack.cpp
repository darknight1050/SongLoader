#include "CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"

#include "Paths.hpp"
#include "Utils/FindComponentsUtils.hpp"

#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp" 

using namespace RuntimeSongLoader;
using namespace GlobalNamespace;
using namespace UnityEngine;

DEFINE_TYPE(RuntimeSongLoader, SongLoaderCustomBeatmapLevelPack);

template <class T>
constexpr ArrayW<T> listToArrayW(::System::Collections::Generic::IReadOnlyList_1<T>* list) {
    return ArrayW<T>(reinterpret_cast<Array<T>*>(list));
}

SongLoaderCustomBeatmapLevelPack* SongLoaderCustomBeatmapLevelPack::Make_New(std::string const& packID, std::string_view packName, Sprite* coverImage) {
    return SongLoaderCustomBeatmapLevelPack::New_ctor(StringW(CustomLevelPackPrefixID + packID), StringW(packName), coverImage);
}

void SongLoaderCustomBeatmapLevelPack::ctor(StringW packID, StringW packName, Sprite* coverImage) {
    CustomLevelsCollection = CustomBeatmapLevelCollection::New_ctor(ArrayW<CustomPreviewBeatmapLevel*>());
    auto newCoverImage = coverImage ? coverImage : FindComponentsUtils::GetCustomLevelLoader()->defaultPackCover;
    CustomLevelsPack = CustomBeatmapLevelPack::New_ctor(packID, packName, packName, newCoverImage, newCoverImage, CustomLevelsCollection);
}

void SongLoaderCustomBeatmapLevelPack::SortLevels() {
    auto array = listToArrayW<CustomPreviewBeatmapLevel*>(CustomLevelsCollection->customPreviewBeatmapLevels);
    if(!array)
        return;
    if(array.Length() > 0)
        std::sort(array.begin(), array.end(), [](CustomPreviewBeatmapLevel* first, CustomPreviewBeatmapLevel* second) { return first->songName < second->songName; } );
}

ArrayW<CustomPreviewBeatmapLevel*> SongLoaderCustomBeatmapLevelPack::GetCustomPreviewBeatmapLevels() {
    return listToArrayW<CustomPreviewBeatmapLevel*>(CustomLevelsCollection->customPreviewBeatmapLevels);
}

void SongLoaderCustomBeatmapLevelPack::SetCustomPreviewBeatmapLevels(ArrayW<CustomPreviewBeatmapLevel*> customPreviewBeatmapLevels) {
    CustomLevelsCollection->customPreviewBeatmapLevels = reinterpret_cast<::System::Collections::Generic::IReadOnlyList_1<CustomPreviewBeatmapLevel*>*>(customPreviewBeatmapLevels.convert());
}

void SongLoaderCustomBeatmapLevelPack::AddTo(SongLoaderBeatmapLevelPackCollectionSO* customBeatmapLevelPackCollectionSO, bool addIfEmpty) {
    if(addIfEmpty || listToArrayW<CustomPreviewBeatmapLevel*>(CustomLevelsCollection->customPreviewBeatmapLevels).Length() > 0) {
        customBeatmapLevelPackCollectionSO->AddLevelPack(CustomLevelsPack);
    }
}