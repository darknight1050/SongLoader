#pragma once
#include <string>
#include <vector>

namespace RuntimeSongLoader::FileUtils {
    
    std::string ReadAllText(std::string_view path);

    std::u16string ReadAllText16(std::string_view path);

    const char* ReadAllBytes(std::string_view path, size_t& outSize);

    std::vector<std::string> GetFolders(std::string_view path);

}