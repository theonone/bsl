#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace bsl {

struct Instruction {
    std::string inst;
    std::vector<std::string> args;
    std::optional<std::string> attachedScope;  // with ifs, fors, and funcs
    size_t depth;
    size_t lineNumber;
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

class BSLParser {
   private:
    std::vector<std::string> _lines;
    ProgramData _pdata;
    std::string _filename;
    size_t _indent;
    bool _allowTabs;
    bool _parsed = false;

    void _validateName(const std::string& name, size_t lineNum);

    Instruction _parseInstruction(size_t lineNumber);

    void _processScope(size_t lineNumber);

    size_t _scopeDepth(size_t lineNumber);

   public:
    BSLParser(const std::string& filename, size_t indent, bool allowTabs);
    ProgramData parse();
};

}  // namespace bsl