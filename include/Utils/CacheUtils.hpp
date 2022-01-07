#pragma once

#include <string>
#include <optional>
#include <vector>

namespace RuntimeSongLoader::CacheUtils {

    struct CacheData {
        int directoryHash = 0;
        std::optional<std::string> sha1 = std::nullopt;
        std::optional<float> songDuration = std::nullopt;
    };

    std::optional<CacheData> GetCacheData(std::string const& path);

    void UpdateCacheData(const std::string& path, CacheData newData);

    void RemoveCacheData(const std::string& path);

    void ClearCache();

    void LoadFromFile();
    
    void SaveToFile(const std::vector <std::string> &paths);

}