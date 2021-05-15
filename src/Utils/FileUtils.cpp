#include "Utils/FileUtils.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include <fstream>
#include <filesystem>

namespace RuntimeSongLoader::FileUtils {
    
    std::string ReadAllText(std::string_view path) {
        if(!fileexists(path))
            return "";
        std::ifstream fileStream(path.data(), std::ifstream::in);
        if(!fileStream.is_open())
            return "";
        std::string text((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
        return text;
    }

    std::u16string ReadAllText16(std::string_view path) {
        if(!fileexists(path))
            return u"";
        std::basic_ifstream<char16_t> fileStream(path.data(), std::ifstream::in);
        if(!fileStream.is_open())
            return u"";
        std::u16string text((std::istreambuf_iterator<char16_t>(fileStream)), std::istreambuf_iterator<char16_t>());
        return text;
    }

    const char* ReadAllBytes(std::string_view path, size_t& outSize) {
        outSize = 0;
        if(!fileexists(path))
            return nullptr;
        std::ifstream fileStream(path.data(), std::ifstream::in | std::ios::binary | std::ifstream::ate);
        outSize = fileStream.tellg();
        if(!fileStream.is_open())
            return nullptr;
        char* data = new char[outSize];
        fileStream.seekg(fileStream.beg);
        fileStream.read(data, outSize);
        return data;
    }

    std::vector<std::string> GetFolders(std::string_view path) {
        std::vector<std::string> directories;
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if(entry.is_directory())
                directories.push_back(entry.path().string());
        }
        return directories;
    }

    void DeleteFolder(std::string_view path) {
        std::filesystem::remove_all(path);
    }

}