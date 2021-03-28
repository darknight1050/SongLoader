#pragma once
#include <string>

namespace RuntimeSongLoader::OggVorbisUtils {
    
    float GetLengthFromOggVorbisFile(std::string_view path);

}