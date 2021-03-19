#pragma once

#include <string>
#include <optional>
#include <vector>

namespace CacheUtils {

    struct CacheData {
        int directoryHash = 0;
        std::optional<std::string> sha1 = std::nullopt;
        std::optional<float> songDuration = std::nullopt;
    };

    std::optional<CacheData> GetCacheData(std::string_view path);

    void UpdateCacheData(std::string path, CacheData newData);

    void LoadFromFile();
    void SaveToFile(std::vector<std::string> paths);

}