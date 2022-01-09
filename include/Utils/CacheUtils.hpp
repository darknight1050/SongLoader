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

    void UpdateCacheData(std::string const& path, CacheData const& newData);

    void RemoveCacheData(std::string const& path);

    void ClearCache();

    void LoadFromFile();
    
    void SaveToFile(std::vector <std::string> const& paths);

}