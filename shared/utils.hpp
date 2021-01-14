#pragma once
#include <string>
#include <string_view>

inline std::string pathJoin(std::string_view path1, std::string_view path2) {
    if (path1.ends_with("/")) {
        return std::string(path1) + path2.data();
    }
    return std::string(path1) + "/" + path2.data();
}

inline void pathJoin(std::string& lhs, std::string_view rhs) {
    if (lhs.ends_with("/")) {
        lhs.append(rhs);
    } else {
        lhs.append("/").append(rhs);
    }
}

inline void suffixPath(std::string& lhs) {
    if (!lhs.ends_with("/")) {
        lhs.append("/");
    }
}