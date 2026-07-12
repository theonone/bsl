#include "compiler.hpp"

#include <iostream>

#include "fileIO.hpp"
#include "stringTools.hpp"

namespace bsl {

std::string compile(const std::string& in) {
    auto s = readFileAsString(in);
    auto lines = split(s, '\n', true);

    return s;
}

}  // namespace bsl