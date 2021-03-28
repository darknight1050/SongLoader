#include "Utils/CacheUtils.hpp"
#include "Utils/HashUtils.hpp"

#include "CustomConfig.hpp"
#include "CustomLogger.hpp"

#include "beatsaber-hook/shared/config/config-utils.hpp"

#include <map>
#include <algorithm>

namespace RuntimeSongLoader::CacheUtils {

    std::map<std::string, CacheData> cacheMap;

    std::optional<CacheData> GetCacheData(std::string_view path) {
        std::string fullPath(path);
        auto directoryHash = HashUtils::GetDirectoryHash(fullPath);
        if(!directoryHash.has_value())
            return std::nullopt;
        auto search = cacheMap.find(fullPath);
        if(search != cacheMap.end()) {
            auto data = search->second;
            if(*directoryHash == data.directoryHash)
                return search->second;
        }
        CacheData data;
        data.directoryHash = *directoryHash;
        data.sha1 = std::nullopt;
        data.songDuration = std::nullopt;
        UpdateCacheData(fullPath, data);
        return data;
    }

    void UpdateCacheData(std::string path, CacheData newData) {
        cacheMap[path] = newData;
    }

    void ClearCache() {
        cacheMap.clear();
        SaveToFile({});
    }

    void LoadFromFile() {
        cacheMap.clear();
        getConfig().Load();
        getConfig().Reload();
        auto& config = getConfig().config;
        for(auto it = config.MemberBegin(); it != config.MemberEnd(); it++) {
            LOG_DEBUG("CacheUtils Loading %s from cache!", it->name.GetString());
            CacheData data;
            auto& value = it->value;
            if(value.HasMember("directoryHash") && value["directoryHash"].IsInt())
                data.directoryHash = value["directoryHash"].GetInt();

            if(value.HasMember("sha1") && value["sha1"].IsString()) {
                data.sha1 = value["sha1"].GetString();
                if(data.sha1.value().empty())
                    data.sha1 = std::nullopt;
            }

            if(value.HasMember("songDuration") && value["songDuration"].IsFloat()) {
                data.songDuration = value["songDuration"].GetFloat();
                if(data.songDuration.value() <= 0.0f)
                    data.songDuration = std::nullopt;
            }
            UpdateCacheData(it->name.GetString(), data);
        }
    }

    void SaveToFile(std::vector<std::string> paths) {
        auto& config = getConfig().config;
        config.RemoveAllMembers();
        config.SetObject();
        if(paths.size() > 0) {
            rapidjson::Document::AllocatorType& allocator = config.GetAllocator();
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