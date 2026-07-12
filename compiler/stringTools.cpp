#include "stringTools.hpp"

namespace bsl {

std::vector<std::string> split(const std::string& s, char delimeter, bool filterEmpty) {
    std::vector<std::string> result;
    size_t pieceBeginning = 0;
    for (size_t i = 0; i < s.length(); ++i) {
        if (s[i] == delimeter) {
            if (!(filterEmpty && i == pieceBeginning))
                result.push_back(s.substr(pieceBeginning, i - pieceBeginning));
            pieceBeginning = i + 1;
        }
    }
    if (!(filterEmpty && s.length() == pieceBeginning))
        result.push_back(s.substr(pieceBeginning, s.length() - pieceBeginning));

    return result;
}

}  // namespace bsl