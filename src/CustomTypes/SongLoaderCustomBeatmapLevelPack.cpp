#include "CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"

#include "Paths.hpp"
#include "Utils/FindComponentsUtils.hpp"

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
    CustomLevelsCollection = ArrayW<BeatmapLevel*>();
    auto newCoverImage = coverImage;
    CustomLevelsPack = BeatmapLevelPack::New_ctor(packID, packName, packName, newCoverImage, newCoverImage, CustomLevelsCollection, GlobalNamespace::PlayerSensitivityFlag::Safe);
}

void SongLoaderCustomBeatmapLevelPack::SortLevels() {
    auto array = CustomLevelsCollection;
    if(!array)
        return;
    if(array.size() > 0)
        std::sort(array->begin(), array->end(), [](BeatmapLevel* first, BeatmapLevel* second) { return first->songName < second->songName; } );
}

ArrayW<BeatmapLevel*> SongLoaderCustomBeatmapLevelPack::GetCustomBeatmapLevels() {
    return CustomLevelsCollection;
}

void SongLoaderCustomBeatmapLevelPack::SetCustomBeatmapLevels(ArrayW<BeatmapLevel*> customBeatmapLevels) {
    CustomLevelsCollection = customBeatmapLevels;
}

void SongLoaderCustomBeatmapLevelPack::AddTo(SongLoaderBeatmapLevelsRepository* customBeatmapLevelsRepository, bool addIfEmpty) {
    if(addIfEmpty || CustomLevelsCollection.size() > 0) {
        customBeatmapLevelsRepository->AddLevelPack(CustomLevelsPack);
    }
}
