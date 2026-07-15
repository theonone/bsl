#include "stringTools.hpp"

namespace bsl {

std::vector<std::string> split(const std::string& s, char delimiter, bool filterEmpty) {
    std::vector<std::string> result;
    size_t pieceBeginning = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == delimiter) {
            if (!(filterEmpty && i == pieceBeginning))
                result.push_back(s.substr(pieceBeginning, i - pieceBeginning));
            pieceBeginning = i + 1;
        }
    }
    if (!(filterEmpty && s.size() == pieceBeginning))
        result.push_back(s.substr(pieceBeginning, s.size() - pieceBeginning));

    return result;
}

std::string trim(const std::string& s, char c) {
    size_t start = 0;
    while (start < s.size() && s[start] == c) ++start;

    if (start == s.size())
        return "";  // all chars are c

    size_t end = s.size() - 1;
    while (end > start && s[end] == c) --end;

    return s.substr(start, end - start + 1);
}

std::string removeAfterSuffix(const std::string& s, const std::string& suff) {
    auto index = s.find(suff);
    if (index == std::string::npos)
        return s;
    return s.substr(0, index);
}

bool startswith(const std::string& s, const std::string& prefix) {
    if (s.size() < prefix.size())
        return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (s[i] != prefix[i])
            return false;
    }

    return true;
}

bool isNumber(const std::string& s) {
    if (s.empty())
        return false;
    size_t i = 0;
    if (s[0] == '-')
        ++i;

    for (; i < s.size(); ++i) {
        if (s[i] < 48 || s[i] > 57)
            return false;
    }

    return true;
}

}  // namespace bsl