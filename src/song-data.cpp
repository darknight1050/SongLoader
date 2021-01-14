#include "song-data.hpp"

namespace SongData {
    SongInfo::SongInfo(std::string_view source) {
        sourceDocument.Parse(source.data());
        if (sourceDocument.HasParseError()) {
            return;
        }
        auto itr = sourceDocument.FindMember("_customData");
        if (itr != sourceDocument.MemberEnd()) {
            customData.emplace(CustomInfoData(itr->value));
        }
        // TODO: Parse data
    }
}