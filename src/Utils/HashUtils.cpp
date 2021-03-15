#include "Utils/HashUtils.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "CustomLogger.hpp"

#include "Utils/FileUtils.hpp"

#include "include/cryptopp/sha.h"
#include "include/cryptopp/hex.h"

#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmap.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmapSet.hpp"

#include "System/BitConverter.hpp"
#include "System/Security/Cryptography/SHA1.hpp"

using namespace GlobalNamespace;

#include <future>
#include <vector>
#include <algorithm>

namespace HashUtils {
    
    std::string GetSHA1(std::string& data) {
        CryptoPP::byte digest[CryptoPP::SHA1::DIGESTSIZE];
        CryptoPP::SHA1().CalculateDigest(digest, reinterpret_cast<const CryptoPP::byte*>(data.c_str()), data.size());
        std::string hash;
        CryptoPP::HexEncoder encoder;
        encoder.Attach(new CryptoPP::StringSink(hash));
        encoder.Put(digest, sizeof(digest));
        encoder.MessageEnd();
        return hash;
    }

    void strToLower(std::string& str) {
        std::for_each(str.begin(), str.end(), [](char& c) { c = std::tolower(c); });
    }

    std::string GetCustomLevelHash(StandardLevelInfoSaveData* level, std::string customLevelPath) {
        auto start = std::chrono::high_resolution_clock::now();
        LOG_DEBUG("GetCustomLevelHash Start");
        std::string actualPath = customLevelPath + "/Info.dat";
        if (!fileexists(actualPath)) actualPath = customLevelPath + "/info.dat";
            
        LOG_DEBUG("GetCustomLevelHash Reading all bytes from %s", actualPath.c_str());

        LOG_DEBUG("GetCustomLevelHash Starting reading beatmaps");
        std::vector<std::shared_future<std::string>> futures;
        futures.push_back(std::async(std::launch::async, 
            [&]() {
                return FileUtils::ReadAllText(actualPath);
            }
        ));
        for (int i = 0; i < level->get_difficultyBeatmapSets()->Length(); i++) {
            for (int j = 0; j < level->get_difficultyBeatmapSets()->values[i]->get_difficultyBeatmaps()->Length(); j++) {
                std::string diffFile = to_utf8(csstrtostr(level->get_difficultyBeatmapSets()->values[i]->get_difficultyBeatmaps()->values[j]->get_beatmapFilename()));
                if (!fileexists(customLevelPath + "/" + diffFile)) {
                    LOG_ERROR("GetCustomLevelHash File %s did not exist", (customLevelPath + "/" + diffFile).c_str());
                    continue;
                } 
                futures.push_back(std::async(std::launch::async, 
                    [&]() {
                        return FileUtils::ReadAllText(customLevelPath + "/" + diffFile);;
                    }
                ));
            }
        }
        std::string hash;
        for_each(futures.begin(), futures.end(), 
            [&](std::shared_future<std::string> future) {
                hash += future.get();
            }
        );

        std::chrono::milliseconds durationFiles = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start); 
        start = std::chrono::high_resolution_clock::now();
        
        LOG_DEBUG("GetCustomLevelHash Start hashing");
        hash = GetSHA1(hash);
        strToLower(hash);

        std::chrono::milliseconds durationHashing = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start); 
        LOG_DEBUG("GetCustomLevelHash Stop %s Time files %d hashing: %d", hash.c_str(), durationFiles, durationHashing);
        return hash;
    }
    
}