// Implementation by https://github.com/StackDoubleFlow
#pragma once

#include "custom-types/shared/macros.hpp"
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/document.h"

#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmapSet.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmap.hpp"

namespace CustomJSONData {
	using ValueUTF16 = rapidjson::GenericValue<rapidjson::UTF16<char16_t>>;
	using DocumentUTF16 = rapidjson::GenericDocument<rapidjson::UTF16<char16_t>>;
}

DECLARE_CLASS_CODEGEN(CustomJSONData, CustomLevelInfoSaveData, 
					  GlobalNamespace::StandardLevelInfoSaveData,
	
	DECLARE_CTOR(ctor, StringW songName, StringW songSubName, 
				 StringW songAuthorName, StringW levelAuthorNeame, float beatsPerMinute,
				 float songTimeOffset, float shuffle, float shufflePeriod, float previewStartTime, 
				 float previewDuration, StringW songFilename, StringW coverImageFilename, 
				 StringW environmentName, StringW allDirectionsEnvironmentName, 
				 ::ArrayW<GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmapSet*> difficultyBeatmapSets);

	DECLARE_SIMPLE_DTOR();

public:
	std::shared_ptr<DocumentUTF16> doc;
	std::optional< std::reference_wrapper<ValueUTF16>> customData;
)

DECLARE_CLASS_CODEGEN(CustomJSONData, CustomDifficultyBeatmap, 
					  GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmap,
	
	DECLARE_CTOR(ctor, StringW difficultyName, int difficultyRank, StringW beatmapFilename, float noteJumpMovementSpeed, float noteJumpStartBeatOffset);

public:
	std::optional<std::reference_wrapper<ValueUTF16>> customData;
)