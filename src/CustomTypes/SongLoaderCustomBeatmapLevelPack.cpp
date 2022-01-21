#include "CustomTypes/SongLoaderCustomBeatmapLevelPack.hpp"

#include "Paths.hpp"
#include "Utils/FindComponentsUtils.hpp"

#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp" 

using namespace RuntimeSongLoader;
using namespace GlobalNamespace;
using namespace UnityEngine;

DEFINE_TYPE(RuntimeSongLoader, SongLoaderCustomBeatmapLevelPack);


SongLoaderCustomBeatmapLevelPack* SongLoaderCustomBeatmapLevelPack::Make_New(std::string const& packID, std::string_view packName, Sprite* coverImage) {
    auto customLevelsPackID = il2cpp_utils::newcsstr(CustomLevelPackPrefixID + packID);
    auto customLevelsPackName = il2cpp_utils::newcsstr(packName);
    return *il2cpp_utils::New<SongLoaderCustomBeatmapLevelPack*>(customLevelsPackID, customLevelsPackName, coverImage);
}

void SongLoaderCustomBeatmapLevelPack::ctor(StringW packID, StringW packName, Sprite* coverImage) {
    CustomLevelsCollection = CustomBeatmapLevelCollection::New_ctor(ArrayW<CustomPreviewBeatmapLevel*>());
    auto newCoverImage = coverImage ? coverImage : FindComponentsUtils::GetCustomLevelLoader()->defaultPackCover;
    CustomLevelsPack = CustomBeatmapLevelPack::New_ctor(packID, packName, packName, newCoverImage, newCoverImage, CustomLevelsCollection);
}

void SongLoaderCustomBeatmapLevelPack::SortLevels() {
    auto array = static_cast<Array<CustomPreviewBeatmapLevel*>*>(CustomLevelsCollection->customPreviewBeatmapLevels);
    if(!array)
        return;
    auto arrayValues = array->values;
    auto length = array->Length();
    if(length > 0)
        std::sort(arrayValues, arrayValues + length, [](CustomPreviewBeatmapLevel* first, CustomPreviewBeatmapLevel* second) { return to_utf8(csstrtostr(first->songName)) < to_utf8(csstrtostr(second->songName)); } );
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