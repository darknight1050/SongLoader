#include "Utils/HashUtils.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "CustomLogger.hpp"

#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmap.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmapSet.hpp"

#include "System/BitConverter.hpp"
#include "System/Security/Cryptography/SHA1.hpp"

using namespace GlobalNamespace;

namespace HashUtils {
    
    std::string GetCustomLevelHash(StandardLevelInfoSaveData* level, std::string customLevelPath) {
        auto start = std::chrono::high_resolution_clock::now(); 
        LOG_DEBUG("GetCustomLevelHash Start");
        std::string actualPath = customLevelPath + "/Info.dat";
        if (!fileexists(actualPath)) actualPath = customLevelPath + "/info.dat";
            
        std::string hash = "";

        LOG_DEBUG("GetCustomLevelHash Reading all bytes from %s", actualPath.c_str());

        std::vector<char> bytesAsChar = readbytes(actualPath);
        LOG_DEBUG("GetCustomLevelHash Starting reading beatmaps");
        for (int i = 0; i < level->get_difficultyBeatmapSets()->Length(); i++) {
            for (int j = 0; j < level->get_difficultyBeatmapSets()->values[i]->get_difficultyBeatmaps()->Length(); j++) {
                std::string diffFile = to_utf8(csstrtostr(level->get_difficultyBeatmapSets()->values[i]->get_difficultyBeatmaps()->values[j]->get_beatmapFilename()));
                if (!fileexists(customLevelPath + "/" + diffFile)) {
                    LOG_ERROR("GetCustomLevelHash File %s did not exist", (customLevelPath + "/" + diffFile).c_str());
                    continue;
                } 

                std::vector<char> currentDiff = readbytes(customLevelPath + "/" + diffFile);
                bytesAsChar.insert(bytesAsChar.end(), currentDiff.begin(), currentDiff.end());
            }
        }
        Array<uint8_t>* bytes = Array<uint8_t>::NewLength(bytesAsChar.size());
        for(int i = 0;i<bytes->Length();i++) {
            bytes->values[i] = bytesAsChar[i];
        }
        LOG_DEBUG("GetCustomLevelHash computing Hash, found %d bytes", bytes->Length());
        hash = to_utf8(csstrtostr(System::BitConverter::ToString(System::Security::Cryptography::SHA1::Create()->ComputeHash(bytes))->ToLower()));

        hash.erase(std::remove(hash.begin(), hash.end(), '-'), hash.end());
            
        std::chrono::milliseconds duration = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start); 
        LOG_DEBUG("GetCustomLevelHash Stop %s Time %d", hash.c_str(), duration);
        return hash;
    }
    
}