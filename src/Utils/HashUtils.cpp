#include "Utils/HashUtils.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "CustomLogger.hpp"

#include "Utils/FileUtils.hpp"
#include "Utils/CacheUtils.hpp"

#include "include/cryptopp/sha.h"
#include "include/cryptopp/hex.h"
#include "include/cryptopp/files.h"

#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmap.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData_DifficultyBeatmapSet.hpp"

#include <dirent.h>
#include <sys/stat.h>

using namespace GlobalNamespace;
using namespace CryptoPP;

namespace HashUtils {
    
    std::optional<std::string> GetCustomLevelHash(StandardLevelInfoSaveData* level, std::string customLevelPath) {
        auto start = std::chrono::high_resolution_clock::now();
        LOG_DEBUG("GetCustomLevelHash Start");

        auto cacheDataOpt = CacheUtils::GetCacheData(customLevelPath);
        if(!cacheDataOpt.has_value())
            return std::nullopt;
        auto cacheData = *cacheDataOpt;
        auto cacheSHA1 = cacheData.sha1;
        if(cacheSHA1.has_value())
            return *cacheSHA1;

        std::string actualPath = customLevelPath + "/Info.dat";
        if(!fileexists(actualPath)) 
            actualPath = customLevelPath + "/info.dat";
        if(!fileexists(actualPath)) 
            return std::nullopt;

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
                    LOG_ERROR("GetCustomLevelHash File %s did not exist", path.c_str());
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

        cacheData.sha1 = hashHex;
        CacheUtils::UpdateCacheData(customLevelPath, cacheData);

        std::chrono::milliseconds duration = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start); 
        LOG_DEBUG("GetCustomLevelHash Stop Result %s Time %d", hashHex.c_str(), duration);
        return hashHex;
    }

    std::optional<int> GetDirectoryHash(std::string_view path) {
        std::string fullPath(path);
        DIR *dir;
        struct dirent *ent;
        if((dir = opendir (fullPath.c_str())) == nullptr) {
            // could not open path to hash, return 0 or some other shit
            closedir(dir);
            return std::nullopt;
        }
        // We return 0 if we have nothing in our folder. This can also be the case if we collide on 0.
        // Perhaps we want to return std::nullopt instead?
        int value = 0;
        while((ent = readdir (dir)) != nullptr) {
            std::string name = ent->d_name;
            if(name != "." && name != "..") {
                // Each file entry in the directory is hashed, we only check top level file entries
                struct stat st;
                if (stat((fullPath + "/" + name).c_str(), &st)) {
                    // Error
                    return std::nullopt;
                }
                // Otherwise, xor all relevant fields of stat
                value ^= st.st_size;
                value ^= st.st_mtime;
            }
        }
        closedir(dir);
        return value;
    }
    
}