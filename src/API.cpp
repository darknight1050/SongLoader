#include "API.hpp"

#include "CustomTypes/SongLoader.hpp"

namespace RuntimeSongLoader::API {

    void RefreshSongs(bool fullRefresh) {
        SongLoader::GetInstance()->RefreshSongs(fullRefresh);
    }

}