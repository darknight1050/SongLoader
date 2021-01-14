#pragma once
#include <list>
#include <vector>
#include <string_view>
#include <map>
#include <unordered_map>
#include "song-data.hpp"
#include "path-handling.hpp"

namespace SongLoader {
    /// @brief Reloads all songs from all paths songs have been loaded from.
    void ReloadAllSongs();
    /// @brief Reloads all songs from the specified path, which may or may not exist.
    /// If the path exists, the path is added to the collection of paths to reload from when ReloadAllSongs is called.
    /// This function supports a path to a specific file, or a folder.
    /// @param path The path to load/reload from. Supports both files and directories.
    /// @returns True if the reload was successful and the path added to the collection, false otherwise.
    bool ReloadSongs(std::string_view path);
    /// @brief Returns the collection of paths that are used for loading.
    /// These paths may be file paths or directories.
    /// @returns A const reference to the paths.
    const std::unordered_map<std::string, PathData>& GetPaths();
    /// @brief Finds and returns the corresponding rapidjson::Document for the info.dat file at the specified level ID.
    /// @param levelId The levelId to match.
    /// @returns A SongInfo reference for read-only access into the info.dat file.
    const SongData::SongInfo& GetInfo(std::string_view levelId);

    const std::map<SongData::BeatmapDifficulty, SongData::DifficultyData>& GetDifficulties(std::string_view levelId);

    /// @brief Removes the specified path from the collection of paths to search when reloading songs.
    /// If the path does not already exist within the search collection, does nothing.
    /// After this is applied, the configuration is written.
    void RemoveSearchPath(std::string_view path);
    // TODO: Get hash (from what?)
    // TODO: Audio clips/textures (write access as well?)
    // TODO: Playlist information?
    // TODO: Instead of paths from GetPaths, return full structures, which include the parse type of the path?
}