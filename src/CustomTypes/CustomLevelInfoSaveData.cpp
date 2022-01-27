// Implementation by https://github.com/StackDoubleFlow
#include "CustomTypes/CustomLevelInfoSaveData.hpp"

using namespace GlobalNamespace;
using namespace CustomJSONData;

DEFINE_TYPE(CustomJSONData, CustomLevelInfoSaveData);

void CustomLevelInfoSaveData::ctor(StringW songName, StringW songSubName, 
				 StringW songAuthorName, StringW levelAuthorName, float beatsPerMinute,
				 float songTimeOffset, float shuffle, float shufflePeriod, float previewStartTime, 
				 float previewDuration, StringW songFilename, StringW coverImageFilename, 
				 StringW environmentName, StringW allDirectionsEnvironmentName,
				 ArrayW<GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmapSet*> difficultyBeatmapSets) {
	INVOKE_CTOR();
	static auto* ctor = il2cpp_utils::FindMethodUnsafe("", "StandardLevelInfoSaveData", ".ctor", 15);
	CRASH_UNLESS(il2cpp_utils::RunMethod(this, ctor, songName, songSubName, songAuthorName, levelAuthorName, 
										 beatsPerMinute, songTimeOffset, shuffle, shufflePeriod, previewStartTime, 
										 previewDuration, songFilename, coverImageFilename, environmentName, allDirectionsEnvironmentName,
										 difficultyBeatmapSets));
}

DEFINE_TYPE(CustomJSONData, CustomDifficultyBeatmap);

void CustomDifficultyBeatmap::ctor(StringW difficultyName, int difficultyRank, StringW beatmapFilename, 
								   float noteJumpMovementSpeed, float noteJumpStartBeatOffset) {
	INVOKE_CTOR();
	this->difficulty = difficultyName;
	this->difficultyRank = difficultyRank;
	this->beatmapFilename = beatmapFilename;
	this->noteJumpMovementSpeed = noteJumpMovementSpeed;
	this->noteJumpStartBeatOffset = noteJumpStartBeatOffset;
}
