#include "compiler.hpp"

#include <iostream>

#include "parser.hpp"

namespace bsl {

void printScope(Scope& s) {
    std::cout << "\n\nScope " << s.name << ", depth=" << s.depth << std::endl;
    for (auto& inst : s.instructions) {
        std::cout << inst.lineNumber << "| " << inst.inst << " ";
        for (auto& arg : inst.args) {
            std::cout << arg << ", ";
        }
        if (inst.attachedScope.has_value()) {
            std::cout << " -> " << inst.attachedScope.value();
        }
        std::cout << std::endl;
    }
}

void printPdata(ProgramData& pdata) {
    for (auto& d : pdata.decls) {
        std::cout << "Declaration " << d.second.type << " " << d.second.name << " = "
                  << d.second.value << std::endl;
    }

    std::cout << "Total scopes - " << pdata.scopes.size() << std::endl;

    for (auto& p : pdata.scopes) {
        printScope(p.second);
    }
}

std::string compile(const std::string& in, size_t indent, bool allowTabs) {
    auto parser = BSLParser(in, indent, allowTabs);
    auto prog = parser.parse();
    printPdata(prog);

    return "";
}

}  // namespace bsl