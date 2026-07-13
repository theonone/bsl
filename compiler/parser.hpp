#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace bsl {

struct Instruction {
    std::string inst;
    std::vector<std::string> operands;
    std::optional<std::string> attachedScope;  // with ifs, fors, and funcs
};

struct Decl {
    std::string name;
    std::string value;
    std::string type;
};

struct Scope {
    std::string name;
    std::vector<Instruction> instructions;
};

struct ProgramData {
    std::map<std::string, Decl> decls;
    std::map<std::string, Scope> scopes;
};

ProgramData parse(const std::string& filename, size_t indent, bool allowTabs);

void processScope(const std::vector<std::string>& lines, ProgramData& data, size_t atLine,
                  size_t indent, bool allowTabs);

}  // namespace bsl
