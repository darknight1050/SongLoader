#include "SongLoader.hpp"
#include "PathHandling.hpp"
#include <unordered_map>
#include "utils.hpp"

#include "GlobalNamespace/BeatmapData.hpp"

namespace SongLoader {
    /// @brief Holds all search paths for loading/reloading songs.
    static std::unordered_map<std::string, PathData> paths;
    /// @brief Represents a mapping of level ID to info.dat data for all songs that are loaded.
    static std::unordered_map<std::string, SongData::SongInfo> infoMap;
    /// @brief Represents a mapping of level ID to difficulty.dat files for all songs that are loaded.
    static std::unordered_map<std::string, std::map<SongData::BeatmapDifficulty, GlobalNamespace::BeatmapData*>> difficultyMap;

    // Returns the level ID from the provided folder path and deserialized info.dat.
    std::string GetLevelIdFromFolder(std::string_view folder, SongData::SongInfo& info) {
        // TODO: libcrypto - SHA1(info.dat bytes | difficulties.dat bytes in order listed in info)
        // Backconvert to hex string, prefix with "custom_level_"
        return "custom_level_";
    }

    void LoadSong(std::string& songFolder) {
        // This function should only ever be called if PathData.type is == PathData::Song
        suffixPath(songFolder);
        // We can assume that EITHER path/Info.dat or path/info.dat must exist.
        SongData::SongInfo info(fileexists(songFolder + "Info.dat") ? songFolder + "Info.dat" : songFolder + "info.dat");
        // Add song to infoMap, create all necessary objects for this song.
        auto levelId = GetLevelIdFromFolder(songFolder, info);
        infoMap.insert_or_assign(levelId, std::move(info));
        // TODO: Add our difficulties to our maps

        // TODO: It should be noted that we don't have to actually PERFORM the loading of the song here
        // We just need to make/allocate what we will need for the actual loading process.
        // In addition, these calls should probably be made as soon as possible (either before or during load)
        // So we will want to avoid making C# objects until absolutely necessary.
        // Therefore, we will not be making any C# objects in these calls, thus another iteration over the entire collection will be done
        // presumably during load that will take care of the allocation and management of all the types.

        // Also of note, we should write our namespace static fields out to a cache somewhere for faster loads,
        // so we don't have to load every song and parse it and recompute the hash, etc.
        // We can certainly store our paths collection + last modified time, and if any are modified when we load songs
        // for the first time, we can simply re-load those, whereas the rest we can simply use without needing to parse extra.
    }

    // Given the path and type pair, load all information as necessary, populating infoMap and difficultyMap as needed.
    void LoadFromPair(std::pair<std::string, PathData> pathPair) {
        switch (pathPair.second.type) {
            case PathType::Song:
                LoadSong(pathPair.first);
                break;
            case PathType::Directory:
                // Iterate over the whole directory, attempt to parse each directory.
                // TODO: Recursively call LoadFromPair with a pair for each valid SONG type
                // We want to ensure that we only ever load SONG types from a Directory, as opposed to playlists or configurations
                // TODO: Is this absolutely the case?
                break;
            case PathType::PlaylistFile:
                // TODO: Treat a playlist file a bit differently, perhaps have some form of downloading,
                // or perhaps think of it as a collection file?
                // Or perhaps don't support it at all?
                break;
            case PathType::ConfigurationFile:
                // TODO: Configuration file should contain everything we would want.
                // It would look like the BMBF generated JSON, or something similar.
                // Goal is to parse this and load from each of the parsed data fields.
                // This should be as simple as a little bit of JSON parsing followed by calls to ResolveFromPair or LoadSong as necessary.
            case PathType::Unknown:
            default:
                // TODO: Probably log that the given path: pathType.first has type: type and could not be parsed
        }
    }

    void ReloadAllSongs() {
        // Load each path pair entirely.
        // TODO: Do we want to actually perform a full reload here?
        // and if we do, we should probably be certain we clean up after ourselves, or at least avoid making redundant objects.
        for (auto p : paths) {
            LoadFromPair(p);
        }
    }

    bool ReloadSongs(std::string_view path) {
        // Determine if this is a valid path, if so, add it.
        // A valid path is one that can be parsed, or one that is to a directory.
        // True on success, false otherwise.
        auto type = PathHandler::GetType(path);
        if (type == PathType::Unknown) {
            return false;
        }
        // Parsing a path is not too complex, however, performing a RELOAD on a particular path with a particular type
        // is where the core of the logic lies.
        auto pair = paths.emplace(path.data(), type);
        if (!pair.second) {
            return false;
        }
        LoadFromPair(*pair.first);
        return true;
    }

    const std::unordered_map<std::string, PathData>& GetPaths() {
        return paths;
    }

    const SongData::SongInfo& GetInfo(std::string_view levelId) {
        // Return a reference to the matching item that is at this levelId.
        // If a match was not found, return a reference to a stack allocated default
        // (const reference lifetime will remain in caller scope)
        auto itr = infoMap.find(levelId.data());
        if (itr != infoMap.end()) {
            return itr->second;
        }
        return SongData::SongInfo();
    }

    const std::map<SongData::BeatmapDifficulty, GlobalNamespace::BeatmapData*>& GetDifficulties(std::string_view levelId) {
        // Return a reference to the matching collection of difficulties at this levelId.
        // If a match was not found, return a reference to a stack allocated empty map.
        auto itr = difficultyMap.find(levelId.data());
        if (itr != difficultyMap.end()) {
            return itr->second;
        }
        return std::map<SongData::BeatmapDifficulty, GlobalNamespace::BeatmapData*>();
    }

    void RemoveSearchPath(std::string_view path) {
        paths.erase(path.data());
    }
}