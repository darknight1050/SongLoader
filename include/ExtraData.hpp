#include "CodegenIncludes.hpp"
#include <string>
#include <vector>

//TODO: do this
struct Contributor
{
    Contributor()
    {
    }
};

struct MapColor
{
    float r;
    float g;
    float b;
    MapColor(float R, float G, float B)
    {
        r = R;
        g = G;
        b = B;
    }
    MapColor()
    {
        r = 0.0f;
        g = 0.0f;
        b = 0.0f;
    }
};

struct MapColorSet
{
    MapColor colorLeft;
    MapColor colorRight;
    MapColor envColorLeft;
    MapColor envColorRight;
    MapColor envColorLeftBoost;
    MapColor envColorRightBoost;
    MapColor obstacleColor;
    MapColorSet()
    {
        
    }
};

struct DifficultyData
{
    std::string _beatmapCharacteristicName;
    GlobalNamespace::BeatmapDifficulty _difficulty;
    std::string _difficultyLabel;
    //RequirementData additionalDifficultyData;
    MapColor _colorLeft;
    MapColor _colorRight;
    MapColor _envColorLeft;
    MapColor _envColorRight;
    MapColor _envColorLeftBoost;
    MapColor _envColorRightBoost;
    MapColor _obstacleColor;
    
    DifficultyData(std::string beatmapCharacteristicName, GlobalNamespace::BeatmapDifficulty difficulty, std::string difficultyLabel, MapColorSet Colors)
    {
        _beatmapCharacteristicName = beatmapCharacteristicName;
        _difficulty = difficulty;
        _difficultyLabel = difficultyLabel;
        _colorLeft = Colors.colorLeft;
        _colorRight = Colors.colorRight;
        _envColorLeft = Colors.envColorLeft;
        _envColorRight = Colors.envColorRight;
        _envColorLeftBoost = Colors.envColorLeftBoost;
        _envColorRightBoost = Colors.envColorRightBoost;
        _obstacleColor = Colors.obstacleColor; 
    }
    DifficultyData()
    {

    }
};

GlobalNamespace::BeatmapDifficulty GetDiffFromName(std::string Name)
{
    GlobalNamespace::BeatmapDifficulty Out = GlobalNamespace::BeatmapDifficulty::Normal;
    if(Name == "Easy") Out = GlobalNamespace::BeatmapDifficulty::Easy;
    if(Name == "Normal") Out = GlobalNamespace::BeatmapDifficulty::Normal;
    if(Name == "Hard") Out = GlobalNamespace::BeatmapDifficulty::Hard;
    if(Name == "Expert") Out = GlobalNamespace::BeatmapDifficulty::Expert;
    if(Name == "ExpertPlus") Out = GlobalNamespace::BeatmapDifficulty::ExpertPlus;
    return Out;
}

struct ExtraSongData
{
    //std::vector<Contributor> contributors;
    std::vector<DifficultyData> _difficulties;
    std::string _defaultCharacteristic = "";
    bool IsInitialized;
    ExtraSongData()
    {
        IsInitialized = false;
    }
    ExtraSongData(std::string levelID, std::string songPath)
    {
        using namespace il2cpp_utils;
        try
        {
            Il2CppString* InfoDatPath = createcsstr(songPath + "/info.dat");
            if (!System::IO::File::Exists(InfoDatPath)) return;
            auto InfoJson = to_utf8(csstrtostr(System::IO::File::ReadAllText(InfoDatPath)));

            rapidjson::Document d;
            d.Parse(InfoJson);    

            
            if(d.HasMember("_customData"))
            {
                rapidjson::Value CustomData = d["_customData"].GetObject();
                
                if(CustomData.HasMember("_defaultCharacteristic"))
                {
                    _defaultCharacteristic = CustomData["_defaultCharacteristic"].GetString();
                }
            }
            std::vector<DifficultyData> diffData;
            auto diffSets = d["_difficultyBeatmapSets"].GetArray();
            
            for(int i = 0; i > diffSets.Size(); i++)
            {
                rapidjson::Value diffSet = diffSets[i].GetObject();
                std::string SetCharacteristic = diffSet["_beatmapCharacteristicName"].GetString();
                auto diffBeatmaps = diffSet["_difficultyBeatmaps"].GetArray();
                for(int x = 0; x > diffBeatmaps.Size(); x++)
                {
                    rapidjson::Value diffBeatmap = diffBeatmaps[x].GetObject();

                    std::string diffLabel;
                    MapColorSet Colors;

                    GlobalNamespace::BeatmapDifficulty diffDifficulty = GetDiffFromName(diffBeatmap["_difficulty"].GetString());

                    rapidjson::Value customBeatmapData;
                    if(diffBeatmap.HasMember("_customData"))
                    {
                        customBeatmapData = diffBeatmap["_customData"].GetObject();

                        if(customBeatmapData.HasMember("_difficultyLabel"))
                        {
                            diffLabel = customBeatmapData["_difficultyLabel"].GetString();
                        }

                        if(customBeatmapData.HasMember("_colorLeft"))
                        {
                            rapidjson::Value SaberLeft = customBeatmapData["_colorLeft"].GetObject();
                            
                            Colors.colorLeft = MapColor(0, 0, 0);
                            Colors.colorLeft.r = SaberLeft["r"].GetFloat();
                            Colors.colorLeft.g = SaberLeft["g"].GetFloat();
                            Colors.colorLeft.b = SaberLeft["b"].GetFloat();
                        }
                        if(customBeatmapData.HasMember("_colorRight"))
                        {
                            rapidjson::Value SaberRight = customBeatmapData["_colorRight"].GetObject();
                            
                            Colors.colorRight = MapColor(0, 0, 0);
                            Colors.colorRight.r = SaberRight["r"].GetFloat();
                            Colors.colorRight.g = SaberRight["g"].GetFloat();
                            Colors.colorRight.b = SaberRight["b"].GetFloat();
                        }
                        if(customBeatmapData.HasMember("_envColorLeft"))
                        {
                            rapidjson::Value envleft = customBeatmapData["_envColorLeft"].GetObject();
                            
                            Colors.envColorLeft = MapColor(0, 0, 0);
                            Colors.envColorLeft.r = envleft["r"].GetFloat();
                            Colors.envColorLeft.g = envleft["g"].GetFloat();
                            Colors.envColorLeft.b = envleft["b"].GetFloat();
                        }
                        if(customBeatmapData.HasMember("_envColorRight"))
                        {
                            rapidjson::Value envRight = customBeatmapData["_envColorRight"].GetObject();
                            
                            Colors.envColorRight = MapColor(0, 0, 0);
                            Colors.envColorRight.r = envRight["r"].GetFloat();
                            Colors.envColorRight.g = envRight["g"].GetFloat();
                            Colors.envColorRight.b = envRight["b"].GetFloat();
                        }
                        if(customBeatmapData.HasMember("_envColorLeftBoost"))
                        {
                            rapidjson::Value envleftBoost = customBeatmapData["_envColorLeftBoost"].GetObject();
                            
                            Colors.envColorLeftBoost = MapColor(0, 0, 0);
                            Colors.envColorLeftBoost.r = envleftBoost["r"].GetFloat();
                            Colors.envColorLeftBoost.g = envleftBoost["g"].GetFloat();
                            Colors.envColorLeftBoost.b = envleftBoost["b"].GetFloat();
                        }
                        if(customBeatmapData.HasMember("_envColorRightBoost"))
                        {
                            rapidjson::Value envrightBoost = customBeatmapData["_envColorRightBoost"].GetObject();
                            
                            Colors.envColorRightBoost = MapColor(0, 0, 0);
                            Colors.envColorRightBoost.r = envrightBoost["r"].GetFloat();
                            Colors.envColorRightBoost.g = envrightBoost["g"].GetFloat();
                            Colors.envColorRightBoost.b = envrightBoost["b"].GetFloat();
                        }
                        if(customBeatmapData.HasMember("_obstacleColor"))
                        {
                            rapidjson::Value obstacle = customBeatmapData["_obstacleColor"].GetObject();
                            
                            Colors.obstacleColor = MapColor(0, 0, 0);
                            Colors.obstacleColor.r = obstacle["r"].GetFloat();
                            Colors.obstacleColor.g = obstacle["g"].GetFloat();
                            Colors.obstacleColor.b = obstacle["b"].GetFloat();
                        }
                    }
                    auto New = DifficultyData
                    (
                        SetCharacteristic,
                        diffDifficulty,
                        diffLabel,
                        //diffReqData,
                        Colors
                    );
                    diffData.emplace_back(New);
                }
             }
            _difficulties = diffData;
            IsInitialized = true;
        }
        catch(const std::exception& e)
        {
            getLogger().error("Error getting extra data for " + songPath + " !");
        }
        
    }
};

static ExtraSongData CurrentSongData;