#include "SongData.hpp"

namespace SongData {
    SongInfo::SongInfo(std::string_view source) : valid(true) {
        sourceDocument.Parse(source.data());
        if (sourceDocument.HasParseError()) {
            return;
        }
        auto itr = sourceDocument.FindMember("_customData");
        if (itr != sourceDocument.MemberEnd()) {
            customData.emplace(CustomInfoData(itr->value));
        }
        
        version.assign(sourceDocument["_version"].GetString());
        songName.assign(sourceDocument["_songName"].GetString());
        songSubName.assign(sourceDocument["_songSubName"].GetString());
        songAuthorName.assign(sourceDocument["_songAuthorName"].GetString());
        levelAuthorName.assign(sourceDocument["_levelAuthorName"].GetString());
        beatsPerMinute = sourceDocument["_beatsPerMinute"].GetFloat();
        songTimeOffset = sourceDocument["_songTimeOffset"].GetFloat();
        shuffle = sourceDocument["_shuffle"].GetFloat();
        shufflePeriod = sourceDocument["_shufflePeriod"].GetFloat();
        previewStartTime = sourceDocument["_previewStartTime"].GetFloat();
        previewDuration = sourceDocument["_previewDuration"].GetFloat();
        songFilename.assign(sourceDocument["_songFilename"].GetString());
        coverImageFilename.assign(sourceDocument["_coverImageFilename"].GetString());
        environmentName.assign(sourceDocument["_environmentName"].GetString());
        allDirectionsEnvironmentName.assign(sourceDocument["_allDirectionsEnvironmentName"].GetString());
    
        const rapidjson::Value& _difficultyBeatmapSets = sourceDocument["_difficultyBeatmapSets"];
        for (rapidjson::SizeType i = 0; i < _difficultyBeatmapSets.Size(); i++) {
            difficultyBeatmapSets.emplace_back(_difficultyBeatmapSets[i]);
        }
    }

    DifficultyBeatmapSet::DifficultyBeatmapSet(rapidjson::Value& source_) : source(source_) {
        characteristicName.assign(source["_beatmapCharacteristicName"].GetString());

        const rapidjson::Value& _difficultyBeatmaps = source["_difficultyBeatmaps"];
        for (rapidjson::SizeType i = 0; i < _difficultyBeatmaps.Size(); i++) {
            difficultyBeatmaps.emplace_back(_difficultyBeatmaps[i]);
        }
    }

    DifficultyBeatmap::DifficultyBeatmap(rapidjson::Value& source_) : source(source_) {
        auto itr = source.FindMember("_customData");
        if (itr != source.MemberEnd()) {
            customData.emplace(CustomDifficultyData(itr->value));
        }

        // It's easy and fast to decode the type of the difficulty using a hashmap
        static std::unordered_map<std::string, BeatmapDifficulty> const difficulties = {
            { "Easy", BeatmapDifficulty::Easy }, 
            { "Normal", BeatmapDifficulty::Normal }, 
            { "Hard", BeatmapDifficulty::Hard }, 
            { "Expert", BeatmapDifficulty::Expert }, 
            { "ExpertPlus", BeatmapDifficulty::ExpertPlus }
        };

        auto difficultyItr = difficulties.find(std::string(source["_difficulty"].GetString()));
        if (difficultyItr != difficulties.end()) {
            difficulty = difficultyItr->second;
        } else {
            // TODO: Do something if the difficulty name is invalid
        }

        difficultyRank = source["_difficultyRank"].GetFloat();
        beatmapFilename.assign(source["_beatmapFilename"].GetString());
        noteJumpMovementSpeed = source["_noteJumpMovementSpeed"].GetFloat();
        noteJumpStartBeatOffset = source["_noteJumpStartBeatOffset"].GetFloat();
    }
}