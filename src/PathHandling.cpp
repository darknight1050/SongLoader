#include "PathHandling.hpp"
#include "beatsaber-hook/shared/utils/utils-functions.h"
#include "utils.hpp"

namespace SongLoader {
    PathType PathHandler::GetType(std::string_view path) {
        if (path.ends_with(".json")) {
            // TODO: Differentiate between a configuration file and a playlist file and neither
            // Ideally validate using a created JSON schema for both.
            return PathType::ConfigurationFile;
        } else {
            // Check for if it is a directory or a song specifically.
            if (direxists(path)) {
                // Perform check to see if there's an info.dat under this, if there is, we assume this is a song, not a dir.
                std::string p(path.data());
                suffixPath(p);
                if (fileexists(p + "Info.dat") || fileexists(p + "info.dat")) {
                    return PathType::Song;
                }
                // Otherwise, fall back to Directory
                return PathType::Directory;
            }
        }
        // If we don't know what it is, then return Unknown
        return PathType::Unknown;
    }
}