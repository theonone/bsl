#include "compiler.hpp"

#include <iostream>

#include "parser.hpp"

namespace bsl {

std::string compile(const std::string& in, size_t indent, bool allowTabs) {
    auto parser = BSLParser(in, indent, allowTabs);
    auto prog = parser.parse();

    return "";
}

}  // namespace bsl