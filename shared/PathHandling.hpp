#pragma once
#include <string_view>
#include <string>

namespace SongLoader {
    enum struct PathType {
        Unknown,
        Directory,
        ConfigurationFile,
        PlaylistFile,
        Song
    };

    struct PathData {
        PathType type;
    };


    struct PathHandler {
        /// @brief Returns the type of the path.
        /// @param path The path to check.
        /// @returns The resultant PathType, or Unknown if invalid or unknown.
        static PathType GetType(std::string_view path);
    };
}