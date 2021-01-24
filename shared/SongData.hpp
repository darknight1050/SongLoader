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

    struct CustomDifficultyData {
        const rapidjson::Value& source;

        std::optional<BeatmapDifficulty> difficulty;
        std::optional<std::string> difficultyLabel;
        std::optional<MapColor> colorLeft;
        std::optional<MapColor> colorRight;
        std::optional<MapColor> envColorLeft;
        std::optional<MapColor> envColorRight;
        std::optional<MapColor> envColorLeftBoost;
        std::optional<MapColor> envColorRightBoost;
        std::optional<MapColor> obstacleColor;
        std::optional<std::vector<std::string>> requirements;
        std::optional<std::vector<std::string>> suggestions;
        std::optional<std::vector<std::string>> warnings;
        std::optional<std::vector<std::string>> information;

        CustomDifficultyData(const rapidjson::Value& source_) : source(source_) {
            // TODO: Parse data
        }
    };

    struct CustomInfoData {
        const rapidjson::Value& source;
        
        std::optional<std::vector<Contributor>> contributors;
        std::optional<std::string> customEnvironment;
        std::optional<std::string> customEnvironmentHash;

        CustomInfoData(const rapidjson::Value& source_) : source(source_) {
            // TODO: Parse data
        }
    };

    struct DifficultyBeatmap {
        const rapidjson::Value& source;

        BeatmapDifficulty difficulty;
        int difficultyRank;
        std::string beatmapFilename;
        int noteJumpMovementSpeed;
        int noteJumpStartBeatOffset;
        std::optional<CustomDifficultyData> customData;

        DifficultyBeatmap(rapidjson::Value& source);
    };

    struct DifficultyBeatmapSet {
        const rapidjson::Value& source;

        std::string characteristicName;
        std::vector<DifficultyBeatmap> difficultyBeatmaps;
        
        DifficultyBeatmapSet(rapidjson::Value& source);
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
        float songTimeOffset;
        float shuffle;
        float shufflePeriod;
        float previewStartTime;
        float previewDuration;
        std::string songFilename;
        std::string coverImageFilename;
        std::string environmentName;
        std::string allDirectionsEnvironmentName;
        std::vector<DifficultyBeatmapSet> difficultyBeatmapSets;
        std::optional<CustomInfoData> customData;

        SongInfo(std::string_view source);
        SongInfo() : valid(false) {}
    };
}