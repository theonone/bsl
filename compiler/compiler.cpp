#include "compiler.hpp"

#include <iostream>

#include "fileIO.hpp"

namespace bsl {

std::string compile(const std::string& in) {
    auto s = readFileAsString(in);

    return s;
}

}  // namespace bsl