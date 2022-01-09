#include "Utils/OggVorbisUtils.hpp"

#include "CustomLogger.hpp"

#include "Utils/FileUtils.hpp"

namespace RuntimeSongLoader::OggVorbisUtils {

    const char VORBIS_BYTES[] = { 0x76, 0x6F, 0x72, 0x62, 0x69, 0x73 }; //"vorbis"
    const char OGG_BYTES[] = { 0x4F, 0x67, 0x67, 0x53, 0x00, 0x04 }; //"OggS" + 0x00 + 0x04
    #define OGG_OFFSET (8 + 2 + 4)
    float GetLengthFromOggVorbisFile(std::string_view path) {
        
        auto start = std::chrono::high_resolution_clock::now();
        LOG_DEBUG("GetLengthFromOggVorbisFile Start");
        float length = 0.0f;

        size_t dataLength;
        auto dataStart = FileUtils::ReadAllBytes(path, dataLength);
        if(!dataStart)
            return length;
        if(dataLength <= 0) {
            delete dataStart;
            return length;
        }
        long rate = 0;
        long long lastSample = 0;

        auto endIndex = dataLength - OGG_OFFSET - 1;
        for(int i = 0; i < endIndex - sizeof(VORBIS_BYTES); i++) {
            char* index = (char*)dataStart + i;
            if(memcmp(index, VORBIS_BYTES, sizeof(VORBIS_BYTES)) == 0) {
                rate = (*reinterpret_cast<long*>(index + 11));
                break;
            }
        }
        for(int i = endIndex; i >= 0; i--) {
            char* index = (char*)dataStart + i;
            if(memcmp(index, OGG_BYTES, sizeof(OGG_BYTES)) == 0) {
                lastSample = (*reinterpret_cast<long long*>(index + 6));
                break;
            }
        }

        delete dataStart;
        if(rate != 0 & lastSample != 0)
            length = lastSample / (float)rate;

        std::chrono::milliseconds duration = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start); 
        LOG_DEBUG("GetLengthFromOggVorbisFile Stop Result %f Time %d", length, duration);
        return length;
    }

}