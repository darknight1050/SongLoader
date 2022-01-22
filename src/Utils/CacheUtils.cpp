#include "Utils/CacheUtils.hpp"
#include "Utils/HashUtils.hpp"

#include "CustomConfig.hpp"
#include "CustomLogger.hpp"

#include "beatsaber-hook/shared/config/config-utils.hpp"

#include <unordered_map>
#include <algorithm>

namespace RuntimeSongLoader::CacheUtils {

    struct StringHash {
        using is_transparent = void; // enables heterogenous lookup
        std::size_t operator()(std::string_view sv) const {
            std::hash<std::string_view> hasher;
            return hasher(sv);
        }
    };

    std::unordered_map<std::string, CacheData, StringHash, std::equal_to<>> cacheMap;
    std::mutex cacheMapMutex;

    std::optional<CacheData> GetCacheData(std::string const& fullPath) {
        auto directoryHash = HashUtils::GetDirectoryHash(fullPath);
        if(!directoryHash.has_value())
        {
            LOG_DEBUG("Hash for %s did not have value!", fullPath.c_str());
            return std::nullopt;
        }
        std::unique_lock<std::mutex> lock(cacheMapMutex);
        auto search = cacheMap.find(fullPath);
        if(search != cacheMap.end()) {
            LOG_DEBUG("Found existing cache data for %s", fullPath.c_str());
            auto data = search->second;
            if(*directoryHash == data.directoryHash)
                return search->second;
        }
        lock.unlock();
        CacheData data;
        data.directoryHash = *directoryHash;
        data.sha1 = std::nullopt;
        data.songDuration = std::nullopt;
        UpdateCacheData(fullPath, data);
        return data;
    }

    void UpdateCacheData(std::string const& path, CacheData const& newData) {
        std::unique_lock<std::mutex> lock(cacheMapMutex);
        cacheMap[path] = newData;
    }

    void RemoveCacheData(std::string const& path) {
        std::unique_lock<std::mutex> lock(cacheMapMutex);
        cacheMap.erase(path);
    }

    void ClearCache() {
        std::unique_lock<std::mutex> lock(cacheMapMutex);
        cacheMap.clear();
        lock.unlock();
        SaveToFile({});
    }

    void LoadFromFile() {
        std::unique_lock<std::mutex> lock(cacheMapMutex);
        cacheMap.clear();
        lock.unlock();
        getConfig().Load();
        getConfig().Reload();
        auto& config = getConfig().config;
        for(auto it = config.MemberBegin(); it != config.MemberEnd(); it++) {
            LOG_DEBUG("CacheUtils Loading %s from cache!", it->name.GetString());
            CacheData data;
            auto& value = it->value;
            auto directoryHashIt = value.FindMember("directoryHash");
            if(directoryHashIt != value.MemberEnd() && directoryHashIt->value.IsNumber())
                data.directoryHash = directoryHashIt->value.GetInt();

            auto sha1It = value.FindMember("sha1");
            if(sha1It != value.MemberEnd() && sha1It->value.IsString()) {
                data.sha1 = sha1It->value.GetString();
                if(data.sha1.value().empty())
                    data.sha1 = std::nullopt;
            }

            auto songDurationIt = value.FindMember("songDuration");
            if(songDurationIt != value.MemberEnd() && songDurationIt->value.IsNumber()) {
                data.songDuration = songDurationIt->value.GetFloat();
                if(data.songDuration.value() <= 0.0f)
                    data.songDuration = std::nullopt;
            }
            UpdateCacheData(it->name.GetString(), data);
        }
    }

    void SaveToFile(std::vector<std::string> const& paths) {
        auto& config = getConfig().config;
        config.RemoveAllMembers();
        config.SetObject();
        if(!paths.empty()) {
            rapidjson::Document::AllocatorType& allocator = config.GetAllocator();
            std::unique_lock<std::mutex> lock(cacheMapMutex);
            for (auto it = cacheMap.cbegin(), next_it = it; it != cacheMap.cend(); it = next_it) {
                next_it++;
                auto& path = it->first;
                auto& data = it->second;
                if(std::find(paths.begin(), paths.end(), path) == paths.end()) {
                    LOG_DEBUG("CacheUtils Removing %s from cache!", path.c_str());
                    cacheMap.erase(it); //Clear unused paths
                } else {
                    LOG_DEBUG("CacheUtils Saving %s to cache!", path.c_str());
                    ConfigValue value(rapidjson::kObjectType);
                    value.AddMember("directoryHash", data.directoryHash, allocator);
                    if(data.sha1.has_value())
                        value.AddMember("sha1", *data.sha1, allocator);
                    if(data.songDuration.has_value())
                        value.AddMember("songDuration", *data.songDuration, allocator);
                    config.AddMember((ConfigValue::StringRefType)path.c_str(), value, allocator);
                }
            }
        }
        getConfig().Write();
    }

}