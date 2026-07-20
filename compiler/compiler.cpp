#include "compiler.hpp"

#include <iostream>

#include "codegen.hpp"
#include "parser.hpp"

namespace bsl {

std::string compile(const std::string& in, std::vector<std::string>& lines, size_t indent,
                    bool allowTabs, const std::string& os, const std::string& arch) {
    auto parser = BSLParser(in, lines, indent, allowTabs);
    auto prog = parser.parse();

    if (arch != "x86_64" || os != "linux") {
        throw std::runtime_error("Sorry, currently the compiler can only compile for x86_64 Linux");
    }
    auto translator = X86_64Translator(prog, in);
    return translator.translate();
}

}  // namespace bsl