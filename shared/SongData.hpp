#pragma once
#include <string>
#include <vector>
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"
#include <optional>

namespace SongData {
    enum struct BeatmapDifficulty {
        Easy,
        Normal,
        Hard,
        Expert,
        ExpertPlus
    };

    struct Contributor {
        std::string role;
        std::string name;
        std::string iconPath;
    };

    struct MapColor {
        float r;
        float g;
        float b;
    };

    struct RequirementData {
        std::string requirements;
        std::string suggestions;
        std::string warnings;
        std::string information;
    };

    struct DifficultyData {
        const rapidjson::Document& sourceDocument;
        std::string beatmapCharacteristicName;
        BeatmapDifficulty difficulty;
        std::string difficultyLabel;
        RequirementData additionalDifficultyData;
        MapColor colorLeft;
        MapColor colorRight;
        MapColor envColorLeft;
        MapColor envColorRight;
        MapColor envColorLeftBoost;
        MapColor envColorRightBoost;
        MapColor obstacleColor;

        DifficultyData(rapidjson::Document& source) : sourceDocument(source) {
            // TODO: Parse data
        }
    };

    struct CustomInfoData {
        const rapidjson::Value& source;
        std::vector<Contributor> contributors;
        std::string customEnvironment;
        std::string customEnvironmentHash;

        CustomInfoData(const rapidjson::Value& source_) : source(source_) {
            // TODO: Parse data
        }
    };

    struct DifficultyBeatmap {
        const rapidjson::Value& sourceDocument;
        std::string difficulty;
        int difficultyRank;
        std::string beatmapFilename;
        int noteJumpMovementSpeed;
        int noteJumpStartBeatOffset;

        DifficultyBeatmap(rapidjson::Value& source) : sourceDocument(source) {
            // TODO: Parse data
        }
    };

    struct DifficultyBeatmapSet {
        const rapidjson::Value& sourceDocument;

        std::string characteristicName;
        std::vector<DifficultyBeatmap> difficultyBeatmaps;
        
        DifficultyBeatmapSet(rapidjson::Value& source) : sourceDocument(source) {
            // TODO: Parse data
        }
    };

    struct SongInfo {
        bool valid;
        rapidjson::Document sourceDocument;

        std::string version;
        std::string songName;
        std::string songSubName;
        std::string songAuthorName;
        std::string levelAuthorName;
        float beatsPerMinute;
        float shuffle;
        float shufflePeriod;
        float previewStartTime;
        float previewDuration;
        std::string songFilename;
        std::string coverImageFilename;
        std::string environmentName;
        std::optional<CustomInfoData> customData;
        std::vector<DifficultyBeatmapSet> difficultyBeatmapSets;

        SongInfo(std::string_view source);
        SongInfo() : valid(false) {}
    };
}