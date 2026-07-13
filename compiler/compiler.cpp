#include "compiler.hpp"

#include <iostream>

#include "parser.hpp"

namespace bsl {

std::string compile(const std::string& in, size_t indent, bool allowTabs) {
    auto prog = parse(in, indent, allowTabs);

    return "";
}

}  // namespace bsl