#include "Utils/HashUtils.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "CustomLogger.hpp"

#include "Utils/FileUtils.hpp"

#include "include/cryptopp/sha.h"
#include "include/cryptopp/hex.h"
#include "include/cryptopp/files.h"

#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmap.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmapSet.hpp"

using namespace GlobalNamespace;
using namespace CryptoPP;

namespace HashUtils {
    
    std::string GetCustomLevelHash(StandardLevelInfoSaveData* level, std::string customLevelPath) {
        auto start = std::chrono::high_resolution_clock::now();
        LOG_DEBUG("GetCustomLevelHash Start");
        std::string actualPath = customLevelPath + "/Info.dat";
        if(!fileexists(actualPath)) actualPath = customLevelPath + "/info.dat";

        SHA1 hashType;
        std::string hashResult;
        HashFilter hashFilter(hashType, new StringSink(hashResult));

        FileSource fs(actualPath.c_str(), false);
        fs.Attach(new Redirector(hashFilter));
        fs.Pump(LWORD_MAX);
        fs.Detach();
        auto difficultyBeatmapSets = level->difficultyBeatmapSets;
        for(int i = 0; i < difficultyBeatmapSets->Length(); i++) {
            auto difficultyBeatmaps = difficultyBeatmapSets->values[i]->difficultyBeatmaps;
            for(int j = 0; j < difficultyBeatmaps->Length(); j++) {
                std::string diffFile = to_utf8(csstrtostr(difficultyBeatmaps->values[j]->beatmapFilename));
                std::string path = customLevelPath + "/" + diffFile;
                if(!fileexists(path)) {
                    LOG_ERROR("GetCustomLevelHash File %s did not exist", (path).c_str());
                    continue;
                } 
                FileSource fs(path.c_str(), false);
                fs.Attach(new Redirector(hashFilter));
                fs.Pump(LWORD_MAX);
                fs.Detach();
            }
        }

        hashFilter.MessageEnd();
        
        std::string hashHex;
        HexEncoder hexEncoder(new StringSink(hashHex));
        hexEncoder.Put((const byte*)hashResult.data(), hashResult.size());

        std::chrono::milliseconds duration = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start); 
        LOG_DEBUG("GetCustomLevelHash Stop Result %s Time %d", hashHex.c_str(), duration);
        return hashHex;
    }
    
}