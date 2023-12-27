// Implementation by https://github.com/StackDoubleFlow
#include "CustomTypes/CustomLevelInfoSaveData.hpp"

using namespace GlobalNamespace;
using namespace CustomJSONData;

DEFINE_TYPE(CustomJSONData, CustomLevelInfoSaveData);

void CustomLevelInfoSaveData::ctor(
	StringW songName,
	StringW songSubName,
	StringW songAuthorName,
	StringW levelAuthorName,
	float beatsPerMinute,
	float songTimeOffset,
	float shuffle,
	float shufflePeriod,
	float previewStartTime,
	float previewDuration,
	StringW songFilename,
	StringW coverImageFilename,
	StringW environmentName,
	StringW allDirectionsEnvironmentName,
	ArrayW<::StringW> environmentNames,
	ArrayW<GlobalNamespace::BeatmapLevelColorSchemeSaveData*> colorSchemes,
	ArrayW<GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmapSet*> difficultyBeatmapSets
) {
	INVOKE_CTOR();

	_ctor(
		songName,
		songSubName,
		songAuthorName,
		levelAuthorName,
		beatsPerMinute,
		songTimeOffset,
		shuffle,
		shufflePeriod,
		previewStartTime,
		previewDuration,
		songFilename,
		coverImageFilename,
		environmentName,
		allDirectionsEnvironmentName,
		environmentNames,
		colorSchemes,
		difficultyBeatmapSets
	);
}

DEFINE_TYPE(CustomJSONData, CustomDifficultyBeatmap);

void CustomDifficultyBeatmap::ctor(
	StringW difficultyName,
	int difficultyRank,
	StringW beatmapFilename,
	float noteJumpMovementSpeed,
	float noteJumpStartBeatOffset,
	int beatmapColorSchemeIdx,
	int environmentNameIdx
) {
	INVOKE_CTOR();

	_ctor(
		difficultyName,
		difficultyRank,
		beatmapFilename,
		noteJumpMovementSpeed,
		noteJumpStartBeatOffset,
		beatmapColorSchemeIdx,
		environmentNameIdx
	);
}
